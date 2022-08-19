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
#include "Settings.h"
#include "ServerInfo.h"
#include "MediaData.h"
#include "MediaUserData.h"

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

#include <QUrlQuery>

QString toString( ERequestType request )
{
    switch ( request )
    {
        case ERequestType::eNone: return "None";
        case ERequestType::eGetUsers: return "GetUsers";
        case ERequestType::eGetMediaList: return "GetMediaList";
        case ERequestType::eReloadMediaData: return "ReloadMediaData";
        case ERequestType::eUpdateData: return "UpdateData";
        case ERequestType::eUpdateFavorite: return "UpdateFavorite";
        case ERequestType::eTestServer: return "TestServer";
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

CSyncSystem::CSyncSystem( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QObject( parent ),
    fSettings( settings ),
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

void CSyncSystem::setUserMsgFunc( std::function< void( const QString & title, const QString & msg, bool isCritical ) > userMsgFunc )
{
    fUserMsgFunc = userMsgFunc;
}

void CSyncSystem::setProgressSystem( std::shared_ptr< CProgressSystem > funcs )
{
    fProgressSystem = funcs;
}

void CSyncSystem::setLoadUserFunc( std::function< std::shared_ptr< CUserData >( const QString & serverName, const QJsonObject & user ) > loadUserFunc )
{
    fLoadUserFunc = loadUserFunc;
}

void CSyncSystem::setLoadMediaFunc( std::function< std::shared_ptr< CMediaData >( const QString & serverName, const QJsonObject & media ) > loadMediaFunc )
{
    fLoadMediaFunc = loadMediaFunc;
}

void CSyncSystem::setReloadMediaFunc( std::function< std::shared_ptr< CMediaData >( const QString & serverName, const QJsonObject & media, const QString & itemID ) > reloadMediaFunc )
{
    fReloadMediaFunc = reloadMediaFunc;
}

void CSyncSystem::setGetMediaDataForIDFunc( std::function< std::shared_ptr< CMediaData >( const QString & serverName, const QString & mediaID ) > getMediaDataForIDFunc )
{
    fGetMediaDataForIDFunc = getMediaDataForIDFunc;
}

void CSyncSystem::setMergeMediaFunc( std::function< bool( std::shared_ptr< CProgressSystem > progressSystem ) > mergeMediaFunc )
{
    fMergeMediaFunc = mergeMediaFunc;
}

void CSyncSystem::setGetAllMediaFunc( std::function< std::unordered_set< std::shared_ptr< CMediaData > >() > getAllMediaFunc )
{
    fGetAllMediaFunc = getAllMediaFunc;
}

void CSyncSystem::reset()
{
    fAttributes.clear();
}

void CSyncSystem::loadUsers()
{
    if ( !fSettings->canSync() )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Server Settings are not Setup" ), tr( "Server Settings are not setup.  Please fix and try again" ), true );
        return;
    }

    fProgressSystem->setTitle( tr( "Loading Users" ) );
    int enabledServers = 0;
    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = fSettings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;
        enabledServers++;
    }
    fProgressSystem->setMaximum( enabledServers );

    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = fSettings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;
        requestGetUsers( fSettings->serverInfo( ii )->keyName() );
    }
}

void CSyncSystem::loadUsersMedia( std::shared_ptr< CUserData > userData )
{
    if ( !userData )
        return;

    fCurrUserData = userData;

    if ( !fCurrUserData->canBeSynced() )
        return;

    fProgressSystem->setTitle( tr( "Loading Users Media" ) );
    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = fSettings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;

        emit sigAddToLog( EMsgType::eInfo, QString( "Loading media for '%1' on server '%2'" ).arg( fCurrUserData->userName( serverInfo->keyName() ) ).arg( serverInfo->displayName() ) );
        requestGetMediaList( serverInfo->keyName() );
    }
}

void CSyncSystem::clearCurrUser()
{
    if ( fCurrUserData )
        fCurrUserData.reset();
}

void CSyncSystem::slotProcess()
{
    selectiveProcess( {} );
}

