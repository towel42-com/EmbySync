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
#include "UserData.h"
#include "Settings.h"
#include "MediaData.h"

#include <unordered_set>

#include <QTimer>
#include <QMessageBox>
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
        case ERequestType::eUsers: return "Users";
        case ERequestType::eMediaList: return "MediaList";
        case ERequestType::eGetMediaInfo: return "MissingMedia";
        case ERequestType::eReloadMediaData: return "ReloadMediaData";
        case ERequestType::eUpdateData: return "UpdateData";
        case ERequestType::eUpdateFavorite: return "UpdateFavorite";
    }
    return {};
}

CSyncSystem::CSyncSystem( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QObject( parent ),
    fSettings( settings )
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


    connect( this, &CSyncSystem::sigUserMediaLoaded, this, &CSyncSystem::slotFindMissingMedia );
}

void CSyncSystem::setMediaItemFunc( std::function< void( std::shared_ptr< CMediaData > userData ) > mediaItemFunc )
{
    fUpdateMediaFunc = mediaItemFunc;
}

void CSyncSystem::setProcessNewMediaFunc( std::function< void( std::shared_ptr< CMediaData > userData ) > processNewMediaFunc )
{
    fProcessNewMediaFunc = processNewMediaFunc;
}

void CSyncSystem::setUserMsgFunc( std::function< void( const QString & title, const QString & msg, bool isCritical ) > userMsgFunc )
{
    fUserMsgFunc = userMsgFunc;
}

void CSyncSystem::setProgressFunctions( const SProgressFunctions & funcs )
{
    fProgressFuncs = funcs;
}

void CSyncSystem::reset()
{
    fUsers.clear();
    fAttributes.clear();
    resetMedia();
}

void CSyncSystem::resetMedia()
{
    fAllMedia.clear();
    fMissingMedia.clear();
    fLHSMedia.clear();
    fRHSMedia.clear();
    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();
}

void CSyncSystem::loadUsers()
{
    fProgressFuncs.setupProgress( tr( "Loading Users" ) );

    requestUsers( true );
    requestUsers( false );
}

void CSyncSystem::loadUsersMedia( std::shared_ptr< CUserData > userData )
{
    if ( !userData )
        return;

    fCurrUserData = userData;

    emit sigAddToLog( QString( "Loading media for '%1'" ).arg( fCurrUserData->displayName() ) );

    fLHSMedia.clear();
    fRHSMedia.clear();
    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();
    fCurrUserData->clearMedia();

    if ( !fCurrUserData->onLHSServer() && !fCurrUserData->onRHSServer() )
        return;

    fProgressFuncs.setupProgress( tr( "Loading Users Media" ) );
    if ( fCurrUserData->onLHSServer() )
        requestUsersMediaList( true );
    if ( fCurrUserData->onRHSServer() )
        requestUsersMediaList( false );
}

void CSyncSystem::clearCurrUser()
{
    if ( fCurrUserData )
    {
        fCurrUserData->clearMedia();
        fCurrUserData.reset();
    }
    resetMedia();
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

    fProgressFuncs.setupProgress( title );

    int cnt = 0;
    for ( auto && ii : fAllMedia )
    {
        if ( !ii || ii->userDataEqual() || ii->isMissingOnEitherServer() )
            continue;
        cnt++;
    }

    if ( cnt == 0 )
    {
        fProgressFuncs.resetProgress();
        emit sigProcessingFinished( fCurrUserData->displayName() );
        return;
    }

    fProgressFuncs.setMaximum( static_cast<int>( cnt ) );

    for ( auto && ii : fAllMedia )
    {
        if ( !ii || ii->userDataEqual() || ii->isMissingOnEitherServer() )
            continue;

        fProgressFuncs.incProgress();
        bool dataProcessed = processData( ii, forceLeft, forceRight );
        (void)dataProcessed;
    }
}

std::shared_ptr< CUserData > CSyncSystem::getUserData( const QString & name ) const
{
    auto pos = fUsers.find( name );
    if ( pos == fUsers.end() )
        return {};

    auto userData = ( *pos ).second;
    if ( !userData )
        return {};
    return userData;
}

