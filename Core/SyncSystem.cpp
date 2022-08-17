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
#include "MediaData.h"

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

void CSyncSystem::setLoadUserFunc( std::function< std::shared_ptr< CUserData >( const QJsonObject & user, bool isLHS ) > loadUserFunc )
{
    fLoadUserFunc = loadUserFunc;
}

void CSyncSystem::setLoadMediaFunc( std::function< std::shared_ptr< CMediaData >( const QJsonObject & media, bool isLHS ) > loadMediaFunc )
{
    fLoadMediaFunc = loadMediaFunc;
}

void CSyncSystem::setReloadMediaFunc( std::function< std::shared_ptr< CMediaData >( const QJsonObject & media, const QString & itemID, bool isLHS ) > reloadMediaFunc )
{
    fReloadMediaFunc = reloadMediaFunc;
}

void CSyncSystem::setGetMediaDataForIDFunc( std::function< std::shared_ptr< CMediaData >( const QString & mediaID, bool isLHS ) > getMediaDataForIDFunc )
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
    fProgressSystem->setMaximum( 2 );

    requestGetUsers( true );
    requestGetUsers( false );
}

void CSyncSystem::loadUsersMedia( std::shared_ptr< CUserData > userData )
{
    if ( !userData )
        return;

    fCurrUserData = userData;

    emit sigAddToLog( EMsgType::eInfo, QString( "Loading media for '%1'" ).arg( fCurrUserData->displayName() ) );

    if ( !fCurrUserData->onLHSServer() && !fCurrUserData->onRHSServer() )
        return;

    fProgressSystem->setTitle( tr( "Loading Users Media" ) );
    requestUsersMediaList( true );
    requestUsersMediaList( false );
}

void CSyncSystem::clearCurrUser()
{
    if ( fCurrUserData )
        fCurrUserData.reset();
}

void CSyncSystem::slotProcess()
{
    process( false, false );
}

void CSyncSystem::slotProcessToLeft()
{
    process( true, false );
}

void CSyncSystem::slotProcessToRight()
{
    process( false, true );
}

void CSyncSystem::process( bool forceLeft, bool forceRight )
{
    auto title = QString( "Processing media for user '%1'" ).arg( fCurrUserData->displayName() );
    if ( forceLeft )
        title += " To the Left";
    else if ( forceLeft )
        title += " To the Right";

    fProgressSystem->setTitle( title );

    Q_ASSERT( fGetAllMediaFunc );
    if ( !fGetAllMediaFunc )
        return;

    auto allMedia = fGetAllMediaFunc();

    int cnt = 0;
    for ( auto && ii : allMedia )
    {
        if ( !ii || ii->userDataEqual() || ii->isMissingOnEitherServer() )
            continue;
        cnt++;
    }

    if ( cnt == 0 )
    {
        fProgressSystem->resetProgress();
        emit sigProcessingFinished( fCurrUserData->displayName() );
        return;
    }

    fProgressSystem->setMaximum( static_cast<int>( cnt ) );

    for ( auto && ii : allMedia )
    {
        if ( !ii || ii->userDataEqual() || ii->isMissingOnEitherServer() )
            continue;

        fProgressSystem->incProgress();
        bool dataProcessed = processMedia( ii, forceLeft, forceRight );
        (void)dataProcessed;
    }
}

bool CSyncSystem::processMedia( std::shared_ptr< CMediaData > mediaData, bool forceLeft, bool forceRight )
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
    bool lhsNeedsUpdating = mediaData->lhsNeedsUpdating();
    if ( forceLeft )
        lhsNeedsUpdating = true;
    else if ( forceRight )
        lhsNeedsUpdating = false;
    auto newData = lhsNeedsUpdating ? mediaData->rhsUserMediaData() : mediaData->lhsUserMediaData();
    updateUserDataForMedia( mediaData, newData, lhsNeedsUpdating );
    return true;
}

void CSyncSystem::updateUserDataForMedia( std::shared_ptr<CMediaData> mediaData, std::shared_ptr<SMediaUserData> newData, bool lhsNeedsUpdating )
{
    requestUpdateUserDataForMedia( mediaData, newData, lhsNeedsUpdating );

    // TODO: Emby currently doesnt support updating favorite status from the "Users/<>/Items/<>/UserData" API
    //       When it does, remove the requestSetFavorite 
    requestSetFavorite( mediaData, newData, lhsNeedsUpdating );
}

void CSyncSystem::requestUpdateUserDataForMedia( std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData, bool lhsNeedsUpdating )
{
    if ( !mediaData || !newData )
        return;

    if ( ( lhsNeedsUpdating && ( *mediaData->lhsUserMediaData() == *newData ) )
      || ( !lhsNeedsUpdating && ( *mediaData->rhsUserMediaData() == *newData ) ) )
        return;


    auto && mediaID = mediaData->getMediaID( lhsNeedsUpdating );
    auto && userID = fCurrUserData->getUserID( lhsNeedsUpdating );
    if ( userID.isEmpty() || mediaID.isEmpty() )
        return;

    auto && url = fSettings->getUrl( QString( "Users/%1/Items/%2/UserData" ).arg( userID ).arg( mediaID ), {}, lhsNeedsUpdating );
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

    setIsLHS( reply, lhsNeedsUpdating );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateData );
}

