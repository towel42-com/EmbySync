// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software iRHS
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "SyncSystem.h"
#include "ProgressSystem.h"
#include "UserData.h"
#include "UserServerData.h"
#include "Settings.h"
#include "UsersModel.h"
#include "MediaModel.h"
#include "ServerModel.h"

#include "ServerInfo.h"
#include "MediaData.h"
#include "MediaServerData.h"
#include "SABUtils/StringUtils.h"

#include <unordered_set>

#include <QTimer>
#include <QDebug>
#include <QCloseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QScrollBar>
#include <QSettings>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QBuffer>
#include <QUrlQuery>

QString toString( ERequestType request )
{
    switch ( request )
    {
        case ERequestType::eNone: return "None";
        case ERequestType::eGetServerInfo: return "GetServerInfo";
        case ERequestType::eGetServerHomePage: return "GetServerHomePage";
        case ERequestType::eGetServerIcon: return "GetServerIcon";
        case ERequestType::eGetUsers: return "GetUsers";
        case ERequestType::eGetUser: return "GetUser";
        case ERequestType::eGetUserAvatar: return "GetUserAvatar";
        case ERequestType::eSetUserAvatar: return "SetUserAvatar";
        case ERequestType::eGetMediaList: return "GetMediaList";
        case ERequestType::eReloadMediaData: return "ReloadMediaData";
        case ERequestType::eUpdateUserMediaData: return "UpdateData";
        case ERequestType::eUpdateFavorite: return "UpdateFavorite";
        case ERequestType::eTestServer: return "TestServer";
        case ERequestType::eDeleteConnectedID: return "DeleteConnectedID";
        case ERequestType::eSetConnectedID: return "SetConnectedID";
        case ERequestType::eUpdateUserData: return "UpdateUserData";
        case ERequestType::eGetMissingEpisodes: return "GetMissingEpisodes";
        case ERequestType::eGetMissingTVDBid: return "GetMissingTVDBid";
        case ERequestType::eGetAllMovies: return "GetAllMovies";
        case ERequestType::eGetAllCollections: return "GetAllCollections";
        case ERequestType::eGetAllCollectionsEx: return "GetAllCollectionsEx";
        case ERequestType::eGetCollection: return "GetCollection";
        case ERequestType::eCreateCollection: return "CreateCollection";
    }
    return {};
}

QString toString( EMsgType type )
{
    switch ( type )
    {
        case EMsgType::eError: return "ERROR";
        case EMsgType::eWarning: return "WARNING";
        case EMsgType::eInfo: return "INFO";
        default:
            return {};
            break;
    }
}

QString createMessage( EMsgType msgType, const QString & msg )
{
    auto realMsg = QString( "%1" ).arg( msg.endsWith( '\n' ) ? msg.left( msg.length() - 1 ) : msg );
    auto fullMsg = QString( "%1: - %2 - %3" ).arg( toString( msgType ).toUpper() ).arg( QDateTime::currentDateTime().toString( "MM-dd-yyyy hh:mm:ss.zzz" ) ).arg( realMsg );
    return fullMsg;
}

CSyncSystem::CSyncSystem( std::shared_ptr< CSettings > settings, std::shared_ptr< CUsersModel > usersModel, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CServerModel > serverModel, QObject * parent ) :
    QObject( parent ),
    fSettings( settings ),
    fUsersModel( usersModel ),
    fMediaModel( mediaModel ),
    fServerModel( serverModel ),
    fProgressSystem( new CProgressSystem )
{
    fManager = new QNetworkAccessManager( this );
#if QT_VERSION > QT_VERSION_CHECK( 5, 14, 0 )
    fManager->setAutoDeleteReplies( true );
#endif

    connect( fManager, &QNetworkAccessManager::authenticationRequired, this, &CSyncSystem::slotAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::encrypted, this, &CSyncSystem::slotEncrypted );
    connect( fManager, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, &CSyncSystem::slotPreSharedKeyAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::proxyAuthenticationRequired, this, &CSyncSystem::slotProxyAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::sslErrors, this, &CSyncSystem::slotSSlErrors );
    connect( fManager, &QNetworkAccessManager::finished, this, &CSyncSystem::slotRequestFinished );
}

void CSyncSystem::setProcessNewMediaFunc( std::function< void( std::shared_ptr< CMediaData > userData ) > processNewMediaFunc )
{
    fProcessNewMediaFunc = processNewMediaFunc;
}

void CSyncSystem::setUserMsgFunc( std::function< void( EMsgType msgType, const QString & title, const QString & msg ) > userMsgFunc )
{
    fUserMsgFunc = userMsgFunc;
}

void CSyncSystem::setProgressSystem( std::shared_ptr< CProgressSystem > funcs )
{
    fProgressSystem = funcs;
}

void CSyncSystem::reset()
{
    fAttributes.clear();
}

void CSyncSystem::loadServerInfo()
{
    if ( !fServerModel->canAnyServerSync() )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Server Settings are not Setup" ), tr( "Server Settings are not setup.  Please fix and try again" ) );
        return;
    }

    fProgressSystem->setTitle( tr( "Loading Server Information" ) );
    auto enabledServers = fServerModel->enabledServerCnt();
    fProgressSystem->setMaximum( enabledServers );

    for ( auto && serverInfo : *fServerModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;
        requestGetServerInfo( serverInfo->keyName() );
        requestGetServerHomePage( serverInfo->keyName() );
    }
}

void CSyncSystem::loadUsers()
{
    if ( !fServerModel->canAnyServerSync() )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Server Settings are not Setup" ), tr( "Server Settings are not setup.  Please fix and try again" ) );
        return;
    }

    fProgressSystem->setTitle( tr( "Loading Users" ) );
    auto enabledServers = fServerModel->enabledServerCnt();
    fProgressSystem->setMaximum( enabledServers );

    for ( auto && serverInfo : *fServerModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;
        requestGetUsers( serverInfo->keyName() );
    }
}

void CSyncSystem::loadUsersMedia( ETool tool, std::shared_ptr< CUserData > userData )
{
    if ( !userData )
        return;

    if ( !setCurrentUser( tool, userData ) )
        return;

    fProgressSystem->setTitle( tr( "Loading Users Media" ) );
    for ( auto && serverInfo : *fServerModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;

        emit sigAddToLog( EMsgType::eInfo, QString( "Loading media for '%1' on server '%2'" ).arg( currUser().second->userName( serverInfo->keyName() ) ).arg( serverInfo->displayName() ) );
        requestGetMediaList( serverInfo->keyName() );
    }
}

bool CSyncSystem::setCurrentUser( ETool tool, std::shared_ptr<CUserData> userData, bool forSync )
{
    if ( !userData )
        return false;
    if ( forSync && !userData->canBeSynced() )
        return false;

    fCurrUserData = { tool, userData };
    return true;
}

bool CSyncSystem::loadMissingEpisodes( std::shared_ptr<const CServerInfo> serverInfo, const QDate & minPremiereDate, const QDate & maxPremiereDate )
{
    for( auto && userInfo : *fUsersModel )
    {
        if ( userInfo->isAdmin( serverInfo->keyName() ) )
        {
            return loadMissingEpisodes( userInfo, serverInfo, minPremiereDate, maxPremiereDate );
        }
    }
    return false;
}

bool CSyncSystem::loadMissingEpisodes( std::shared_ptr< CUserData > userData, std::shared_ptr<const CServerInfo> serverInfo, const QDate & minPremiereDate, const QDate & maxPremiereDate )
{
    if ( !serverInfo || !serverInfo->isEnabled() )
        return false;

    if ( !userData->isAdmin( serverInfo->keyName() ) )
        return false;

    if ( !setCurrentUser( ETool::eMissingEpisodes, userData, false ) )
        return false;

    emit sigAddToLog( EMsgType::eInfo, QString( "Loading Missing Episodes on server '%1' using admin user '%2'" ).arg( serverInfo->displayName() ).arg( userData->userName( serverInfo->keyName() ) ) );
    requestMissingEpisodes( serverInfo->keyName(), minPremiereDate, maxPremiereDate );
    return true;
}