std::list< std::shared_ptr< CMediaData > > CSyncSystem::getAllMedia() const
{
    auto retVal = std::list< std::shared_ptr< CMediaData > >( { fAllMedia.begin(), fAllMedia.end() } );
    return retVal;
}

std::list< std::shared_ptr< CUserData > > CSyncSystem::getAllUsers( bool andClear )
{
    auto retVal = getAllUsers();
    if ( andClear )
        fUsers.clear();
    return std::move( retVal );
}

std::list< std::shared_ptr< CUserData > > CSyncSystem::getAllUsers() const
{
    std::list< std::shared_ptr< CUserData > > retVal;
    for ( auto && ii : fUsers )
    {
        retVal.emplace_back( ii.second );
    }
    return std::move( retVal );
}

std::shared_ptr< CUserData > CSyncSystem::currUser() const
{
    return fCurrUserData;
}

void CSyncSystem::slotFindMissingMedia()
{
    if ( !fCurrUserData->onLHSServer() || !fCurrUserData->onRHSServer() )
    {
        checkForMissingMedia();
        return;
    }

    for ( auto && ii : fAllMedia )
    {
        if ( !ii->hasProviderIDs() )
            continue;

        if ( !ii->isMissingOnEitherServer() )
            continue;

        fMissingMedia[ ii->name() ] = ii;
    }
    if ( checkForMissingMedia() )
        return;

    fProgressFuncs.setupProgress( "Finding missing media data" );
    fProgressFuncs.setMaximum( static_cast<int>( fMissingMedia.size() ) );

    for ( auto && ii : fMissingMedia )
    {
        if ( ii.second->isMissingOnServer( true ) )
            requestMediaInformation( ii.second, true );
        if ( ii.second->isMissingOnServer( false ) )
            requestMediaInformation( ii.second, false );
    }
}

void CSyncSystem::requestUsersMediaList( bool isLHSServer )
{
    if ( !fCurrUserData )
        return;

    auto && url = fSettings->getServerURL( isLHSServer );
    auto path = url.path();
    path += QString( "Users/%1/Items" ).arg( fCurrUserData->getUserID( isLHSServer ) );
    //path += QString( "/%1" ).arg( isLHSServer ? "209179" : "110303" );
    url.setPath( path );

    QUrlQuery query;

    query.addQueryItem( "api_key", isLHSServer ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    //query.addQueryItem( "Filters", "IsPlayed" );
    query.addQueryItem( "IncludeItemTypes", fSettings->getSyncItemTypes() );
    query.addQueryItem( "SortBy", "SortName" );
    query.addQueryItem( "SortOrder", "Ascending" );
    query.addQueryItem( "Recursive", "True" );
    query.addQueryItem( "IsMissing", "False" );
    query.addQueryItem( "Fields", "ProviderIds,ExternalUrls,Missing" );
    url.setQuery( query );

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( QString( "Requesting media for '%1' from server '%2'" ).arg( fCurrUserData->displayName() ).arg( fSettings->getServerName( isLHSServer ) ) );

    auto reply = makeRequest( request );
    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eMediaList );
    setExtraData( reply, fCurrUserData->displayName() );
}

bool CSyncSystem::processData( std::shared_ptr< CMediaData > mediaData, bool forceLeft, bool forceRight )
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
    requestSetFavorite( mediaData, newData, lhsNeedsUpdating );
}

