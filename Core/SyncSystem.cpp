﻿// The MIT License( MIT )
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
        case ERequestType::eNone: return "eNone";
        case ERequestType::eUsers: return "eUsers";
        case ERequestType::eMediaList: return "eMediaList";
        case ERequestType::eGetMediaInfo: return "eMissingMedia";
        case ERequestType::eMediaData: return "eMediaData";
        case ERequestType::eReloadMediaData: return "eReloadMediaData";
        case ERequestType::eUpdateData: return "eUpdateData";
    }
    return {};
}

CSyncSystem::CSyncSystem( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QObject( parent ),
    fSettings( settings )
{
    fManager = new QNetworkAccessManager( this );
    fManager->setAutoDeleteReplies( true );

    connect( fManager, &QNetworkAccessManager::authenticationRequired, this, &CSyncSystem::slotAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::encrypted, this, &CSyncSystem::slotEncrypted );
    connect( fManager, &QNetworkAccessManager::preSharedKeyAuthenticationRequired, this, &CSyncSystem::slotPreSharedKeyAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::proxyAuthenticationRequired, this, &CSyncSystem::slotProxyAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::sslErrors, this, &CSyncSystem::slotSSlErrors );
    connect( fManager, &QNetworkAccessManager::finished, this, &CSyncSystem::slotRequestFinished );
}

void CSyncSystem::setUserItemFunc( std::function< void( std::shared_ptr< CUserData > userData ) > updateUserFunc )
{
    fUpdateUserFunc = updateUserFunc;
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

bool CSyncSystem::isRunning() const
{
    return !fAttributes.empty();
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

    emit sigAddToLog( QString( "Loading watched media for '%1'" ).arg( fCurrUserData->name() ) );

    fLHSMedia.clear();
    fRHSMedia.clear();
    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();
    fCurrUserData->clearWatchedMedia();

    if ( !fCurrUserData->onLHSServer() && !fCurrUserData->onRHSServer() )
        return;

    fProgressFuncs.setupProgress( tr( "Loading Users Played Media" ) );
    if ( fCurrUserData->onLHSServer() )
        requestUsersPlayedMedia( true );
    if ( fCurrUserData->onRHSServer() )
        requestUsersPlayedMedia( false );
}

void CSyncSystem::clearCurrUser()
{
    if ( fCurrUserData )
    {
        fCurrUserData->clearWatchedMedia();
        fCurrUserData.reset();
    }
    resetMedia();
}

void CSyncSystem::process()
{
    fProgressFuncs.setupProgress( "Processing Data" );
    fProgressFuncs.setMaximum( static_cast<int>( fAllMedia.size() ) );
    for ( auto && ii : fAllMedia )
    {
        fProgressFuncs.incProgress();
        bool dataProcessed = processData( ii );
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

void CSyncSystem::forEachUser( std::function< void( std::shared_ptr< CUserData > media ) > onUser )
{
    for ( auto && ii : fUsers )
    {
        onUser( ii.second );
    }
}

void CSyncSystem::forEachMedia( std::function< void( std::shared_ptr< CMediaData > media ) > onMediaItem )
{
    for ( auto && ii : fAllMedia )
    {
        onMediaItem( ii );
    }
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
        if ( !ii->hasMissingInfo() || !ii->hasProviderIDs() )
            continue;

        fMissingMedia[ ii->name() ] = ii;
    }
    if ( checkForMissingMedia() )
        return;

    fProgressFuncs.setupProgress( "Finding Unplayed Info" );
    fProgressFuncs.setMaximum( static_cast<int>( fMissingMedia.size() ) );

    for ( auto && ii : fMissingMedia )
    {
        requestMediaInformation( ii.second, ii.second->isMissing( true ) );
    }
}

void CSyncSystem::requestUsersPlayedMedia( bool isLHSServer )
{
    if ( !fCurrUserData )
        return;

    auto && url = fSettings->getServerURL( isLHSServer );
    auto path = url.path();
    path += QString( "Users/%1/Items" ).arg( fCurrUserData->getUserID( isLHSServer ) );
    url.setPath( path );

    QUrlQuery query;

    query.addQueryItem( "api_key", isLHSServer ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    query.addQueryItem( "Filters", "IsPlayed" );
    query.addQueryItem( "IncludeItemTypes", "Movie,Episode,Video" );
    query.addQueryItem( "SortBy", "SortName" );
    query.addQueryItem( "SortOrder", "Ascending" );
    query.addQueryItem( "Recursive", "True" );
    url.setQuery( query );

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    emit sigAddToLog( QString( "Requesting Played media for '%1' from server '%2'" ).arg( fCurrUserData->name() ).arg( fSettings->getServerName( isLHSServer ) ) );

    auto reply = fManager->get( request );
    setIsLHS( reply, isLHSServer );
    setRequestType( reply, ERequestType::eMediaList );
    setExtraData( reply, fCurrUserData->name() );
}

bool CSyncSystem::processData( std::shared_ptr< CMediaData > mediaData )
{
    if ( !mediaData || mediaData->userDataEqual( true ) )
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
    bool lhsMoreRecent = mediaData->rhsLastPlayedOlder();
    requestUpdateUserDataForMedia( mediaData, lhsMoreRecent ? mediaData->lhsUserData() : mediaData->rhsUserData(), !lhsMoreRecent );
    return true;
}

void CSyncSystem::requestUpdateUserDataForMedia( std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData, bool sendToLHS )
{
    if ( !mediaData || !newData )
        return;

    if ( *mediaData->lhsUserData() == *newData )
        return;

    auto && url = fSettings->getServerURL( sendToLHS );
    auto && mediaID = mediaData->getMediaID( sendToLHS );
    auto && userID = fCurrUserData->getUserID( sendToLHS );

    auto obj = newData->toJSON();
    QByteArray data = QJsonDocument( obj ).toJson();

    //qDebug() << "userID" << userID;
    //qDebug() << "mediaID" << mediaID;

    auto path = url.path();
    path += QString( "Users/%1/Items/%2/UserData" ).arg( userID ).arg( mediaID );
    url.setPath( path );
    QUrlQuery query;

    query.addQueryItem( "api_key", sendToLHS ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    url.setQuery( query );

    //qDebug() << url;

    auto request = QNetworkRequest( url );
    request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );

    QNetworkReply * reply = nullptr;
    reply = fManager->post( request, data );

    setIsLHS( reply, sendToLHS );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateData );

    requestSetFavorite( mediaData, newData, sendToLHS );
}

//void CSyncSystem::setMediaData( std::shared_ptr< CMediaData > mediaData, bool deleteUpdate, const QString & updateType = "" )
void CSyncSystem::requestSetFavorite( std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData, bool sendToLHS )
{
    if ( mediaData->isFavorite( sendToLHS ) == newData->fIsFavorite )
        return;

    auto && url = fSettings->getServerURL( sendToLHS );
    auto && mediaID = mediaData->getMediaID( sendToLHS );
    auto && userID = fCurrUserData->getUserID( sendToLHS );

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
    query.addQueryItem( "api_key", sendToLHS ? fSettings->lhsAPI() : fSettings->rhsAPI() );
    url.setQuery( query );

    auto request = QNetworkRequest( url );

    QByteArray data;
    QNetworkReply * reply = nullptr;
    if ( newData->fIsFavorite )
        reply = fManager->post( request, data ); 
    else
        reply = fManager->deleteResource( request );

    setIsLHS( reply, sendToLHS );
    setExtraData( reply, mediaID );
    setRequestType( reply, ERequestType::eUpdateData );
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

    auto reply = fManager->get( request );
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

void CSyncSystem::decRequestCount( ERequestType requestType )
{
    auto pos = fRequests.find( requestType );
    if ( pos == fRequests.end() )
        return;
    if ( ( *pos ).second > 0 )
        ( *pos ).second--;

    if ( ( *pos ).second == 0 )
    {
        fRequests.erase( pos );
    }
}

void CSyncSystem::setRequestType( QNetworkReply * reply, ERequestType requestType )
{
    if ( !reply )
        return;
    fAttributes[ reply ][ kRequestType ] = static_cast<int>( requestType );
    fRequests[ requestType ]++;
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
                loadMediaData();
                break;
            case ERequestType::eNone:
            case ERequestType::eGetMediaInfo:
            case ERequestType::eMediaData:
            case ERequestType::eReloadMediaData:
            case ERequestType::eUpdateData:
            default:
                break;
        }

        postHandlRequest( requestType );
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
                    loadMediaData();
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
        case ERequestType::eMediaData:
            if ( !fProgressFuncs.wasCanceled() )
            {
                loadMediaData( data, isLHSServer, extraData.toString() );
                fProgressFuncs.incProgress();
                if ( fAttributes.empty() && !fLoadingMediaData )
                {
                    fLoadingMediaData = true;
                    QTimer::singleShot( 0, this, &CSyncSystem::slotMergeMedia );
                }
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
            break;
        }
    }

    postHandlRequest( requestType );
}

void CSyncSystem::loadMediaData( const QString & mediaID, bool isLHSServer )
{
    auto pos = isLHSServer ? fLHSMedia.find( mediaID ) : fRHSMedia.find( mediaID );
    if ( pos != ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
    {
        requestMediaData( ( *pos ).second, isLHSServer, true );
    }
}

void CSyncSystem::postHandlRequest( ERequestType requestType )
{
    decRequestCount( requestType );
    if ( fAttributes.empty() )
        fProgressFuncs.resetProgress();
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
    fProgressFuncs.setMaximum( 0 );


    mergeMediaData( fLHSMedia, fRHSMedia, true );
    mergeMediaData( fRHSMedia, fLHSMedia, false );

    for ( auto && ii : fLHSMedia )
        fAllMedia.insert( ii.second );

    for ( auto && ii : fRHSMedia )
        fAllMedia.insert( ii.second );

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

        auto userData = getUserData( name );
        if ( !userData )
        {
            userData = std::make_shared< CUserData >( name );
            fUsers[ userData->name() ] = userData;
        }

        userData->setUserID( id, isLHSServer );
        if ( fUpdateUserFunc )
            fUpdateUserFunc( userData );
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
    fProgressFuncs.setMaximum( mediaList.count() );

    emit sigAddToLog( QString( "%1 has %2 watched media items on server '%3' that match search name for '%4'- %5 remaining" ).arg( fCurrUserData->name() ).arg( mediaList.count() ).arg( fSettings->getServerName( isLHSServer ) ).arg( mediaName ).arg( fMissingMedia.size() ) );

    for ( auto && ii : mediaList )
    {
        auto media = ii.toObject();

        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        auto id = media[ "Id" ].toString();
        auto name = CMediaData::computeName( media );

        std::shared_ptr< CMediaData > mediaData;
        auto pos = fMissingMedia.find( name );
        //Q_ASSERT( pos != fMissingMedia.end() );
        if ( pos != fMissingMedia.end() )
        {
            mediaData = ( *pos ).second;
            fMissingMedia.erase( pos );
            if ( isLHSServer )
                mediaData->setMediaID( id, isLHSServer );
            else
                mediaData->setMediaID( id, !isLHSServer );
        }
        else
        {
            pos = isLHSServer ? fLHSMedia.find( id ) : fRHSMedia.find( id );
            if ( pos == ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
            {
                emit sigAddToLog( QString( "Error:  COULD NOT FIND MEDIA '%1' on %2 server" ).arg( mediaName ).arg( fSettings->getServerName( isLHSServer ) ) );
                continue;
            }
            mediaData = ( *pos ).second;
        }

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

        mediaData->loadUserDataFromJSON( media, isLHSServer );
        if ( fUpdateMediaFunc )
            fUpdateMediaFunc( mediaData );
        break;
    }
    checkForMissingMedia();
}

bool CSyncSystem::checkForMissingMedia()
{
    if ( fMissingMedia.empty() )
        emit sigUserMediaCompletelyLoaded();
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

    auto reply = fManager->get( request );
    setIsLHS( reply, forLHS );
    setExtraData( reply, mediaData->name() );
    setRequestType( reply, ERequestType::eGetMediaInfo );
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
    fProgressFuncs.resetProgress();
    fProgressFuncs.setupProgress( tr( "Loading Users Played Media Data" ) );
    fProgressFuncs.setMaximum( mediaList.count() );

    emit sigAddToLog( QString( "%1 has %2 watched media items on server '%3'" ).arg( fCurrUserData->name() ).arg( mediaList.count() ).arg( fSettings->getServerName( isLHSServer ) ) );

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

        mediaData->loadUserDataFromJSON( media, isLHSServer );
        fCurrUserData->addPlayedMedia( mediaData );
        if ( isLHSServer )
            fLHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;
        else
            fRHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;
        fProgressFuncs.incProgress();
    }
    fProgressFuncs.resetProgress();
}

void CSyncSystem::loadMediaData()
{
    if ( !fCurrUserData )
        return;

    if ( !fCurrUserData->hasMedia() )
        return;

    auto playedMedia = fCurrUserData->playedMedia();
    fProgressFuncs.resetProgress();
    fProgressFuncs.setupProgress( tr( "Requesting Media Provider Details" ) );
    fProgressFuncs.setMaximum( static_cast< int >( playedMedia.size() ) );
    for ( auto && ii : playedMedia )
    {
        requestMediaData( ii, true, false );
        requestMediaData( ii, false, false );
        fProgressFuncs.incProgress();
    }

    fProgressFuncs.resetProgress();
    fProgressFuncs.setupProgress( tr( "Loading Media Provider Info" ) );
    fProgressFuncs.setMaximum( fCurrUserData->numPlayedMedia() );
}

void CSyncSystem::requestMediaData( std::shared_ptr< CMediaData > mediaData, bool isLHSServer, bool reload )
{
    if ( !reload )
    {
        if ( mediaData->beenLoaded( isLHSServer ) )
            return;

        if ( mediaData->isMissing( isLHSServer ) )
            return;
    }
    auto && url = fSettings->getServerURL( isLHSServer );
    auto path = url.path();
    path += QString( "Users/%1/Items/%2" ).arg( fCurrUserData->getUserID( isLHSServer ) ).arg( mediaData->getMediaID( isLHSServer ) );
    url.setPath( path );

    //qDebug() << url;
    auto request = QNetworkRequest( url );

    auto reply = fManager->get( request );

    setIsLHS( reply, isLHSServer );
    setRequestType( reply, reload ? ERequestType::eReloadMediaData : ERequestType::eMediaData );
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

    mediaData->loadUserDataFromJSON( media, isLHSServer );
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
    QStringList providers;
    for ( auto && ii = providerIDsObj.begin(); ii != providerIDsObj.end(); ++ii )
    {
        auto providerName = ii.key();
        auto providerID = ii.value().toString();
        mediaData->addProvider( providerName, providerID );
        providers << providerName + "=" + providerID;
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
        for ( auto && jj : mediaData->getProviders() )
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
    auto value = ( *pos ).second;
    return value <= 1;
}