//void CSyncSystem::setMediaData( std::shared_ptr< CMediaData > mediaData, bool deleteUpdate, const QString & updateType = "" )
void CSyncSystem::requestSetFavorite( std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData, bool lhsNeedsUpdating )
{
    if ( mediaData->isFavorite( lhsNeedsUpdating ) == newData->fIsFavorite )
        return;

    auto && mediaID = mediaData->getMediaID( lhsNeedsUpdating );
    auto && userID = fCurrUserData->getUserID( lhsNeedsUpdating );
    if ( userID.isEmpty() || mediaID.isEmpty() )
        return;

    auto && url = fSettings->getUrl( QString( "Users/%1/FavoriteItems/%3" ).arg( userID ).arg( mediaID ), {}, lhsNeedsUpdating );
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

    setIsLHS( reply, lhsNeedsUpdating );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateFavorite );
}

void CSyncSystem::setIsLHS( QNetworkReply * reply, bool isLHSServer )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kLHSServer ] = isLHSServer;
}

bool CSyncSystem::isLHSServer( QNetworkReply * reply )
{
    if ( !reply )
        return false;
    return fAttributes[ reply ][ kLHSServer ].toBool();
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

void CSyncSystem::decRequestCount( ERequestType requestType, QNetworkReply * reply )
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

void CSyncSystem::postHandleRequest( ERequestType requestType, QNetworkReply * reply )
{
    decRequestCount( requestType, reply );
    if ( !isRunning() )
    {
        fProgressSystem->resetProgress();
        if ( ( requestType == ERequestType::eReloadMediaData ) || ( requestType == ERequestType::eUpdateData ) )
            emit sigProcessingFinished( fCurrUserData->displayName() );
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

std::shared_ptr< CMediaData > CSyncSystem::loadMedia( const QJsonObject & media, bool isLHSServer )
{
    Q_ASSERT( fLoadMediaFunc );
    if ( !fLoadMediaFunc )
        return{};
    return fLoadMediaFunc( media, isLHSServer );
}

std::shared_ptr<CUserData> CSyncSystem::loadUser( const QJsonObject & user, bool isLHSServer )
{
    Q_ASSERT( fLoadUserFunc );
    if ( !fLoadUserFunc )
        return {};
    return fLoadUserFunc( user, isLHSServer );
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
bool CSyncSystem::handleError( QNetworkReply * reply, bool isLHSServer )
{
    Q_ASSERT( reply );
    if ( !reply )
        return false;

    if ( reply && ( reply->error() != QNetworkReply::NoError ) ) // replys with an error do not get cached
    {
        if ( reply->error() == QNetworkReply::OperationCanceledError )
        {
            emit sigAddToLog( EMsgType::eWarning, QString( "Request canceled on server '%1'" ).arg( fSettings->serverName( isLHSServer ) ) );
            return false;
        }

        auto data = reply->readAll();
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Error response from server" ), tr( "Error from Server: %1%2" ).arg( reply->errorString() ).arg( data.isEmpty() ? QString() : QString( " - %1" ).arg( QString( data ) ) ), true );
        return false;
    }
    return true;
}

void CSyncSystem::slotRequestFinished( QNetworkReply * reply )
{
    auto isLHSServer = this->isLHSServer( reply );
    auto requestType = this->requestType( reply );
    auto extraData = this->extraData( reply );

    auto pos = fAttributes.find( reply );
    if ( pos != fAttributes.end() )
    {
        fAttributes.erase( pos );
    }

    //emit sigAddToLog( EMsgType::eInfo, QString( "Request Completed: %1" ).arg( reply->url().toString() ) );
    //emit sigAddToLog( EMsgType::eInfo, QString( "Is LHS? %1" ).arg( isLHSServer ? "Yes" : "No" ) );
    //emit sigAddToLog( EMsgType::eInfo, QString( "Request Type: %1" ).arg( toString( requestType ) ) );
    //emit sigAddToLog( EMsgType::eInfo, QString( "Extra Data: %1" ).arg( extraData.toString() ) );

    if ( !handleError( reply, isLHSServer ) )
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
        default:
            break;
        }

        postHandleRequest( requestType, reply );
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
            handleGetUsersResponse( data, isLHSServer );

            if ( isLastRequestOfType( ERequestType::eGetUsers ) )
            {
                emit sigLoadingUsersFinished();
            }
        }
        break;
    case ERequestType::eGetMediaList:
        if ( !fProgressSystem->wasCanceled() )
        {
            handleGetMediaListResponse( data, isLHSServer );

            if ( isLastRequestOfType( ERequestType::eGetMediaList ) )
            {
                fProgressSystem->resetProgress();
                slotMergeMedia();
            }
        }
        break;
    case ERequestType::eReloadMediaData:
    {
        handleReloadMediaResponse( data, isLHSServer, extraData.toString() );
        break;
    }
    case ERequestType::eUpdateData:
    {
        requestReloadMediaItemData( extraData.toString(), isLHSServer );
        if ( fGetMediaDataForIDFunc )
        {
            auto mediaData = fGetMediaDataForIDFunc( extraData.toString(), isLHSServer );
            if ( mediaData )
                emit sigAddToLog( EMsgType::eInfo, QString( "Updated '%1(%2)' on Server '%3' successfully" ).arg( mediaData->name() ).arg( extraData.toString() ).arg( fSettings->serverName( isLHSServer ) ) );
        }
        break;
    }
    case ERequestType::eUpdateFavorite:
    {
        requestReloadMediaItemData( extraData.toString(), isLHSServer );
        emit sigAddToLog( EMsgType::eInfo, QString( "Updated Favorite status for '%1' on Server '%2' successfully" ).arg( extraData.toString() ).arg( fSettings->serverName( isLHSServer ) ) );
        break;
    }
    }

    postHandleRequest( requestType, reply );
}

void CSyncSystem::requestGetUsers( bool isLHSServer )
{
    emit sigAddToLog( EMsgType::eInfo, tr( "Loading users from server '%1'" ).arg( fSettings->serverName( isLHSServer ) ) );;
    auto && url = fSettings->getUrl( "Users", {}, isLHSServer );
    if ( !url.isValid() )
        return;

    emit sigAddToLog( EMsgType::eInfo, tr( "Server URL: %1" ).arg( url.toString() ) );

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eGetUsers );
}