void CSyncSystem::requestUpdateUserDataForMedia( std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData, bool lhsNeedsUpdating )
{
    if ( !mediaData || !newData )
        return;

    if ( ( lhsNeedsUpdating && ( *mediaData->lhsUserMediaData() == *newData ) )
      || ( !lhsNeedsUpdating && ( *mediaData->rhsUserMediaData() == *newData ) ) )
        return;


    auto && url = fSettings->getServerURL( lhsNeedsUpdating );
    auto && mediaID = mediaData->getMediaID( lhsNeedsUpdating );
    auto && userID = fCurrUserData->getUserID( lhsNeedsUpdating );

    if ( userID.isEmpty() || mediaID.isEmpty() )
        return;

    auto obj = newData->userDataJSON();
    QByteArray data = QJsonDocument( obj ).toJson();

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;

    auto path = url.path();
    path += QString( "Users/%1/Items/%2/UserData" ).arg( userID ).arg( mediaID );
    url.setPath( path );
    QUrlQuery query;

    query.addQueryItem( "api_key", lhsNeedsUpdating ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    url.setQuery( query );

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

    auto && url = fSettings->getServerURL( lhsNeedsUpdating );
    auto && mediaID = mediaData->getMediaID( lhsNeedsUpdating );
    auto && userID = fCurrUserData->getUserID( lhsNeedsUpdating );

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;
    //qDebug() << "updateType" << updateType;

    if ( mediaID.isEmpty() || userID.isEmpty() )
        return;

    auto path = url.path();
    path += QString( "Users/%1/FavoriteItems/%3" ).arg( userID ).arg( mediaID );
    url.setPath( path );
    //qDebug() << url;

    QUrlQuery query;
    query.addQueryItem( "api_key", lhsNeedsUpdating ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    url.setQuery( query );

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


void CSyncSystem::requestUsers( bool isLHSServer )
{
    emit sigAddToLog( tr( "Loading users from server '%1'" ).arg( fSettings->getServerName( isLHSServer ) ) );;
    auto && url = fSettings->getServerURL( isLHSServer );
    if ( !url.isValid() )
        return;

    emit sigAddToLog( tr( "Server URL: %1" ).arg( url.toString() ) );

    auto path = url.path();
    path += "Users";
    url.setPath( path );
    //qDebug() << url;

    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );
    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eUsers );
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

    //emit sigAddToLog( QString( "Request Completed: %1" ).arg( reply->url().toString() ) );
    //emit sigAddToLog( QString( "Is LHS? %1" ).arg( isLHSServer ? "Yes" : "No" ) );
    //emit sigAddToLog( QString( "Request Type: %1" ).arg( toString( requestType ) ) );
    //emit sigAddToLog( QString( "Extra Data: %1" ).arg( extraData.toString() ) );

    if ( !handleError( reply ) )
    {
        switch ( requestType )
        {
            case ERequestType::eUsers:
                emit sigLoadingUsersFinished();
                break;
            case ERequestType::eMediaList:
                emit sigUserMediaLoaded();
                break;
            case ERequestType::eNone:
            case ERequestType::eGetMediaInfo:
            case ERequestType::eReloadMediaData:
            case ERequestType::eUpdateData:
            case ERequestType::eUpdateFavorite:
            default:
                break;
        }

        postHandlRequest( requestType, reply );
        return;
    }

    //qDebug() << "Requests Remaining" << fAttributes.size();
    auto data = reply->readAll();
    //qDebug() << data;

    switch ( requestType )
    {
        case ERequestType::eNone:
            break;
        case ERequestType::eUsers:
            if ( !fProgressFuncs.wasCanceled() )
            {
                loadUsers( data, isLHSServer );

                if ( isLastRequestOfType( ERequestType::eUsers ) )
                {
                    emit sigLoadingUsersFinished();
                }
            }
            break;
        case ERequestType::eMediaList:
            if ( !fProgressFuncs.wasCanceled() )
            {
                loadMediaList( data, isLHSServer );

                if ( isLastRequestOfType( ERequestType::eMediaList ) )
                {
                    fProgressFuncs.resetProgress();
                    slotMergeMedia();
                }
            }
            break;
        case ERequestType::eGetMediaInfo:
            if ( !fProgressFuncs.wasCanceled() )
            {
                loadMediaInfo( data, extraData.toString(), isLHSServer );
                fProgressFuncs.incProgress();
            }
            break;
        case ERequestType::eReloadMediaData:
        {
            reloadMediaData( data, isLHSServer, extraData.toString() );
            break;
        }
        case ERequestType::eUpdateData:
        {
            loadMediaData( extraData.toString(), isLHSServer );
            QString name;
            if ( isLHSServer )
            {
                auto pos = fLHSMedia.find( extraData.toString() );
                if ( pos != fLHSMedia.end() )
                    name = ( *pos ).second->name();
            }
            else
            {
                auto pos = fRHSMedia.find( extraData.toString() );
                if ( pos != fRHSMedia.end() )
                    name = ( *pos ).second->name();
            }

            emit sigAddToLog( QString( "Updated '%1(%2)' on Server '%3' successfully" ).arg( name ).arg( extraData.toString() ).arg( fSettings->getServerName( isLHSServer ) ) );
            break;
        }
        case ERequestType::eUpdateFavorite:
        {
            loadMediaData( extraData.toString(), isLHSServer );
            emit sigAddToLog( QString( "Updated Favorite status for '%1' on Server '%2' successfully" ).arg( extraData.toString() ).arg( fSettings->getServerName( isLHSServer ) ) );
            break;
        }
    }

    postHandlRequest( requestType, reply );
}

void CSyncSystem::loadMediaData( const QString & mediaID, bool isLHSServer )
{
    auto pos = isLHSServer ? fLHSMedia.find( mediaID ) : fRHSMedia.find( mediaID );
    if ( pos != ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
    {
        requestReloadMediaData( ( *pos ).second, isLHSServer );
    }
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

void CSyncSystem::postHandlRequest( ERequestType requestType, QNetworkReply * reply )
{
    decRequestCount( requestType, reply );
    if ( !isRunning() )
    {
        fProgressFuncs.resetProgress();
        if ( requestType == ERequestType::eGetMediaInfo )
            emit sigUserMediaCompletelyLoaded();
        else if ( ( requestType == ERequestType::eReloadMediaData ) || ( requestType == ERequestType::eUpdateData ) )
            emit sigProcessingFinished( fCurrUserData->displayName() );
    }
}

void CSyncSystem::slotCheckPendingRequests()
{
    if ( !isRunning() )
    {
        emit sigAddToLog( QString( "Info: 0 pending requests on any server" ) );
        fPendingRequestTimer->stop();
        return;
    }
    for ( auto && ii : fRequests )
    {
        for ( auto && jj : ii.second )
        {
            if ( jj.second )
            {
                emit sigAddToLog( QString( "Info: %1 pending '%2' request%3 on server '%4'" ).arg( jj.second ).arg( toString( ii.first ) ).arg( ( jj.second != 1 ) ? "s" : "" ).arg( jj.first ) );
            }
        }
    }
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

    fLoadingMediaData = false;

    fProgressFuncs.resetProgress();
    fProgressFuncs.setupProgress( tr( "Merging media data" ) );
    fProgressFuncs.setMaximum( static_cast< int >( fLHSMedia.size() * 3 + fRHSMedia.size() * 3 ) );


    mergeMediaData( fLHSMedia, fRHSMedia, true );
    mergeMediaData( fRHSMedia, fLHSMedia, false );

    for ( auto && ii : fLHSMedia )
    {
        fAllMedia.insert( ii.second );
        fProgressFuncs.incProgress();
    }

    for ( auto && ii : fRHSMedia )
    {
        fAllMedia.insert( ii.second );
        fProgressFuncs.incProgress();
    }

    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();

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

bool CSyncSystem::handleError( QNetworkReply * reply )
{
    Q_ASSERT( reply );
    if ( !reply )
        return false;

    if ( reply && ( reply->error() != QNetworkReply::NoError ) ) // replys with an error do not get cached
    {
        auto data = reply->readAll();
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Error response from server" ), tr( "Error from Server: %1%2" ).arg( reply->errorString() ).arg( data.isEmpty() ? QString() : QString( " - %1" ).arg( QString( data ) ) ), true );
        return false;
    }
    return true;
}

void CSyncSystem::loadUsers( const QByteArray & data, bool isLHSServer )
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
    fProgressFuncs.setMaximum( users.count() );

    emit sigAddToLog( QString( "Server '%1' has %2 Users" ).arg( fSettings->getServerName( isLHSServer ) ).arg( users.count() ) );

    for ( auto && ii : users )
    {
        if ( fProgressFuncs.wasCanceled() )
            break;
        fProgressFuncs.incProgress();


        auto user = ii.toObject();
        auto name = user[ "Name" ].toString();
        auto id = user[ "Id" ].toString();
        if ( name.isEmpty() || id.isEmpty() )
            continue;

        auto linkType = user[ "ConnectLinkType" ].toString();
        QString connectedID;
        if ( linkType == "LinkedUser" )
            connectedID = user[ "ConnectUserName" ].toString();

        auto userData = getUserData( connectedID );
        if ( !userData )
            userData = getUserData( name );

        if ( !userData )
        {
            userData = std::make_shared< CUserData >( name, connectedID, isLHSServer );
            if ( !connectedID.isEmpty() )
                fUsers[ userData->connectedID() ] = userData;
            else
                fUsers[ userData->name( isLHSServer ) ] = userData;
        }

        userData->setName( name, isLHSServer );
        userData->setUserID( id, isLHSServer );
    }
}

void CSyncSystem::loadMediaInfo( const QByteArray & data, const QString & mediaName, bool isLHSServer )
{
    fProgressFuncs.incProgress();
    //qDebug() << progressDlg()->value() << fMissingMedia.size();

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ), true );
        return;
    }

    //qDebug() << doc.toJson();
    auto mediaList = doc[ "Items" ].toArray();
    //fProgressFuncs.setMaximum( mediaList.count() );

    bool found = false;
    for ( auto && ii : mediaList )
    {
        auto media = ii.toObject();

        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        auto id = media[ "Id" ].toString();
        auto name = CMediaData::computeName( media );

        auto pos = fMissingMedia.find( name );
        //Q_ASSERT( pos != fMissingMedia.end() );
        if ( pos == fMissingMedia.end() )
            continue;

        found = true;
        auto mediaData = ( *pos ).second;
        fMissingMedia.erase( pos );
        if ( isLHSServer )
            mediaData->setMediaID( id, isLHSServer );
        else
            mediaData->setMediaID( id, !isLHSServer );

        //qDebug() << isLHSServer << mediaData->name();


        /*
        "UserData": {
        "IsFavorite": false,
        "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
        "PlayCount": 1,
        "PlaybackPositionTicks": 0,
        "Played": true
        }
        */

        addMediaInfo( mediaData, media, isLHSServer );
        if ( fUpdateMediaFunc && fMissingMedia.empty() )
            fUpdateMediaFunc( mediaData );
        break;
    }
    if ( !found )
    {
        emit sigAddToLog( QString( "Error:  COULD NOT FIND MEDIA '%1' on %2 server" ).arg( mediaName ).arg( fSettings->getServerName( isLHSServer ) ) );
        auto pos = fMissingMedia.find( mediaName );
        if ( pos != fMissingMedia.end() )
            fMissingMedia.erase( pos );
    }

    checkForMissingMedia();
}