bool CSyncSystem::loadMissingTVDBid( std::shared_ptr<const CServerInfo> serverInfo )
{
    for ( auto && userInfo : *fUsersModel )
    {
        if ( userInfo->isAdmin( serverInfo->keyName() ) )
        {
            return loadMissingTVDBid( userInfo, serverInfo );
        }
    }
    return false;
}

bool CSyncSystem::loadMissingTVDBid( std::shared_ptr< CUserData > userData, std::shared_ptr<const CServerInfo> serverInfo )
{
    if ( !serverInfo || !serverInfo->isEnabled() )
        return false;

    if ( !userData->isAdmin( serverInfo->keyName() ) )
        return false;

    if ( !setCurrentUser( ETool::eMissingTMDBId, userData, false ) )
        return false;

    emit sigAddToLog( EMsgType::eInfo, QString( "Loading Missing TVDBid on server '%1' using admin user '%2'" ).arg( serverInfo->displayName() ).arg( userData->userName( serverInfo->keyName() ) ) );
    requestMissingTVDBid( serverInfo->keyName() );
    return true;
}

bool CSyncSystem::loadAllMovies( std::shared_ptr<const CServerInfo> serverInfo )
{
    for ( auto && userInfo : *fUsersModel )
    {
        if ( userInfo->isAdmin( serverInfo->keyName() ) )
        {
            return loadAllMovies( userInfo, serverInfo );
        }
    }
    return false;
}

bool CSyncSystem::loadAllMovies( std::shared_ptr< CUserData > userData, std::shared_ptr<const CServerInfo> serverInfo )
{
    if ( !serverInfo || !serverInfo->isEnabled() )
        return false;

    if ( !userData->isAdmin( serverInfo->keyName() ) )
        return false;

    if ( !setCurrentUser( ETool::eMissingMovies, userData, false ) )
        return false;

    emit sigAddToLog( EMsgType::eInfo, QString( "Loading All Movies on server '%1' using admin user '%2'" ).arg( serverInfo->displayName() ).arg( userData->userName( serverInfo->keyName() ) ) );
    requestAllMovies( serverInfo->keyName() );
    return true;
}

bool CSyncSystem::loadAllCollections( std::shared_ptr<const CServerInfo> serverInfo )
{
    for ( auto && userInfo : *fUsersModel )
    {
        if ( userInfo->isAdmin( serverInfo->keyName() ) )
        {
            return loadAllCollections( userInfo, serverInfo );
        }
    }
    return false;
}

bool CSyncSystem::loadAllCollections( std::shared_ptr< CUserData > userData, std::shared_ptr<const CServerInfo> serverInfo )
{
    if ( !serverInfo || !serverInfo->isEnabled() )
        return false;

    if ( !userData->isAdmin( serverInfo->keyName() ) )
        return false;

    if ( !setCurrentUser( ETool::eMissingCollections, userData, false ) )
        return false;

    emit sigAddToLog( EMsgType::eInfo, QString( "Loading All Collections on server '%1' using admin user '%2'" ).arg( serverInfo->displayName() ).arg( userData->userName( serverInfo->keyName() ) ) );
    requestAllCollections( serverInfo->keyName() );
    return true;
}

bool CSyncSystem::createCollection( std::shared_ptr<const CServerInfo> serverInfo, const QString & collectionName, const std::list< std::shared_ptr< CMediaData > > & items )
{
    for ( auto && userInfo : *fUsersModel )
    {
        if ( userInfo->isAdmin( serverInfo->keyName() ) )
        {
            return createCollection( userInfo, serverInfo, collectionName, items );
        }
    }
    return false;
}

bool CSyncSystem::createCollection( std::shared_ptr< CUserData > userData, std::shared_ptr<const CServerInfo> serverInfo, const QString & collectionName, const std::list< std::shared_ptr< CMediaData > > & items )
{
    if ( !serverInfo || !serverInfo->isEnabled() )
        return false;

    if ( !userData->isAdmin( serverInfo->keyName() ) )
        return false;

    if ( !setCurrentUser( ETool::eMissingCollections, userData, false ) )
        return false;

    emit sigAddToLog( EMsgType::eInfo, QString( "Creating Collection '%3' on server '%1' using admin user '%2'" ).arg( serverInfo->displayName() ).arg( userData->userName( serverInfo->keyName() ) ).arg( collectionName ) );
    requestCreateCollection( serverInfo->keyName(), collectionName, items );
    return true;
}

void CSyncSystem::slotProcessMedia()
{
    selectiveProcessMedia( {} );
}

void CSyncSystem::selectiveProcessMedia( const QString & selectedServer )
{
    auto title = QString( "Processing media for user '%1'" ).arg( currUser().second->userName( selectedServer ) );
    if ( !selectedServer.isEmpty() )
        title += QString( " From '%1'" ).arg( selectedServer );

    emit sigAddToLog( EMsgType::eInfo, title );

    fProgressSystem->setTitle( title );

    auto allMedia = fMediaModel->getAllMedia();
    int cnt = 0;
    for ( auto && ii : allMedia )
    {
        if ( !ii || !ii->isValidForAllServers() )
            continue;
        if ( ii->validUserDataEqual() )
            continue;

        cnt++;
    }

    if ( cnt == 0 )
    {
        fProgressSystem->resetProgress();
        emit sigProcessingFinished( currUser().second->userName( selectedServer ) );
        return;
    }

    fProgressSystem->setMaximum( static_cast<int>( cnt ) );

    for ( auto && ii : allMedia )
    {
        if ( !ii || !ii->isValidForAllServers() )
            continue;
        if ( ii->validUserDataEqual() )
            continue;

        fProgressSystem->incProgress();
        bool dataProcessed = processMedia( ii, selectedServer );
        (void)dataProcessed;
    }
}

bool CSyncSystem::processMedia( std::shared_ptr< CMediaData > mediaData, const QString & selectedServer )
{
    if ( !mediaData || mediaData->validUserDataEqual() )
        return false;

    if ( !currUser().second )
        return false;

    /*
        "UserData": {
        "IsFavorite": false,
        "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
        "PlayCount": 1,
        "PlaybackPositionTicks": 0,
        "Played": true
        }
    */

    //qDebug() << "processing " << mediaData->name();
    for ( auto && serverInfo : *fServerModel )
    {
        auto serverName = serverInfo->keyName();
        bool needsUpdating = mediaData->needsUpdating( serverName );
        if ( !selectedServer.isEmpty() )
        {
            needsUpdating = ( serverName != selectedServer );
        }
        if ( !needsUpdating )
            continue;

        auto newData = selectedServer.isEmpty() ? mediaData->newestMediaData() : mediaData->userMediaData( selectedServer );
        updateUserDataForMedia( serverName, mediaData, newData );
    }
    return true;
}

void CSyncSystem::updateUserDataForMedia( const QString & serverName, std::shared_ptr<CMediaData> mediaData, std::shared_ptr<SMediaServerData> newData )
{
    requestUpdateUserDataForMedia( serverName, mediaData, newData );

    // TODO: Emby currently doesnt support updating favorite status from the "Users/<>/Items/<>/UserData" API
    //       When it does, remove the requestSetFavorite 
    requestSetFavorite( serverName, mediaData, newData );
}

void CSyncSystem::requestUpdateUserDataForMedia( const QString & serverName, std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaServerData > newData )
{
    if ( !mediaData || !mediaData->userMediaData( serverName ) || !newData )
        return;

    if ( *mediaData->userMediaData( serverName ) == *newData )
        return;

    auto && mediaID = mediaData->getMediaID( serverName );
    auto && userID = currUser().second->getUserID( serverName );
    if ( userID.isEmpty() || mediaID.isEmpty() )
        return;

    // PlaystateService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Items/%2/UserData" ).arg( userID ).arg( mediaID ), {} );
    if ( !url.isValid() )
        return;

    auto obj = newData->toJson();
    QByteArray data = QJsonDocument( obj ).toJson();

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;

    //qDebug() << url;

    auto request = QNetworkRequest( url );
    auto reply = makeRequest( request, ENetworkRequestType::ePost, data );

    setServerName( reply, serverName );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateUserMediaData );
}