void CSyncSystem::handleGetUsersResponse( const QByteArray & data, bool isLHSServer )
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

    emit sigAddToLog( EMsgType::eInfo, QString( "Server '%1' has %2 Users" ).arg( fSettings->serverName( isLHSServer ) ).arg( users.count() ) );

    for ( auto && ii : users )
    {
        if ( fProgressSystem->wasCanceled() )
            break;
        fProgressSystem->incProgress();

        auto user = ii.toObject();
        loadUser( user, isLHSServer );
    }
    fProgressSystem->popState();
    fProgressSystem->incProgress();
}

void CSyncSystem::requestUsersMediaList( bool isLHSServer )
{
    if ( !fCurrUserData )
        return;

    std::list< std::pair< QString, QString > > queryItems =
    {
        std::make_pair( "IncludeItemTypes", fSettings->getSyncItemTypes() ),
        std::make_pair( "SortBy", "SortName" ),
        std::make_pair( "SortOrder", "Ascending" ),
        std::make_pair( "Recursive", "True" ),
        std::make_pair( "IsMissing", "False" ),
        std::make_pair( "Fields", "ProviderIds,ExternalUrls,Missing" )
    };

    auto && url = fSettings->getUrl( QString( "Users/%1/Items" ).arg( fCurrUserData->getUserID( isLHSServer ) ), queryItems, isLHSServer );
    if ( !url.isValid() )
        return;

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( EMsgType::eInfo, QString( "Requesting media for '%1' from server '%2'" ).arg( fCurrUserData->displayName() ).arg( fSettings->serverName( isLHSServer ) ) );

    auto reply = makeRequest( request );
    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eGetMediaList );
    setExtraData( reply, fCurrUserData->displayName() );
}

void CSyncSystem::handleGetMediaListResponse( const QByteArray & data, bool isLHSServer )
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
        loadMedia( doc.object(), isLHSServer );
        return;
    }

    auto mediaList = doc[ "Items" ].toArray();
    fProgressSystem->pushState();

    fProgressSystem->resetProgress();
    fProgressSystem->setTitle( tr( "Loading Users Media Data" ) );
    fProgressSystem->setMaximum( mediaList.count() );

    emit sigAddToLog( EMsgType::eInfo, QString( "%1 has %2 media items on server '%3'" ).arg( fCurrUserData->displayName() ).arg( mediaList.count() ).arg( fSettings->serverName( isLHSServer ) ) );
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

        loadMedia( media, isLHSServer );
    }
    fProgressSystem->resetProgress();
    fProgressSystem->popState();
    fProgressSystem->incProgress();
}

void CSyncSystem::requestReloadMediaItemData( const QString & mediaID, bool isLHSServer )
{
    if ( !fGetMediaDataForIDFunc )
        return;

    auto mediaData = fGetMediaDataForIDFunc( mediaID, isLHSServer );
    if ( !mediaData )
        return;

    requestReloadMediaItemData( mediaData, isLHSServer );
}

void CSyncSystem::requestReloadMediaItemData( std::shared_ptr< CMediaData > mediaData, bool isLHSServer )
{
    auto && url = fSettings->getUrl( QString( "Users/%1/Items/%2" ).arg( fCurrUserData->getUserID( isLHSServer ) ).arg( mediaData->getMediaID( isLHSServer ) ), {}, isLHSServer );
    if ( !url.isValid() )
        return;

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );

    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eReloadMediaData );
    setExtraData( reply, mediaData->getMediaID( isLHSServer ) );
    //qDebug() << "Media Data for " << mediaData->name() << reply;
}

void CSyncSystem::handleReloadMediaResponse( const QByteArray & data, bool isLHSServer, const QString & itemID )
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

    fReloadMediaFunc( media, itemID, isLHSServer );
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