bool CSyncSystem::checkForMissingMedia()
{
    if ( fMissingMedia.empty() )
        emit sigFinishedCheckingForMissingMedia();
    return fMissingMedia.empty();
}

void CSyncSystem::requestMediaInformation( std::shared_ptr< CMediaData > mediaData, bool forLHS )
{
    if ( !mediaData || !mediaData->hasProviderIDs() )
        return;

    QUrl url;
    QString apiKey;
    if ( forLHS )
    {
        url = fSettings->getServerURL( true );
        apiKey = fSettings->lhsAPI();
    }
    else
    {
        url = fSettings->getServerURL( false );
        apiKey = fSettings->rhsAPI();
    }

    auto path = url.path();
    path += QString( "Items" );
    url.setPath( path );

    auto query = mediaData->getSearchForMediaQuery();
    query.addQueryItem( "api_key", forLHS ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    url.setQuery( query );


    //qDebug() << mediaData->name() << url;
    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );

    setIsLHS( reply, forLHS );
    setExtraData( reply, mediaData->name() );
    setRequestType( reply, ERequestType::eGetMediaInfo );
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

void CSyncSystem::loadMediaList( const QByteArray & data, bool isLHSServer )
{
    if ( !fCurrUserData )
        return;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ), true );
        return;
    }

    //qDebug() << doc.toJson();
    auto mediaList = doc[ "Items" ].toArray();
    if ( mediaList.size() == 0 )
    {
        loadMedia( doc.object(), isLHSServer );
        return;
    }
    fProgressFuncs.resetProgress();
    fProgressFuncs.setupProgress( tr( "Loading Users Media Data" ) );
    fProgressFuncs.setMaximum( mediaList.count() );

    emit sigAddToLog( QString( "%1 has %2 media items on server '%3'" ).arg( fCurrUserData->displayName() ).arg( mediaList.count() ).arg( fSettings->getServerName( isLHSServer ) ) );
    if ( fSettings->maxItems() > 0 )
        emit sigAddToLog( QString( "Loading %2 media items" ).arg( fSettings->maxItems() ) );

    int curr = 0;
    for ( auto && ii : mediaList )
    {
        if ( fSettings->maxItems() > 0 )
        {
            if ( curr >= fSettings->maxItems() )
                break;
        }
        curr++;

        if ( fProgressFuncs.wasCanceled() )
            break;
        fProgressFuncs.incProgress();

        auto media = ii.toObject();

        loadMedia( media, isLHSServer );

    }
    fProgressFuncs.resetProgress();
}