void CSyncSystem::handleUpdateUserDataForMedia( const QString & serverName, const QString & mediaID )
{
    requestReloadMediaItemData( serverName, mediaID );
    auto mediaData = fMediaModel->getMediaDataForID( serverName, mediaID );
    if ( mediaData )
        emit sigAddToLog( EMsgType::eInfo, tr( "Updated '%1(%2)' on Server '%3' successfully" ).arg( mediaData->name() ).arg( mediaID ).arg( serverName ) );
}

void CSyncSystem::requestSetFavorite( const QString & serverName, std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaServerData > newData )
{
    if ( mediaData->isFavorite( serverName ) == newData->fIsFavorite )
        return;

    auto && mediaID = mediaData->getMediaID( serverName );
    auto && userID = currUser().second->getUserID( serverName );
    if ( userID.isEmpty() || mediaID.isEmpty() )
        return;

    // UserLibraryService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/FavoriteItems/%3" ).arg( userID ).arg( mediaID ), {} );
    if ( !url.isValid() )
        return;

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;
    //qDebug() << "updateType" << updateType;

    auto request = QNetworkRequest( url );


    QNetworkReply * reply = nullptr;
    if ( newData->fIsFavorite )
        reply = makeRequest( request, ENetworkRequestType::ePost );
    else
        reply = makeRequest( request, ENetworkRequestType::eDeleteResource );

    setServerName( reply, serverName );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateFavorite );
}

void CSyncSystem::handleSetFavorite( const QString & serverName, const QString & mediaID )
{
    requestReloadMediaItemData( serverName, mediaID );
    emit sigAddToLog( EMsgType::eInfo, tr( "Updated Favorite status for '%1' on Server '%2' successfully" ).arg( mediaID ).arg( serverName ) );
}

void CSyncSystem::slotProcessUsers()
{
    selectiveProcessUsers( {} );
}

void CSyncSystem::selectiveProcessUsers( const QString & selectedServer )
{
    auto title = QString( "Processing user data" );
    if ( !selectedServer.isEmpty() )
        title += QString( " From '%1'" ).arg( selectedServer );

    emit sigAddToLog( EMsgType::eInfo, title );

    fProgressSystem->setTitle( title );

    auto allUsers = fUsersModel->getAllUsers( false );
    int cnt = 0;
    for ( auto && ii : allUsers )
    {
        if ( !ii || ii->validUserDataEqual() )
            continue;
        cnt++;
    }

    if ( cnt == 0 )
    {
        fProgressSystem->resetProgress();
        emit sigProcessingFinished( currUser().second->userName( selectedServer ) );
        return;
    }

    fProgressSystem->setMaximum( static_cast<int>( cnt ) );

    for ( auto && ii : allUsers )
    {
        if ( !ii || ii->validUserDataEqual() )
            continue;

        fProgressSystem->incProgress();
        bool dataProcessed = processUser( ii, selectedServer );
        (void)dataProcessed;
    }
}

void CSyncSystem::findMovieOnServer( const QString & /*movieName*/, int /*year*/ )
{

}

bool CSyncSystem::processUser( std::shared_ptr< CUserData > userData, const QString & selectedServer )
{
    if ( !userData || userData->validUserDataEqual() )
        return false;

    if ( !currUser().second )
        return false;

    //qDebug() << "processing " << mediaData->name();
    for ( auto && serverInfo : *fServerModel )
    {
        auto serverName = serverInfo->keyName();
        bool needsUpdating = userData->needsUpdating( serverName );
        if ( !selectedServer.isEmpty() )
        {
            needsUpdating = ( serverName != selectedServer );
        }
        if ( !needsUpdating )
            continue;

        auto newData = selectedServer.isEmpty() ? userData->newestUserInfo() : userData->userInfo( selectedServer );
        updateUserData( serverName, userData, newData );
    }
    return true;
}

void CSyncSystem::updateUserData( const QString & serverName, std::shared_ptr<CUserData> userData, std::shared_ptr<SUserServerData> newData )
{
    requestUpdateUserData( serverName, userData, newData );
}

void CSyncSystem::requestUpdateUserData( const QString & serverName, std::shared_ptr<CUserData> userData, std::shared_ptr<SUserServerData> newData )
{
    if ( !userData || !userData->userInfo( serverName ) || !newData )
        return;

    if ( *userData->userInfo( serverName ) == *newData )
        return;

    auto && userID = userData->getUserID( serverName );
    if ( userID.isEmpty() )
        return;

    emit sigAddToLog( EMsgType::eInfo, tr( "Setting user information on server '%1'" ).arg( serverName ) );
    // UserService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1" ).arg( userID ), {} );
    if ( !url.isValid() )
        return;

    auto obj = newData->userDataJSON();
    QByteArray data = QJsonDocument( obj ).toJson();

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;

    //qDebug() << url;

    auto request = QNetworkRequest( url );
    auto reply = makeRequest( request, ENetworkRequestType::ePost, data );

    setServerName( reply, serverName );
    setExtraData( reply, userID );
    setRequestType( reply, ERequestType::eUpdateUserData );
}

void CSyncSystem::handleUpdateUserData( const QString & serverName, const QString & userID )
{
    requestGetUser( serverName, userID );
}

void CSyncSystem::setServerName( QNetworkReply * reply, const QString & serverName )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kServerName ] = serverName;
}

QString CSyncSystem::serverName( QNetworkReply * reply )
{
    if ( !reply )
        return false;
    return fAttributes[ reply ][ kServerName ].toString();
}

void CSyncSystem::setRequestType( QNetworkReply * reply, ERequestType requestType )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kRequestType ] = static_cast<int>( requestType );
    fRequests[ requestType ][ hostName( reply ) ]++;
}

QString CSyncSystem::hostName( QNetworkReply * reply )
{
    if ( !reply )
        return {};

    auto url = reply->url();
    auto retVal = url.toString( QUrl::RemovePath | QUrl::RemoveQuery );
    return retVal;
}

ERequestType CSyncSystem::requestType( QNetworkReply * reply )
{
    return static_cast<ERequestType>( fAttributes[ reply ][ kRequestType ].toInt() );
}

void CSyncSystem::setExtraData( QNetworkReply * reply, QVariant extraData )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kExtraData ] = extraData;
}

QVariant CSyncSystem::extraData( QNetworkReply * reply )
{
    if ( !reply )
        return false;
    return fAttributes[ reply ][ kExtraData ];
}

void CSyncSystem::decRequestCount( QNetworkReply * reply, ERequestType requestType )
{
    auto pos = fRequests.find( requestType );
    if ( pos == fRequests.end() )
        return;

    auto pos2 = ( *pos ).second.find( hostName( reply ) );
    if ( pos2 == ( *pos ).second.end() )
        return;

    if ( ( *pos2 ).second > 0 )
        ( *pos2 ).second--;

    if ( ( *pos2 ).second == 0 )
    {
        ( *pos ).second.erase( pos2 );
    }

    if ( ( *pos ).second.empty() )
        fRequests.erase( pos );
}

void CSyncSystem::postHandleRequest( QNetworkReply * reply, const QString & serverName, ERequestType requestType )
{
    decRequestCount( reply, requestType );
    if ( !isRunning() )
    {
        fProgressSystem->resetProgress();
        if ( ( requestType == ERequestType::eReloadMediaData ) || ( requestType == ERequestType::eUpdateUserMediaData ) )
            emit sigProcessingFinished( currUser().second->userName( serverName ) );
    }
}

void CSyncSystem::slotCheckPendingRequests()
{
    if ( !isRunning() )
    {
        emit sigAddToLog( EMsgType::eInfo, QString( "0 pending requests on all servers" ) );
        fPendingRequestTimer->stop();
        return;
    }
    QStringList msgs;
    int numRequestsTotal = 0;
    for ( auto && ii : fRequests )
    {
        for ( auto && jj : ii.second )
        {
            if ( jj.second )
            {
                numRequestsTotal += jj.second;
                msgs.push_back( QString( "|---> %1 pending '%2' request%3 on server '%4'" ).arg( jj.second ).arg( toString( ii.first ) ).arg( ( jj.second != 1 ) ? "s" : "" ).arg( jj.first ) );
            }
        }
    }
    emit sigAddToLog( EMsgType::eInfo, QString( "There are %1 pending requests across all servers" ).arg( numRequestsTotal ) );
    for ( auto && ii : msgs )
        emit sigAddToLog( EMsgType::eInfo, ii );
}