void CSyncSystem::selectiveProcess( const QString & selectedServer )
{
    auto title = QString( "Processing media for user '%1'" ).arg( fCurrUserData->userName( selectedServer ) );
    if ( !selectedServer.isEmpty() )
        title += QString( " From '%1'" ).arg( selectedServer );
    
    emit sigAddToLog( EMsgType::eInfo, title );

    fProgressSystem->setTitle( title );

    Q_ASSERT( fGetAllMediaFunc );
    if ( !fGetAllMediaFunc )
        return;

    auto allMedia = fGetAllMediaFunc();

    int cnt = 0;
    for ( auto && ii : allMedia )
    {
        if ( !ii || !ii->isValidForAllServers() )
            continue;
        if ( ii->userDataEqual() )
            continue;

        cnt++;
    }

    if ( cnt == 0 )
    {
        fProgressSystem->resetProgress();
        emit sigProcessingFinished( fCurrUserData->userName( selectedServer ) );
        return;
    }

    fProgressSystem->setMaximum( static_cast<int>( cnt ) );

    for ( auto && ii : allMedia )
    {
        if ( !ii || !ii->isValidForAllServers() )
            continue;
        if ( ii->userDataEqual() )
            continue;

        fProgressSystem->incProgress();
        bool dataProcessed = processMedia( ii, selectedServer );
        (void)dataProcessed;
    }
}

bool CSyncSystem::processMedia( std::shared_ptr< CMediaData > mediaData, const QString & selectedServer )
{
    if ( !mediaData || mediaData->userDataEqual() )
        return false;

    if ( !fCurrUserData )
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
    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverName = fSettings->serverInfo( ii )->keyName();
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

void CSyncSystem::updateUserDataForMedia( const QString & serverName, std::shared_ptr<CMediaData> mediaData, std::shared_ptr<SMediaUserData> newData )
{
    requestUpdateUserDataForMedia( serverName, mediaData, newData );

    // TODO: Emby currently doesnt support updating favorite status from the "Users/<>/Items/<>/UserData" API
    //       When it does, remove the requestSetFavorite 
    requestSetFavorite( serverName , mediaData, newData);
}

void CSyncSystem::requestUpdateUserDataForMedia( const QString & serverName, std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData )
{
    if ( !mediaData || !newData )
        return;

    if ( *mediaData->userMediaData( serverName ) == *newData )
        return;

    auto && mediaID = mediaData->getMediaID( serverName );
    auto && userID = fCurrUserData->getUserID( serverName );
    if ( userID.isEmpty() || mediaID.isEmpty() )
        return;

    auto && url = fSettings->serverInfo( serverName )->getUrl( QString( "Users/%1/Items/%2/UserData" ).arg( userID ).arg( mediaID ), {} );
    if ( !url.isValid() )
        return;

    auto obj = newData->userDataJSON();
    QByteArray data = QJsonDocument( obj ).toJson();

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;

    //qDebug() << url;

    auto request = QNetworkRequest( url );
    request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );

    QNetworkReply * reply = nullptr;
    reply = fManager->post( request, data );

    setServerName( reply, serverName );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateData );
}

void CSyncSystem::requestSetFavorite( const QString & serverName, std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData )
{
    if ( mediaData->isFavorite( serverName ) == newData->fIsFavorite )
        return;

    auto && mediaID = mediaData->getMediaID( serverName );
    auto && userID = fCurrUserData->getUserID( serverName );
    if ( userID.isEmpty() || mediaID.isEmpty() )
        return;

    auto && url = fSettings->serverInfo( serverName )->getUrl( QString( "Users/%1/FavoriteItems/%3" ).arg( userID ).arg( mediaID ), {} );
    if ( !url.isValid() )
        return;

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;
    //qDebug() << "updateType" << updateType;

    auto request = QNetworkRequest( url );

    QByteArray data;
    QNetworkReply * reply = nullptr;
    if ( newData->fIsFavorite )
        reply = fManager->post( request, data ); 
    else
        reply = fManager->deleteResource( request );

    setServerName( reply, serverName );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateFavorite );
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
    if ( pos2 == (*pos).second.end() )
        return;

    if ( ( *pos2 ).second > 0 )
        ( *pos2 ).second--;

    if ( (*pos2).second == 0 )
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
        if ( ( requestType == ERequestType::eReloadMediaData ) || ( requestType == ERequestType::eUpdateData ) )
            emit sigProcessingFinished( fCurrUserData->userName( serverName ) );
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
    emit sigAddToLog( EMsgType::eInfo, QString( "There are %1 total pending requests on all servers" ).arg( numRequestsTotal ) );
    for ( auto && ii : msgs )
        emit sigAddToLog( EMsgType::eInfo, ii );
}

void CSyncSystem::testServer( const QString & serverName )
{
    auto serverInfo = fSettings->serverInfo( serverName );
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
    for( auto && ii : servers )
        requestTestServer( ii );
}

bool CSyncSystem::isRunning() const
{
    return !fAttributes.empty() && !fRequests.empty();
}