void CSyncSystem::loadMedia( const QJsonObject & media, bool isLHSServer )
{
    auto id = media[ "Id" ].toString();
    std::shared_ptr< CMediaData > mediaData;
    auto pos = ( isLHSServer ? fLHSMedia.find( id ) : fRHSMedia.find( id ) );
    if ( pos == ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
        mediaData = std::make_shared< CMediaData >( CMediaData::computeName( media ), media[ "Type" ].toString() );
    else
        mediaData = ( *pos ).second;
    //qDebug() << isLHSServer << mediaData->name();

    mediaData->setMediaID( id, isLHSServer );

    /*
    "UserData": {
    "IsFavorite": false,
    "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
    "PlayCount": 1,
    "PlaybackPositionTicks": 0,
    "Played": true
    }
    */

    addMediaInfo( mediaData, media, isLHSServer );
    fCurrUserData->addMedia( mediaData );
    fProgressFuncs.incProgress();
}

void CSyncSystem::addMediaInfo( std::shared_ptr<CMediaData> mediaData, const QJsonObject & mediaInfo, bool isLHSServer )
{
    mediaData->loadUserDataFromJSON( mediaInfo, isLHSServer );
    if ( isLHSServer )
        fLHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;
    else
        fRHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;

    auto && providers = mediaData->getProviders( true );
    for ( auto && ii : providers )
    {
        if ( isLHSServer )
            fLHSProviderSearchMap[ ii.first ][ ii.second ] = mediaData;
        else
            fRHSProviderSearchMap[ ii.first ][ ii.second ] = mediaData;
    }
}

void CSyncSystem::requestReloadMediaData( std::shared_ptr< CMediaData > mediaData, bool isLHSServer )
{
    auto && url = fSettings->getServerURL( isLHSServer );
    auto path = url.path();
    path += QString( "Users/%1/Items/%2" ).arg( fCurrUserData->getUserID( isLHSServer ) ).arg( mediaData->getMediaID( isLHSServer ) );
    url.setPath( path );

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    auto reply = makeRequest( request );

    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eReloadMediaData );
    setExtraData( reply, mediaData->getMediaID( isLHSServer ) );
    //qDebug() << "Media Data for " << mediaData->name() << reply;
}