void CSyncSystem::testServer( const QString & serverName )
{
    auto serverInfo = fServerModel->findServerInfo( serverName );
    if ( !serverInfo )
        return;
    testServer( serverInfo );
}

void CSyncSystem::testServer( std::shared_ptr< const CServerInfo > serverInfo )
{
    if ( !serverInfo )
        return;

    requestTestServer( serverInfo );
}

void CSyncSystem::testServers( const std::vector< std::shared_ptr< const CServerInfo > > & servers )
{
    for ( auto && ii : servers )
        requestTestServer( ii );
}

bool CSyncSystem::isRunning() const
{
    return !fAttributes.empty() && !fRequests.empty();
}

void CSyncSystem::slotMergeMedia( ERequestType requestType )
{
    if ( !fAttributes.empty() )
    {
        QTimer::singleShot( 500,
                            [this, requestType]()
                            {
                                slotMergeMedia( requestType );
                            }
        );
        return;
    }

    if ( !fMediaModel->mergeMedia( fProgressSystem ) )
        clearCurrUser();

    switch ( requestType )
    {
        case ERequestType::eGetMissingEpisodes:
            emit sigMissingEpisodesLoaded();
            emit sigUserMediaLoaded();
            break;
        case ERequestType::eGetMissingTVDBid:
            emit sigMissingTVDBidLoaded();
            emit sigUserMediaLoaded();
            break;
        case ERequestType::eGetAllMovies:
            emit sigAllMoviesLoaded();
            emit sigUserMediaLoaded();
            break;
        default:
            emit sigUserMediaLoaded();
            break;
    }
}

void CSyncSystem::slotAuthenticationRequired( QNetworkReply * /*reply*/, QAuthenticator * /*authenticator*/ )
{
    //qDebug() << "slotAuthenticationRequired:" << reply << reply->url().toString() << authenticator;
}

void CSyncSystem::slotEncrypted( QNetworkReply * /*reply*/ )
{
    //qDebug() << "slotEncrypted:" << reply << reply->url().toString();
}

void CSyncSystem::slotPreSharedKeyAuthenticationRequired( QNetworkReply * /*reply*/, QSslPreSharedKeyAuthenticator * /*authenticator*/ )
{
    //qDebug() << "slotPreSharedKeyAuthenticationRequired: 0x" << Qt::hex << reply << reply->url().toString() << authenticator;
}

void CSyncSystem::slotProxyAuthenticationRequired( const QNetworkProxy & /*proxy*/, QAuthenticator * /*authenticator*/ )
{
    //qDebug() << "slotProxyAuthenticationRequired: 0x" << Qt::hex << &proxy << authenticator;
}

void CSyncSystem::slotSSlErrors( QNetworkReply * /*reply*/, const QList<QSslError> & /*errors*/ )
{
    //qDebug() << "slotSSlErrors: 0x" << Qt::hex << reply << errors;
}

QNetworkReply * CSyncSystem::makeRequest( QNetworkRequest & request, ENetworkRequestType requestType, const QByteArray & data, QString contentType )
{
    if ( !fPendingRequestTimer )
    {
        fPendingRequestTimer = new QTimer( this );
        fPendingRequestTimer->setSingleShot( false );
        fPendingRequestTimer->setInterval( 1500 );
        connect( fPendingRequestTimer, &QTimer::timeout, this, &CSyncSystem::slotCheckPendingRequests );
    }
    fPendingRequestTimer->start();

    request.setAttribute( QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy );

    switch ( requestType )
    {
        case ENetworkRequestType::eDeleteResource:
            return fManager->deleteResource( request );
        case ENetworkRequestType::ePost:
        {
            if ( contentType.isEmpty() )
                contentType = "application/json";
            request.setHeader( QNetworkRequest::ContentTypeHeader, contentType );
            return fManager->post( request, data );
        }
        case ENetworkRequestType::eGet:
            return fManager->get( request );
        default:
            return nullptr;
    }
}

std::shared_ptr<CUserData> CSyncSystem::loadUser( const QString & serverName, const QJsonObject & userData )
{
    return fUsersModel->loadUser( serverName, userData );
}

bool CSyncSystem::isLastRequestOfType( ERequestType type ) const
{
    auto pos = fRequests.find( type );
    if ( pos == fRequests.end() )
        return true;
    int cnt = 0;
    for ( auto && jj : ( *pos ).second )
        cnt += jj.second;
    return cnt <= 1;
}

// functions to handle the responses from the servers
bool CSyncSystem::handleError( QNetworkReply * reply, const QString & serverName, QString & errorMsg, bool reportMsg )
{
    Q_ASSERT( reply );
    if ( !reply )
        return false;

    if ( reply && ( reply->error() != QNetworkReply::NoError ) ) // replys with an error do not get cached
    {
        if ( reply->error() == QNetworkReply::OperationCanceledError )
        {
            emit sigAddToLog( EMsgType::eWarning, QString( "Request canceled on server '%1'" ).arg( serverName ) );
            return false;
        }

        auto data = reply->readAll();
        errorMsg = tr( "Error from Server '%1': %2%3" ).arg( serverName ).arg( reply->errorString() ).arg( data.isEmpty() ? QString() : QString( " - %1" ).arg( QString( data ) ) );
        if ( fUserMsgFunc && reportMsg )
            fUserMsgFunc( EMsgType::eError, tr( "Error response from server" ), errorMsg );
        return false;
    }
    return true;
}