void CSyncSystem::slotMergeMedia()
{
    if ( !fAttributes.empty() )
    {
        QTimer::singleShot( 500, this, &CSyncSystem::slotMergeMedia );
        return;
    }

    Q_ASSERT( fMergeMediaFunc );
    if ( !fMergeMediaFunc )
        return;

    if ( !fMergeMediaFunc( fProgressSystem ) )
        fCurrUserData.reset();

    emit sigUserMediaLoaded();
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

QNetworkReply * CSyncSystem::makeRequest( const QNetworkRequest & request )
{
    if ( !fPendingRequestTimer )
    {
        fPendingRequestTimer = new QTimer( this );
        fPendingRequestTimer->setSingleShot( false );
        fPendingRequestTimer->setInterval( 1500 );
        connect( fPendingRequestTimer, &QTimer::timeout, this, &CSyncSystem::slotCheckPendingRequests );
    }
    fPendingRequestTimer->start();
    return fManager->get( request );
}

std::shared_ptr< CMediaData > CSyncSystem::loadMedia( const QString & serverName, const QJsonObject & media )
{
    Q_ASSERT( fLoadMediaFunc );
    if ( !fLoadMediaFunc )
        return{};
    return fLoadMediaFunc( serverName, media );
}

std::shared_ptr<CUserData> CSyncSystem::loadUser( const QString & serverName, const QJsonObject & user )
{
    Q_ASSERT( fLoadUserFunc );
    if ( !fLoadUserFunc )
        return {};
    return fLoadUserFunc( serverName, user );
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
        errorMsg = tr( "Error from Server: %1%2" ).arg( reply->errorString() ).arg( data.isEmpty() ? QString() : QString( " - %1" ).arg( QString( data ) ) );
        if ( fUserMsgFunc && reportMsg )
            fUserMsgFunc( tr( "Error response from server" ), errorMsg, true );
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
        case ERequestType::eGetUsers:
            emit sigLoadingUsersFinished();
            break;
        case ERequestType::eGetMediaList:
            emit sigUserMediaLoaded();
            break;
        case ERequestType::eNone:
        case ERequestType::eReloadMediaData:
        case ERequestType::eUpdateData:
        case ERequestType::eUpdateFavorite:
        case ERequestType::eTestServer:
            emit sigTestServerResults(serverName, false, errorMsg );
        default:
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
    case ERequestType::eGetUsers:
        if ( !fProgressSystem->wasCanceled() )
        {
            handleGetUsersResponse( serverName, data );

            if ( isLastRequestOfType( ERequestType::eGetUsers ) )
            {
                emit sigLoadingUsersFinished();
            }
        }
        break;
    case ERequestType::eGetMediaList:
        if ( !fProgressSystem->wasCanceled() )
        {
            handleGetMediaListResponse( serverName, data );

            if ( isLastRequestOfType( ERequestType::eGetMediaList ) )
            {
                fProgressSystem->resetProgress();
                slotMergeMedia();
            }
        }
        break;
    case ERequestType::eReloadMediaData:
    {
        handleReloadMediaResponse( serverName, data, extraData.toString() );
        break;
    }
    case ERequestType::eUpdateData:
    {
        requestReloadMediaItemData( serverName, extraData.toString() );
        if ( fGetMediaDataForIDFunc )
        {
            auto mediaData = fGetMediaDataForIDFunc( serverName, extraData.toString() );
            if ( mediaData )
                emit sigAddToLog( EMsgType::eInfo, tr( "Updated '%1(%2)' on Server '%3' successfully" ).arg( mediaData->name() ).arg( extraData.toString() ).arg( serverName ) );
        }
        break;
    }
    case ERequestType::eUpdateFavorite:
    {
        requestReloadMediaItemData( serverName, extraData.toString() );
        emit sigAddToLog( EMsgType::eInfo, tr( "Updated Favorite status for '%1' on Server '%2' successfully" ).arg( extraData.toString() ).arg( serverName ) );
        break;
    }
    case ERequestType::eTestServer:
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

        break;
    }
    }

    postHandleRequest( reply, serverName, requestType );
}