void CSyncSystem::reloadMediaData( const QByteArray & data, bool isLHSServer, const QString & itemID )
{
    auto pos = ( isLHSServer ? fLHSMedia.find( itemID ) : fRHSMedia.find( itemID ) );
    if ( pos == ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
        return;

    auto mediaData = ( *pos ).second;
    if ( !mediaData )
        return;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ), true );
        return;
    }


    auto media = doc.object();
    if ( media.find( "UserData" ) == media.end() )
        return;

    addMediaInfo( mediaData, media, isLHSServer );
    if ( fUpdateMediaFunc )
        fUpdateMediaFunc( mediaData );

}

void CSyncSystem::loadMediaData( const QByteArray & data, bool isLHSServer, const QString & itemID )
{
    auto pos = ( isLHSServer ? fLHSMedia.find( itemID ) : fRHSMedia.find( itemID ) );
    if ( pos == ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
        return;

    auto mediaData = ( *pos ).second;
    if ( !mediaData )
        return;

    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( fUserMsgFunc )
            fUserMsgFunc( tr( "Invalid Response" ), tr( "Invalid Response from Server: %1 - %2" ).arg( error.errorString() ).arg( QString( data ) ).arg( error.offset ), true );
        return;
    }

    //qDebug() << doc.toJson();
    auto mediaObject = doc.object();
    auto providerIDsObj = mediaObject[ "ProviderIds" ].toObject();
    for ( auto && ii = providerIDsObj.begin(); ii != providerIDsObj.end(); ++ii )
    {
        auto providerName = ii.key();
        auto providerID = ii.value().toString();
        mediaData->addProvider( providerName, providerID );
        if ( isLHSServer )
        {
            fLHSProviderSearchMap[ providerName ][ providerID ] = mediaData;
        }
        else
        {
            fRHSProviderSearchMap[ providerName ][ providerID ] = mediaData;
        }
    }

    if ( fProcessNewMediaFunc )
        fProcessNewMediaFunc( mediaData );
}