void CSyncSystem::slotRequestFinished( QNetworkReply * reply )
{
    auto serverName = this->serverName( reply );
    auto requestType = this->requestType( reply );
    auto extraData = this->extraData( reply );

    auto pos = fAttributes.find( reply );
    if ( pos != fAttributes.end() )
    {
        fAttributes.erase( pos );
    }

    //emit sigAddToLog( EMsgType::eInfo, QString( "Request Completed: %1" ).arg( reply->url().toString() ) );
    //emit sigAddToLog( EMsgType::eInfo, QString( "Is LHS? %1" ).arg( serverName ? "Yes" : "No" ) );
    //emit sigAddToLog( EMsgType::eInfo, QString( "Request Type: %1" ).arg( toString( requestType ) ) );
    //emit sigAddToLog( EMsgType::eInfo, QString( "Extra Data: %1" ).arg( extraData.toString() ) );

    QString errorMsg;
    if ( !handleError( reply, serverName, errorMsg, requestType != ERequestType::eTestServer ) )
    {
        switch ( requestType )
        {
            case ERequestType::eGetServerInfo:
                break;
            case ERequestType::eGetUsers:
                if ( isLastRequestOfType( ERequestType::eGetUsers ) )
                    emit sigLoadingUsersFinished();
                break;
            case ERequestType::eGetUser:
            case ERequestType::eGetUserAvatar:
            case ERequestType::eSetUserAvatar:
                break;
            case ERequestType::eGetMediaList:
                emit sigUserMediaLoaded();
                break;
            case ERequestType::eGetMissingEpisodes:
                emit sigMissingEpisodesLoaded();
                break;
            case ERequestType::eGetMissingTVDBid:
                emit sigMissingTVDBidLoaded();
                break;
            case ERequestType::eGetAllMovies:
                emit sigAllMoviesLoaded();
                break;
            case ERequestType::eGetAllCollections:
            case ERequestType::eGetAllCollectionsEx:
                break;
            case ERequestType::eGetCollection:
                emit sigAllCollectionsLoaded();
                break;
            case ERequestType::eNone:
            case ERequestType::eReloadMediaData:
            case ERequestType::eUpdateUserMediaData:
            case ERequestType::eUpdateFavorite:
            case ERequestType::eTestServer:
                emit sigTestServerResults( serverName, false, errorMsg );
            case ERequestType::eDeleteConnectedID:
            case ERequestType::eSetConnectedID:
            case ERequestType::eUpdateUserData:
            case ERequestType::eGetServerHomePage:
            case ERequestType::eGetServerIcon:
            case ERequestType::eCreateCollection:
                break;
        }

        postHandleRequest( reply, serverName, requestType );
        return;
    }

    //qDebug() << "Requests Remaining" << fAttributes.size();
    auto data = reply->readAll();
    //qDebug() << data;

    switch ( requestType )
    {
        case ERequestType::eNone:
            break;
        case ERequestType::eGetServerInfo:
            handleGetServerInfoResponse( serverName, data );
            break;
        case ERequestType::eGetServerHomePage:
            handleGetServerHomePageResponse( serverName, data );
            break;
        case ERequestType::eGetServerIcon:
            handleGetServerIconResponse( serverName, data, extraData.toString() );
            break;
        case ERequestType::eGetUsers:
            if ( !fProgressSystem->wasCanceled() )
            {
                handleGetUsersResponse( serverName, data );

                if ( isLastRequestOfType( ERequestType::eGetUsers ) )
                    emit sigLoadingUsersFinished();
            }
            break;
        case ERequestType::eGetUser:
            handleGetUserResponse( serverName, data );
            break;
        case ERequestType::eGetUserAvatar:
            handleGetUserAvatarResponse( serverName, extraData.toString(), data );
            break;
        case ERequestType::eSetUserAvatar:
            handleSetUserAvatarResponse( serverName, extraData.toString() );
            break;
        case ERequestType::eGetMediaList:
            if ( !fProgressSystem->wasCanceled() )
            {
                handleGetMediaListResponse( serverName, data );

                if ( isLastRequestOfType( ERequestType::eGetMediaList ) )
                {
                    fProgressSystem->resetProgress();
                    slotMergeMedia( requestType );
                }
            }
            break;
        case ERequestType::eGetMissingEpisodes:
        {
            if ( !fProgressSystem->wasCanceled() )
            {
                handleMissingEpisodesResponse( serverName, data );

                if ( isLastRequestOfType( ERequestType::eGetMissingEpisodes ) )
                {
                    fProgressSystem->resetProgress();
                    slotMergeMedia( requestType );
                }
            }
            break;
        }
        case ERequestType::eGetMissingTVDBid:
        {
            if ( !fProgressSystem->wasCanceled() )
            {
                handleMissingTVDBidResponse( serverName, data );

                if ( isLastRequestOfType( ERequestType::eGetMissingTVDBid ) )
                {
                    fProgressSystem->resetProgress();
                    slotMergeMedia( requestType );
                }
            }
            break;
        }
        case ERequestType::eGetAllMovies:
        {
            if ( !fProgressSystem->wasCanceled() )
            {
                handleAllMoviesResponse( serverName, data );
                if ( isLastRequestOfType( ERequestType::eGetAllMovies ) )
                {
                    fProgressSystem->resetProgress();
                    slotMergeMedia( requestType );
                }
            }
            break;
        }
        case ERequestType::eGetAllCollections:
        {
            if ( !fProgressSystem->wasCanceled() )
            {
                handleAllCollectionsResponse( serverName, data );
            }
            break;
        }
        case ERequestType::eGetAllCollectionsEx:
        {
            if ( !fProgressSystem->wasCanceled() )
            {
                handleAllCollectionsExResponse( serverName, data );
                if ( isLastRequestOfType( ERequestType::eGetAllCollectionsEx ) )
                {
                    fProgressSystem->resetProgress();
                }
            }
            break;
        }
        case ERequestType::eGetCollection:
        {
            if ( !fProgressSystem->wasCanceled() )
            {
                auto extraStrings = extraData.toStringList();
                handleGetCollectionResponse( serverName, extraStrings.front(), extraStrings.back(), data );
                if ( isLastRequestOfType( ERequestType::eGetCollection ) )
                {
                    emit sigAllCollectionsLoaded();
                    fProgressSystem->resetProgress();
                }
            }
            break;
        }
        case ERequestType::eReloadMediaData:
        {
            handleReloadMediaResponse( serverName, data, extraData.toString() );
            break;
        }
        case ERequestType::eUpdateUserMediaData:
        {
            handleUpdateUserDataForMedia( serverName, extraData.toString() );
            break;
        }
        case ERequestType::eUpdateUserData:
        {
            //qDebug() << data;
            handleUpdateUserData( serverName, extraData.toString() );
            break;
        }
        case ERequestType::eUpdateFavorite:
        {
            handleSetFavorite( serverName, extraData.toString() );
            break;
        }
        case ERequestType::eTestServer:
        {
            handleTestServer( serverName );
            break;
        }
        case ERequestType::eDeleteConnectedID:
        {
            handleDeleteConnectedID( serverName );
            break;
        }
        case ERequestType::eSetConnectedID:
        {
            handleSetConnectedID( serverName );
            break;
        }
        case ERequestType::eCreateCollection:
        {
            handleCreateCollection( serverName, data );
            break;
        }

    }
    postHandleRequest( reply, serverName, requestType );
}

void CSyncSystem::requestTestServer( std::shared_ptr< const CServerInfo > serverInfo )
{
    fTestServers[ serverInfo->keyName() ] = serverInfo;

    // UserService
    auto && url = serverInfo->getUrl( "Users/Query", {} );
    emit sigAddToLog( EMsgType::eInfo, tr( "Testing Server '%1' - %2" ).arg( serverInfo->displayName() ).arg( url.toString() ) );

    if ( !url.isValid() )
    {
        emit sigTestServerResults( serverInfo->keyName(), false, tr( "Invalid URL - '%1'" ).arg( url.toString() ) );
        return;
    }

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setServerName( reply, serverInfo->keyName() );
    setRequestType( reply, ERequestType::eTestServer );
}

void CSyncSystem::handleTestServer( const QString & serverName )
{
    auto pos = fTestServers.find( serverName );
    Q_ASSERT( pos != fTestServers.end() );
    if ( pos != fTestServers.end() )
    {
        auto testServer = ( *pos ).second;
        fTestServers.erase( pos );

        emit sigAddToLog( EMsgType::eInfo, tr( "Finished Testing server '%1' successfully" ).arg( testServer->displayName() ) );
        emit sigTestServerResults( serverName, true, QString() );
    }
    else
        emit sigTestServerResults( serverName, false, tr( "Internal error" ) );
}

void CSyncSystem::requestGetServerInfo( const QString & serverName )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading server information from server '%1'" ).arg( serverName ) );

    // SystemService 
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( "/System/Info/Public", {} );
    if ( !url.isValid() )
        return;

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetServerInfo );
}

void CSyncSystem::handleGetServerInfoResponse( const QString & serverName, const QByteArray & data )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug() << doc.toJson();
    auto serverInfo = doc.object();

    fServerModel->updateServerInfo( serverName, serverInfo );
}

void CSyncSystem::requestGetServerHomePage( const QString & serverName )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading server homepage from server '%1'" ).arg( serverName ) );

    // SystemService 
    auto && url = QUrl( fServerModel->findServerInfo( serverName )->url( true ) );
    if ( !url.isValid() )
        return;

    emit sigAddToLog( EMsgType::eInfo, tr( "Server URL: %1" ).arg( url.toString() ) );

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetServerHomePage );
}

void CSyncSystem::handleGetServerHomePageResponse( const QString & serverName, const QByteArray & data )
{
    static std::list< QRegularExpression > regExs =
    {
          QRegularExpression( "\\<link\\s+rel=\"(?<rel>(shortcut |mask-)icon)\" href=\"(?<url>.*)\"\\>" )
        , QRegularExpression( "\\<link\\s+rel=\"(?<rel>(shortcut |mask-)icon)\" type=\"(?<type>.*)\" href=\"(?<url>.*)\"\\>" )
        , QRegularExpression( "\\<link\\s+rel=\"(?<rel>(shortcut |mask-)icon)\" href=\"(?<url>.*)\" color=\"(?<color>.*)\"\\>" )
    };

    for ( auto && ii : regExs )
    {
        auto match = ii.match( data );
        if ( match.hasMatch() )
        {
            auto url = match.captured( "url" );
            auto type = match.captured( "type" );
            auto color = match.captured( "color" );
            auto rel = match.captured( "rel" );
            requestGetServerIcon( serverName, url, type );
            return;
        }
    }
}