void CSyncSystem::requestTestServer( std::shared_ptr< const CServerInfo > serverInfo )
{
    fTestServers[ serverInfo->keyName() ] = serverInfo;

    auto && url = serverInfo->getUrl( "Users", {} );
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

void CSyncSystem::requestGetUsers( const QString & serverName )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading users from server '%1'" ).arg( serverName ) );;
    auto && url = fSettings->serverInfo( serverName )->getUrl( "Users", {} );
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
            fUserMsgFunc( tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ), true );
        return;
    }

    //qDebug() << doc.toJson();
    auto users = doc.array();
    fProgressSystem->pushState();

    emit sigAddToLog( EMsgType::eInfo, QString( "Server '%1' has %2 Users" ).arg( serverName ).arg( users.count() ) );

    for ( auto && ii : users )
    {
        if ( fProgressSystem->wasCanceled() )
            break;
        fProgressSystem->incProgress();

        auto user = ii.toObject();
        loadUser( serverName, user );
    }
    fProgressSystem->popState();
    fProgressSystem->incProgress();
}

void CSyncSystem::requestGetMediaList( const QString & serverName )
{
    if ( !fCurrUserData )
        return;

    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "IncludeItemTypes", fSettings->getSyncItemTypes() ),
        std::make_pair( "SortBy", "Type,SortName" ),
        std::make_pair( "SortOrder", "Ascending" ),
        std::make_pair( "Recursive", "True" ),
        std::make_pair( "IsMissing", "False" ),
        std::make_pair( "Fields", "ProviderIds,ExternalUrls,Missing" )
    };

    auto && url = fSettings->serverInfo( serverName )->getUrl( QString( "Users/%1/Items" ).arg( fCurrUserData->getUserID( serverName ) ), queryItems );
    if ( !url.isValid() )
        return;

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting media for '%1' from server '%2'" ).arg( fCurrUserData->userName( serverName ) ).arg( serverName ) );

    auto reply = makeRequest( request );
    setServerName( reply, serverName );
    setRequestType( reply, ERequestType::eGetMediaList );
}

void CSyncSystem::handleGetMediaListResponse( const QString & serverName, const QByteArray & data )
{
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ), true );
        return;
    }

    if ( !fCurrUserData )
        return;

    //qDebug() << doc.toJson();
    if ( !doc[ "Items" ].isArray() )
    {
        loadMedia( serverName, doc.object() );
        return;
    }

    auto mediaList = doc[ "Items" ].toArray();
    fProgressSystem->pushState();

    fProgressSystem->resetProgress();
    fProgressSystem->setTitle( tr( "Loading Users Media Data" ) );
    fProgressSystem->setMaximum( mediaList.count() );

    emit sigAddToLog( EMsgType::eInfo, QString( "%1 has %2 media items on server '%3'" ).arg( fCurrUserData->userName( serverName ) ).arg( mediaList.count() ).arg( serverName ) );
    if ( fSettings->maxItems() > 0 )
        emit sigAddToLog( EMsgType::eInfo, QString( "Loading %2 media items" ).arg( fSettings->maxItems() ) );

    int curr = 0;
    for ( auto && ii : mediaList )
    {
        if ( fSettings->maxItems() > 0 )
        {
            if ( curr >= fSettings->maxItems() )
                break;
        }
        curr++;

        if ( fProgressSystem->wasCanceled() )
            break;
        fProgressSystem->incProgress();

        auto media = ii.toObject();

        loadMedia( serverName, media );
    }
    fProgressSystem->resetProgress();
    fProgressSystem->popState();
    fProgressSystem->incProgress();
}

void CSyncSystem::requestReloadMediaItemData( const QString & serverName, const QString & mediaID )
{
    if ( !fGetMediaDataForIDFunc )
        return;

    auto mediaData = fGetMediaDataForIDFunc( serverName, mediaID );
    if ( !mediaData )
        return;

    requestReloadMediaItemData( serverName, mediaData );
}

void CSyncSystem::requestReloadMediaItemData( const QString & serverName, std::shared_ptr< CMediaData > mediaData )
{
    auto && url = fSettings->serverInfo( serverName )->getUrl( QString( "Users/%1/Items/%2" ).arg( fCurrUserData->getUserID( serverName ) ).arg( mediaData->getMediaID( serverName ) ), {} );
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
            fUserMsgFunc( tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ), true );
        return;
    }

    auto media = doc.object();

    Q_ASSERT( fReloadMediaFunc );
    if ( !fReloadMediaFunc )
        return;

    fReloadMediaFunc( serverName, media, itemID );
}


void CSyncSystem::slotCanceled()
{
    auto tmp = fAttributes;
    for ( auto && ii : tmp )
    {
        ii.first->abort();
    }
    fCurrUserData.reset();
}

std::shared_ptr< CUserData > CSyncSystem::currUser() const
{
    return fCurrUserData;
}