void CSyncSystem::mergeMediaData( TMediaIDToMediaData & lhs, TMediaIDToMediaData & rhs, bool lhsIsLHS )
{
    mergeMediaData( lhs, lhsIsLHS );
    mergeMediaData( rhs, !lhsIsLHS );
}

void CSyncSystem::mergeMediaData( TMediaIDToMediaData & lhs, bool lhsIsLHS )
{
    std::unordered_map< std::shared_ptr< CMediaData >, std::shared_ptr< CMediaData > > replacementMap;
    for ( auto && ii : lhs )
    {
        fProgressFuncs.incProgress();
        auto mediaData = ii.second;
        if ( !mediaData )
            continue;
        QStringList dupeProviderForMedia;
        for ( auto && jj : mediaData->getProviders( true ) )
        {
            auto providerName = jj.first;
            auto providerID = jj.second;

            auto myMappedMedia = findMediaForProvider( providerName, providerID, lhsIsLHS );
            if ( myMappedMedia != mediaData )
            {
                replacementMap[ mediaData ] = myMappedMedia;
                continue;
            }

            auto otherData = findMediaForProvider( providerName, providerID, !lhsIsLHS );
            if ( otherData != mediaData )
                setMediaForProvider( providerName, providerID, mediaData, !lhsIsLHS );
        }
    }
    for ( auto && ii : replacementMap )
    {
        auto currMediaID = ii.first->getMediaID( lhsIsLHS );
        auto pos = lhs.find( currMediaID );
        lhs.erase( pos );
        ii.second->updateFromOther( ii.first, lhsIsLHS );

        lhs[ currMediaID ] = ii.second;
    }
}

std::shared_ptr< CMediaData > CSyncSystem::findMediaForProvider( const QString & provider, const QString & id, bool lhs )
{
    auto && map = lhs ? fLHSProviderSearchMap : fRHSProviderSearchMap;
    auto pos = map.find( provider );
    if ( pos == map.end() )
        return {};

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == ( *pos ).second.end() )
        return {};
    return ( *pos2 ).second;
}

void CSyncSystem::setMediaForProvider( const QString & providerName, const QString & providerID, std::shared_ptr< CMediaData > mediaData, bool isLHS )
{
    ( isLHS ? fLHSProviderSearchMap : fRHSProviderSearchMap )[ providerName ][ providerID ] = mediaData;
}

void SProgressFunctions::setupProgress( const QString & title )
{
    if ( fSetupFunc )
        fSetupFunc( title );
}

void SProgressFunctions::setMaximum( int count )
{
    if ( fSetMaximumFunc )
        fSetMaximumFunc( count );
}

void SProgressFunctions::incProgress()
{
    if ( fIncFunc )
        fIncFunc();
}

void SProgressFunctions::resetProgress() const
{
    if ( fResetFunc )
        fResetFunc();
}

bool SProgressFunctions::wasCanceled() const
{
    if ( fWasCanceledFunc )
        return fWasCanceledFunc();
    return false;
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