void CSyncSystem::requestGetServerIcon( const QString & serverName, const QString & iconRelPath, const QString & type )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading server icon from server '%1'" ).arg( serverName ) );

    // SystemService 
    auto && url = QUrl( fServerModel->findServerInfo( serverName )->url( true ) + "/" + iconRelPath );
    if ( !url.isValid() )
        return;

    emit sigAddToLog( EMsgType::eInfo, tr( "Server URL: %1" ).arg( url.toString() ) );

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setExtraData( reply, type );
    setRequestType( reply, ERequestType::eGetServerIcon );
}

void CSyncSystem::handleGetServerIconResponse( const QString & serverName, const QByteArray & data, const QString & type )
{
    fServerModel->setServerIcon( serverName, data, type );
}

void CSyncSystem::requestGetUsers( const QString & serverName )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading users from server '%1'" ).arg( serverName ) );

    // UserService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( "Users/Query", {} );
    if ( !url.isValid() )
        return;

    emit sigAddToLog( EMsgType::eInfo, tr( "Server URL: %1" ).arg( url.toString() ) );

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetUsers );
}

void CSyncSystem::handleGetUsersResponse( const QString & serverName, const QByteArray & data )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug() << doc.toJson();
    auto users = doc.object()[ "Items" ].toArray();
    fProgressSystem->pushState();

    emit sigAddToLog( EMsgType::eInfo, QString( "Server '%1' has %2 Users" ).arg( serverName ).arg( users.count() ) );

    for ( auto && ii : users )
    {
        if ( fProgressSystem->wasCanceled() )
            break;
        fProgressSystem->incProgress();

        auto user = ii.toObject();
        //qDebug() << QJsonDocument( user ).toJson();
        loadUser( serverName, user );
    }

    fProgressSystem->popState();
    fProgressSystem->incProgress();
}

void CSyncSystem::requestGetUser( const QString & serverName, const QString & userID )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading users from server '%1'" ).arg( serverName ) );

    // UserService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1" ).arg( userID ), {} );
    if ( !url.isValid() )
        return;

    emit sigAddToLog( EMsgType::eInfo, tr( "Server URL: %1" ).arg( url.toString() ) );

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetUser );
}

void CSyncSystem::handleGetUserResponse( const QString & serverName, const QByteArray & data )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug() << doc.toJson();

    emit sigAddToLog( EMsgType::eInfo, QString( "Reloading user on server %1" ).arg( serverName ) );

    auto user = doc.object();
    auto userData = loadUser( serverName, user );
    if ( userData->hasAvatarInfo( serverName ) )
    {
        requestGetUserAvatar( serverName, userData->getUserID( serverName ) );
    }
}

void CSyncSystem::requestGetUserAvatar( const QString & serverName, const QString & userID )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading user image from server '%1'" ).arg( serverName ) );

    // UserService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "/Users/%1/Images/Primary" ).arg( userID ), {} );
    if ( !url.isValid() )
        return;
    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetUserAvatar );
    setExtraData( reply, userID );
}

void CSyncSystem::handleGetUserAvatarResponse( const QString & serverName, const QString & userID, const QByteArray & data )
{
    auto user = fUsersModel->userDataOnServer( serverName, userID );
    if ( !user )
        return;

    emit sigAddToLog( EMsgType::eInfo, tr( "Setting user image from server '%1' for '%2'" ).arg( serverName ).arg( user->name( serverName ) ) );
    fUsersModel->setUserAvatar( serverName, userID, data );
}

void CSyncSystem::requestSetUserAvatar( const QString & serverName, const QString & userID, const QImage & image )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Setting user image on server '%1'" ).arg( serverName ) );

    // UserService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "/Users/%1/Images/Primary" ).arg( userID ), {} );
    if ( !url.isValid() )
        return;
    auto request = QNetworkRequest( url );

    QByteArray data;
    QBuffer buffer( &data );
    buffer.open( QIODevice::WriteOnly );
    image.save( &buffer, "PNG" ); // writes image into ba in PNG format

    auto reply = makeRequest( request, ENetworkRequestType::ePost, data.toBase64(), "image/png" );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eSetUserAvatar );
    setExtraData( reply, userID );
}

void CSyncSystem::handleSetUserAvatarResponse( const QString & serverName, const QString & userID )
{
    requestGetUserAvatar( serverName, userID );
}

static QString kForceDelete = "<FORCE DELETE>";

void CSyncSystem::repairConnectIDs( const std::list< std::shared_ptr< CUserData > > & users )
{
    auto needsTimer = fUsersNeedingConnectIDUpdates.empty();

    for ( auto && ii : users )
    {
        auto connectedID = ii->connectedID();
        if ( !NSABUtils::NStringUtils::isValidEmailAddress( connectedID ) )
            connectedID = kForceDelete;
        auto connectedIDType = ii->connectedIDType();

        fUsersNeedingConnectIDUpdates.push_back( { {}, { connectedIDType, connectedID }, ii } );
    }

    if ( needsTimer )
        QTimer::singleShot( 0, [this]() { slotRepairNextUser(); } );
}

void CSyncSystem::setConnectedID( const QString & serverName, const QString & connectedID, std::shared_ptr< CUserData > & user )
{
    return setConnectedID( serverName, "LinkedUser", connectedID, user );
}

void CSyncSystem::setConnectedID( const QString & serverName, const QString & idType, const QString & connectedID, std::shared_ptr< CUserData > & user )
{
    auto needsTimer = fUsersNeedingConnectIDUpdates.empty();

    fUsersNeedingConnectIDUpdates.push_back( { serverName, { idType, connectedID }, user } );

    if ( needsTimer )
        QTimer::singleShot( 0, [this]() { slotRepairNextUser(); } );
}

void CSyncSystem::slotRepairNextUser()
{
    if ( fUsersNeedingConnectIDUpdates.empty() )
        return;

    fCurrUserConnectID = fUsersNeedingConnectIDUpdates.front();
    fUsersNeedingConnectIDUpdates.pop_front();

    if ( fCurrUserConnectID.fServerName.isEmpty() )
    {
        // do this for all servers
        for ( auto && serverInfo : *fServerModel )
        {
            if ( !serverInfo->isEnabled() )
                continue;;

            updateConnectID( serverInfo->keyName() );
        }
    }
    else
    {
        updateConnectID( fCurrUserConnectID.fServerName );
    }
}

void CSyncSystem::updateConnectID( const QString & serverName )
{
    if ( !fCurrUserConnectID.fUserData->onServer( serverName ) )
        return;

    auto oldID = fCurrUserConnectID.fUserData->connectedID( serverName );
    auto oldIDType = fCurrUserConnectID.fUserData->connectedIDType( serverName );

    auto newID = fCurrUserConnectID.fConnectID.second;
    auto newIDType = fCurrUserConnectID.fConnectID.first;

    if ( ( oldID != newID ) && !newID.isEmpty() && ( newID != kForceDelete ) )
        requestSetConnectedID( serverName );
    else if ( newID.isEmpty() || ( newID == kForceDelete ) )
        requestDeleteConnectedID( serverName );
}

void CSyncSystem::requestDeleteConnectedID( const QString & serverName )
{
    if ( !fCurrUserConnectID.fUserData )
        return;

    // ConnectService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Connect/Link" ).arg( fCurrUserConnectID.fUserData->getUserID( serverName ) ), {} );
    if ( !url.isValid() )
        return;

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Deleting ConnectID for User '%1' from server '%2'" ).arg( fCurrUserConnectID.fUserData->userName( serverName ) ).arg( serverName ) );

    auto reply = makeRequest( request, ENetworkRequestType::eDeleteResource );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eDeleteConnectedID );
}

void CSyncSystem::handleDeleteConnectedID( const QString & serverName )
{
    if ( !fCurrUserConnectID.fUserData )
        return;
    if ( !fCurrUserConnectID.fConnectID.second.isEmpty() )
        requestSetConnectedID( serverName );
    else
    {
        requestGetUser( serverName, fCurrUserConnectID.fUserData->getUserID( serverName ) );
        return fUsersModel->updateUserConnectID( serverName, fCurrUserConnectID.fUserData->getUserID( serverName ), fCurrUserConnectID.fUserData->connectedIDType( serverName ), fCurrUserConnectID.fUserData->connectedID( serverName ) );
    }

}

void CSyncSystem::requestSetConnectedID( const QString & serverName )
{
    if ( !fCurrUserConnectID.fUserData )
        return;

    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "ConnectUsername", fCurrUserConnectID.fConnectID.second )
    };

    // ConnectService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Connect/Link" ).arg( fCurrUserConnectID.fUserData->getUserID( serverName ) ), queryItems );
    if ( !url.isValid() )
        return;


    //qDebug() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Setting ConnectID for User '%1' from server '%2' to '%3'" ).arg( fCurrUserConnectID.fUserData->userName( serverName ) ).arg( serverName ).arg( fCurrUserConnectID.fConnectID.second ) );

    auto reply = makeRequest( request, ENetworkRequestType::ePost );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eSetConnectedID );
}

void CSyncSystem::handleSetConnectedID( const QString & serverName )
{
    if ( !fCurrUserConnectID.fUserData )
        return;

    requestGetUser( serverName, fCurrUserConnectID.fUserData->getUserID( serverName ) );
    return fUsersModel->updateUserConnectID( serverName, fCurrUserConnectID.fUserData->getUserID( serverName ), fCurrUserConnectID.fUserData->connectedIDType(), fCurrUserConnectID.fUserData->connectedID() );
}

void CSyncSystem::requestGetMediaList( const QString & serverName )
{
    if ( !currUser().second )
        return;

    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "IncludeItemTypes", fSettings->getSyncItemTypes() ),
        std::make_pair( "SortBy", "Type,ProductionYear,PremiereDate,SortName" ),
        std::make_pair( "SortOrder", "Ascending" ),
        std::make_pair( "Recursive", "True" ),
        std::make_pair( "IsMissing", "False" ),
        std::make_pair("Fields", "Path,ProviderIds,ExternalUrls,Missing,ProductionYear,PremiereDate,DateCreated,EndDate,StartDate,OriginalTitle")
    };

    // ItemsService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Items" ).arg( currUser().second->getUserID( serverName ) ), queryItems );
    if ( !url.isValid() )
        return;

    //qDebug().noquote().nospace() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting media for '%1' from server '%2'" ).arg( currUser().second->userName( serverName ) ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetMediaList );
}

std::list< std::shared_ptr< CMediaData > > CSyncSystem::handleGetMediaListResponse( const QString & serverName, const QByteArray & data, const QString & progressTitle, const QString & logMsg, const QString & partialLogMsg )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return {};
    }

    //qDebug() << doc.toJson();
    if ( !doc[ "Items" ].isArray() )
    {
        if ( CMediaData::isExtra( doc.object() ) )
            return {};
        fMediaModel->loadMedia( serverName, doc.object() );
        return {};
    }

    auto mediaList = doc[ "Items" ].toArray();
    auto showProgress = mediaList.count() > 10;
    if ( showProgress )
    {
        fProgressSystem->pushState();

        fProgressSystem->resetProgress();
        fProgressSystem->setTitle( progressTitle );
        fProgressSystem->setMaximum( mediaList.count() );
    }
    emit sigAddToLog( EMsgType::eInfo, logMsg.arg( serverName ).arg( mediaList.count() ) );
    if ( fSettings->maxItems() > 0 )
        emit sigAddToLog( EMsgType::eInfo, partialLogMsg.arg( fSettings->maxItems() ) );

    int curr = 0;
    std::list< std::shared_ptr< CMediaData > > retVal;
    //fMediaModel->beginBatchLoad();
    for ( auto && ii : mediaList )
    {
        auto media = ii.toObject();
        if ( CMediaData::isExtra( media ))
            continue;
        if ( fSettings->maxItems() > 0 )
        {
            if ( retVal.size() >= fSettings->maxItems() )
                break;
        }
        curr++;

        if ( showProgress )
        {
            if ( fProgressSystem->wasCanceled() )
                break;
            fProgressSystem->incProgress();
        }

        auto curr = fMediaModel->loadMedia( serverName, media );
        retVal.push_back( curr );
    }
    //fMediaModel->endBatchLoad();
    if ( showProgress )
    {
        fProgressSystem->resetProgress();
        fProgressSystem->popState();
        fProgressSystem->incProgress();
    }
    return retVal;
}

void CSyncSystem::handleGetMediaListResponse( const QString & serverName, const QByteArray & data )
{
    handleGetMediaListResponse( serverName, data, tr( "Loading Users Media Data" ), tr( "%1 has %2 media items on server '%3'" ), tr( "Loading %2 media items" ) );
}

void CSyncSystem::requestMissingEpisodes( const QString & serverName, const QDate & minPremiereDate, const QDate & maxPremiereDate )
{
    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "IncludeItemTypes", "Episode" ),
        std::make_pair( "SortBy", "Type,ProductionYear,PremiereDate,SortName" ),
        std::make_pair( "SortOrder", "Ascending" ),
        std::make_pair( "Recursive", "True" ),
        std::make_pair( "IsMissing", "True" ),
        std::make_pair("Fields", "Path,ProviderIds,ExternalUrls,Missing,ProductionYear,PremiereDate,DateCreated,EndDate,StartDate,OriginalTitle")
    };
    if ( minPremiereDate.isValid() )
    {
        queryItems.emplace_back( std::make_pair( "MinPremiereDate", minPremiereDate.toString( Qt::ISODate ) ) );
    }
    if ( maxPremiereDate.isValid() )
    {
        queryItems.emplace_back( std::make_pair( "MaxPremiereDate", maxPremiereDate.toString( Qt::ISODate ) ) );
    }

    // ItemsService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Items" ).arg( currUser().second->getUserID( serverName ) ), queryItems );
    if ( !url.isValid() )
        return;

    //qDebug().noquote().nospace() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting missing episodes from server '%2'" ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetMissingEpisodes );
}

void CSyncSystem::requestMissingTVDBid( const QString & serverName )
{
    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "IncludeItemTypes", "Episode" ),
        std::make_pair( "SortBy", "Type,ProductionYear,PremiereDate,SortName" ),
        std::make_pair( "SortOrder", "Ascending" ),
        std::make_pair( "Recursive", "True" ),
        //std::make_pair( "IsMissing", "True" ),
        std::make_pair( "HasTvdbId", "False" ),
        std::make_pair( "HasSpecialFeature", "False"),
        std::make_pair( "Fields", "Path,ProviderIds,ExternalUrls,Missing,ProductionYear,PremiereDate,DateCreated,EndDate,StartDate,OriginalTitle" )
    };

    // ItemsService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Items" ).arg( currUser().second->getUserID( serverName ) ), queryItems );
    if ( !url.isValid() )
        return;

    //qDebug().noquote().nospace() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting missing episodes from server '%2'" ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetMissingTVDBid );
}

void CSyncSystem::handleMissingTVDBidResponse( const QString & serverName, const QByteArray & data )
{
    handleGetMediaListResponse( serverName, data, tr( "Loading Users Missing TVDBid Media Data" ), tr( "Server '%1' has %2 missing TVDBid episodes" ), tr( "Loading %2 missing TVDBid episodes" ) );
}

void CSyncSystem::handleMissingEpisodesResponse( const QString & serverName, const QByteArray & data )
{
    handleGetMediaListResponse( serverName, data, tr( "Loading Users Missing Media Data" ), tr( "Server '%1' has %2 missing episodes" ), tr( "Loading %2 missing episodes" ) );
}

void CSyncSystem::handleAllMoviesResponse( const QString & serverName, const QByteArray & data )
{
    handleGetMediaListResponse( serverName, data, tr( "Loading All Movies" ), tr( "Server '%1' has %2 movies" ), tr( "Loading %2 movies" ) );
}

void CSyncSystem::requestAllMovies( const QString & serverName )
{
    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "IncludeItemTypes", "Movie" ),
        std::make_pair( "SortBy", "Type,ProductionYear,PremiereDate,SortName" ),
        std::make_pair( "SortOrder", "Ascending" ),
        std::make_pair( "Recursive", "True" ),
        std::make_pair("Fields", "Path,ProviderIds,ExternalUrls,Missing,ProductionYear,PremiereDate,DateCreated,EndDate,StartDate,OriginalTitle")
    };

    // ItemsService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Items" ).arg( currUser().second->getUserID( serverName ) ), queryItems );
    if ( !url.isValid() )
        return;

    //qDebug().noquote().nospace() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting all movies from server '%2'" ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetAllMovies );
}

bool CSyncSystem::requestCreateCollection( const QString & serverName, const QString & collectionName, const std::list< std::shared_ptr< CMediaData > > & items )
{
    if ( collectionName.isEmpty() )
        return false;

    QStringList ids;
    for ( auto && ii : items )
    {
        auto id = ii->getMediaID( serverName );
        if ( id.isEmpty() )
            continue;

        ids << id;
    }
    if ( ids.empty() )
        return false;
    
    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "IsLocked", "false" ),
        std::make_pair( "Name", collectionName ),
        std::make_pair( "Ids", ids.join( "," ) )
    };

    // collection service
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "emby/Collections" ), queryItems );
    if ( !url.isValid() )
        return false;
    // http://127.0.0.1:8096/emby/Collections?IsLocked=false&Name=AFI%20top%2010%20Animation&Ids=38871%2C38869%2C38867%2C38872%2C38868%2C38873%2C39091%2C39092%2C39007%2C39031&api_key=ead9662360084257ab76f01dabea92a5
    // http://localhost:8096/emby/Collections?IsLocked=false&Name=AFI%20top%2010%20Animation&Ids=38871%2C38869%2C38867%2C38872%2C38868%2C38873%2C39091%2C39092%2C39007%2C39031&api_key=596f3a7d4e974692847d8885c278a11a



    //qDebug().noquote().nospace() << url.toEncoded();
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting to create media collection '%1' with '%3' media items on server '%2'" ).arg( collectionName ).arg( serverName ).arg( ids.count() ) );

    auto reply = makeRequest( request, ENetworkRequestType::ePost );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eCreateCollection );
    return true;
}

void CSyncSystem::handleCreateCollection( const QString & /*serverName*/, const QByteArray & data )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug().nospace().noquote() << doc.toJson();
}

void CSyncSystem::requestAllCollections( const QString & serverName )
{
    // ItemsService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Library/MediaFolders" ), {} );
    if ( !url.isValid() )
        return;

    //qDebug().noquote().nospace() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting all media folders from server '%2'" ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetAllCollections );
}

void CSyncSystem::handleAllCollectionsResponse( const QString & serverName, const QByteArray & data )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug() << doc.toJson();
    if ( !doc[ "Items" ].isArray() )
    {
        return;
    }

    auto folders = doc[ "Items" ].toArray();
    emit sigAddToLog( EMsgType::eInfo, tr( "There are %1 media folders on server %2" ).arg( folders.count() ).arg( serverName ) );

    for ( auto && ii : folders )
    {
        auto folder = ii.toObject();
        if ( !folder.contains( "CollectionType" ) )
            continue;

        if ( !folder.contains( "Id" ) )
            continue;

        auto type = folder[ "CollectionType" ].toString();
        if ( type.toLower() != "boxsets" )
            continue;

        auto id = folder[ "Id" ].toString();
        if ( id.isEmpty() )
            continue;

        requestAllCollectionsEx( serverName, id );
    }
}

void CSyncSystem::requestAllCollectionsEx( const QString & serverName, const QString & boxSetsId )
{
    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "ParentId", boxSetsId ),
        std::make_pair( "Recursive", "False" )
    };


    // ItemsService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Items" ), queryItems );
    if ( !url.isValid() )
        return;

    //qDebug().noquote().nospace() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting all collections from server '%2'" ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetAllCollectionsEx );
}

void CSyncSystem::handleAllCollectionsExResponse( const QString & serverName, const QByteArray & data )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    //qDebug().noquote().nospace() << doc.toJson();
    if ( !doc[ "Items" ].isArray() )
    {
        return;
    }

    auto collections = doc[ "Items" ].toArray();
    emit sigAddToLog( EMsgType::eInfo, tr( "There are %1 collections on server %2" ).arg( collections.count() ).arg( serverName ) );

    for ( auto && ii : collections )
    {
        auto folder = ii.toObject();
        if ( !folder.contains( "Name" ) )
            continue;

        if ( !folder.contains( "Id" ) )
            continue;

        auto name = folder[ "Name" ].toString();
        auto id = folder[ "Id" ].toString();
        if ( id.isEmpty() )
            continue;

        requestGetCollection( serverName, name, id );
    }
}

void CSyncSystem::requestGetCollection( const QString & serverName, const QString & collectionName, const QString & collectionId )
{
    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "ParentId", collectionId ),
        std::make_pair( "Recursive", "False" ),
        std::make_pair( "IncludeItemTypes", "Movie" ),
        std::make_pair( "SortBy", "Type,ProductionYear,PremiereDate,SortName" ),
        std::make_pair( "SortOrder", "Ascending" ),
        std::make_pair("Fields", "Path,ProviderIds,ExternalUrls,Missing,ProductionYear,PremiereDate,DateCreated,EndDate,StartDate,OriginalTitle")
    };


    // ItemsService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Items" ), queryItems );
    if ( !url.isValid() )
        return;

    //qDebug().noquote().nospace() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting collections %1 from server '%2'" ).arg( collectionName ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetCollection );
    setExtraData( reply, QStringList() << collectionName << collectionId );
}


void CSyncSystem::handleGetCollectionResponse( const QString & serverName, const QString & collectionName, const QString & collectionId, const QByteArray & data )
{
    auto items = handleGetMediaListResponse( serverName, data, tr( "Loading Movies for Collection '%1'" ).arg( collectionName ), tr( "There are %3 movies in collection '%1' on server %2" ).arg( collectionName ), tr( "Loading %2 movies" ) );
    fMediaModel->addCollection( serverName, collectionName, collectionId, items );
}

void CSyncSystem::requestReloadMediaItemData( const QString & serverName, const QString & mediaID )
{
    auto mediaData = fMediaModel->getMediaDataForID( serverName, mediaID );
    if ( !mediaData )
        return;

    requestReloadMediaItemData( serverName, mediaData );
}

void CSyncSystem::requestReloadMediaItemData( const QString & serverName, std::shared_ptr< CMediaData > mediaData )
{
    // UserLibraryService
    auto && url = fServerModel->findServerInfo( serverName )->getUrl( QString( "Users/%1/Items/%2" ).arg( currUser().second->getUserID( serverName ) ).arg( mediaData->getMediaID( serverName ) ), {} );
    if ( !url.isValid() )
        return;

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );

    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eReloadMediaData );
    setExtraData( reply, mediaData->getMediaID( serverName ) );
    //qDebug() << "Media Data for " << mediaData->name() << reply;
}

void CSyncSystem::handleReloadMediaResponse( const QString & serverName, const QByteArray & data, const QString & itemID )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( EMsgType::eError, tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ) );
        return;
    }

    auto mediaData = doc.object();
    fMediaModel->reloadMedia( serverName, mediaData, itemID );
}

void CSyncSystem::slotCanceled()
{
    auto tmp = fAttributes;
    for ( auto && ii : tmp )
    {
        ii.first->abort();
    }
    clearCurrUser();
}

std::pair< ETool, std::shared_ptr< CUserData > > CSyncSystem::currUser() const
{
    return fCurrUserData;
}


void CSyncSystem::clearCurrUser()
{
    fCurrUserData.first = ETool::eNone;
    if ( fCurrUserData.second )
        fCurrUserData.second.reset();
}
