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

#include "MediaData.h"
#include "MediaServerData.h"
#include "Settings.h"
#include "ServerInfo.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QObject>
#include <QVariant>

#include <chrono>
#include <optional>

QStringList CMediaData::getHeaderLabels()
{
    return QStringList()
        << QObject::tr( "Name" )
        << QObject::tr( "ID" )
        << QObject::tr( "Is Favorite?" )
        << QObject::tr( "Played?" )
        << QObject::tr( "Last Played" )
        << QObject::tr( "Play Count" )
        << QObject::tr( "Play Position" )
        ;
}

std::function< QString( uint64_t ) > CMediaData::sMSecsToStringFunc;
void CMediaData::setMSecsToStringFunc( std::function< QString( uint64_t ) > func )
{
    sMSecsToStringFunc = func;
}

std::function< QString( uint64_t ) > CMediaData::mecsToStringFunc()
{
    return sMSecsToStringFunc;
}

CMediaData::CMediaData( const QJsonObject & mediaObj, std::shared_ptr< CSettings > settings )
{
    fName = computeName( mediaObj );
    fType = mediaObj[ "Type" ].toString();
    for ( int ii = 0; ii < settings->serverCnt(); ++ii )
    {
        auto serverInfo = settings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;
        fInfoForServer[ serverInfo->keyName() ] = std::make_shared< SMediaServerData >();
    }
}

std::shared_ptr<SMediaServerData> CMediaData::userMediaData( const QString & serverName ) const
{
    auto pos = fInfoForServer.find( serverName );
    if ( pos != fInfoForServer.end() )
        return ( *pos ).second;
    return {};
}

QString CMediaData::name() const
{
    return fName;
}

QString CMediaData::mediaType() const
{
    return fType;
}

QString CMediaData::computeName( const QJsonObject & media )
{
    auto name = media[ "Name" ].toString();
    QString retVal = name;
    if ( media[ "Type" ] == "Episode" )
    {
        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        auto series = media[ "SeriesName" ].toString();
        auto season = media[ "SeasonName" ].toString();
        auto pos = season.lastIndexOf( ' ' );
        if ( pos != -1 )
        {
            bool aOK;
            auto seasonNum = season.mid( pos + 1 ).toInt( &aOK );
            if ( aOK )
                season = QString( "S%1" ).arg( seasonNum, 2, 10, QChar( '0' ) );
        }

        auto episodeNum = media[ "IndexNumber" ].toInt();
        retVal = QString( "%1 - %2E%3" ).arg( series ).arg( season ).arg( episodeNum, 2, 10, QChar( '0' ) );
        auto episodeName = media[ "EpisodeTitle" ].toString();
        if ( !episodeName.isEmpty() )
            retVal += QString( " - %1" ).arg( episodeName );
        if ( !name.isEmpty() )
            retVal += QString( " - %1" ).arg( name );
    }
    return retVal;
}

void CMediaData::loadUserDataFromJSON( const QString & serverName, const QJsonObject & media )
{
    //auto tmp = QJsonDocument( media );
    //qDebug() << tmp.toJson();

    auto externalUrls = media[ "ExternalUrls" ].toArray();
    for ( auto && ii : externalUrls )
    {
        auto urlObj = ii.toObject();
        auto name = urlObj[ "Name" ].toString();
        auto url = urlObj[ "Url" ].toString();
        fExternalUrls[ name ] = url;
    }

    auto userDataObj = media[ "UserData" ].toObject();

    //auto tmp = QJsonDocument( userDataObj );
    //qDebug() << tmp.toJson();

    auto mediaData = userMediaData( serverName );
    mediaData->loadUserDataFromJSON( userDataObj );

    auto providerIDsObj = media[ "ProviderIds" ].toObject();
    for ( auto && ii = providerIDsObj.begin(); ii != providerIDsObj.end(); ++ii )
    {
        auto providerName = ii.key();
        auto providerID = ii.value().toString();
        addProvider( providerName, providerID );
    }
}

QString CMediaData::externalUrlsText() const
{
    auto retVal = QString( "External Urls:</br>\n<ul>\n%1\n</ul>" );

    QStringList externalUrls;
    for ( auto && ii : fExternalUrls )
    {
        auto curr = QString( R"(<li>%1 - <a href="%2">%2</a></li>)" ).arg( ii.first ).arg( ii.second );
        externalUrls << curr;
    }
    retVal = retVal.arg( externalUrls.join( "\n" ) );
    return retVal;
}

bool CMediaData::hasProviderIDs() const
{
    return !fProviders.empty();
}

QString CMediaData::getProviderList() const
{
    QStringList retVal;
    for ( auto && ii : fProviders )
    {
        retVal << ii.first.toLower() + "." + ii.second.toLower();
    }
    return retVal.join( "," );
}

bool CMediaData::isPlayed( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return false;
    return mediaData->fPlayed;
}

uint64_t CMediaData::playCount( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return 0;
    return mediaData->fPlayCount;
}

bool CMediaData::allPlayCountEqual() const
{
    return allEqual< uint64_t >( []( std::shared_ptr< SMediaServerData > data )
                     {
                         return data->fPlayCount;
                     } );
}

bool CMediaData::isFavorite( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return false;
    return mediaData->fIsFavorite;
}

bool CMediaData::allFavoriteEqual() const
{
    return allEqual< bool >( []( std::shared_ptr< SMediaServerData > data )
                                 {
                                     return data->fIsFavorite;
                                 } );
}

QDateTime CMediaData::lastPlayed( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return {};
    return mediaData->fLastPlayedDate;
}

bool CMediaData::allLastPlayedEqual() const
{
    return allEqual< QDateTime >( []( std::shared_ptr< SMediaServerData > data )
                             {
                                 return data->fLastPlayedDate;
                             } );
}

// 1 tick = 10000 ms
uint64_t CMediaData::playbackPositionTicks( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return 0;
    return mediaData->fPlaybackPositionTicks;
}

// stored in ticks
// 1 tick = 10000 ms
uint64_t CMediaData::playbackPositionMSecs( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return 0;
    return mediaData->playbackPositionMSecs();
}

QString CMediaData::playbackPosition( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return {};
    return mediaData->playbackPosition();
}

QTime CMediaData::playbackPositionTime( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return {};
    return mediaData->playbackPositionTime();
}


bool CMediaData::allPlayedEqual() const
{
    return allEqual< bool >( []( std::shared_ptr< SMediaServerData > data )
                                  {
                                      return data->fPlayed;
                                  } );
}

bool CMediaData::allPlaybackPositionTicksEqual() const
{
    return allEqual< int64_t >( []( std::shared_ptr< SMediaServerData > data )
                             {
                                 return data->fPlaybackPositionTicks;
                             } );
}

QUrlQuery CMediaData::getSearchForMediaQuery() const
{
    QUrlQuery query;

    query.addQueryItem( "IncludeItemTypes", "Movie,Episode,Video" );
    query.addQueryItem( "AnyProviderIdEquals", getProviderList() );
    query.addQueryItem( "SortBy", "SortName" );
    query.addQueryItem( "SortOrder", "Ascending" );
    query.addQueryItem( "Recursive", "True" );

    return query;
}

QString CMediaData::getProviderID( const QString & provider )
{
    auto pos = fProviders.find( provider );
    if ( pos == fProviders.end() )
        return {};
    return ( *pos ).second;
}

std::map< QString, QString > CMediaData::getProviders( bool addKeyIfEmpty /*= false */ ) const
{
    auto retVal = fProviders;
    if ( addKeyIfEmpty && retVal.empty() )
    {
        retVal[ fType ] = fName;
    }

    return std::move( retVal );
}

void CMediaData::addProvider( const QString & providerName, const QString & providerID )
{
    fProviders[ providerName ] = providerID;
}

void CMediaData::setMediaID( const QString & serverName, const QString & mediaID )
{
    auto mediaData = userMediaData( serverName );
    mediaData->fMediaID = mediaID;
    updateCanBeSynced();
}

void CMediaData::updateCanBeSynced()
{
    int serverCnt = 0;
    for ( auto && ii : fInfoForServer )
    {
        if ( ii.second->isValid() )
            serverCnt++;
    }
    fCanBeSynced = serverCnt > 1;
}

QString CMediaData::getMediaID( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return {};
    return mediaData->fMediaID;
}

bool CMediaData::beenLoaded( const QString & serverName ) const
{
    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return {};
    return mediaData->fBeenLoaded;
}

QIcon CMediaData::getDirectionIcon( const QString & serverName ) const
{
    static QIcon sErrorIcon( ":/resources/error.png" );
    static QIcon sEqualIcon( ":/resources/equal.png" );
    static QIcon sArrowUpIcon( ":/resources/arrowup.png" );
    static QIcon sArrowDownIcon( ":/resources/arrowdown.png" );
    QIcon retVal;
    if ( !isValidForServer( serverName ) )
        retVal = sErrorIcon;
    else if ( !canBeSynced() )
        return {};
    else if ( validUserDataEqual() )
        retVal = sEqualIcon;
    else if ( needsUpdating( serverName ) )
        retVal = sArrowDownIcon;
    else 
        retVal = sArrowUpIcon;

    return retVal;
}

void CMediaData::updateFromOther( const QString & otherServerName, std::shared_ptr< CMediaData > other )
{
    auto otherMediaData = other->userMediaData( otherServerName );
    if ( !otherMediaData )
        return;

    fInfoForServer[ otherServerName ] = otherMediaData;
    updateCanBeSynced();
}


bool CMediaData::needsUpdating( const QString & serverName ) const
{
    // TODO: When Emby supports last modified use that

    auto mediaData = userMediaData( serverName );
    if ( !mediaData )
        return false;
    return ( mediaData != newestMediaData() );
}

std::shared_ptr<SMediaServerData> CMediaData::newestMediaData() const
{
    std::shared_ptr<SMediaServerData> retVal;
    for ( auto && ii : fInfoForServer )
    {
        if ( !ii.second->isValid() )
            continue;

        if ( !retVal )
            retVal = ii.second;
        else
        {
            if ( ii.second->fLastPlayedDate > retVal->fLastPlayedDate )
                retVal = ii.second;
        }
    }
    return retVal;
}

bool CMediaData::isValidForServer( const QString & serverName ) const
{
    auto mediaInfo = userMediaData( serverName );
    if ( !mediaInfo )
        return false;
    return mediaInfo->isValid();
}

bool CMediaData::isValidForAllServers() const
{
    for ( auto && ii : fInfoForServer )
    {
        if ( !ii.second->isValid() )
            return false;
    }
    return true;
}

bool CMediaData::canBeSynced() const
{
    return fCanBeSynced;
}

EMediaSyncStatus CMediaData::syncStatus() const
{
    if ( !canBeSynced() )
        return EMediaSyncStatus::eNoServerPairs;

    if ( validUserDataEqual() )
        return EMediaSyncStatus::eMediaEqualOnValidServers;
    return EMediaSyncStatus::eMediaNeedsUpdating;
}

bool CMediaData::validUserDataEqual() const
{
    std::list< std::pair< QString, std::shared_ptr< SMediaServerData > > > validServerData;
    for ( auto && ii : fInfoForServer )
    {
        if ( ii.second->isValid() )
            validServerData.push_back( ii );
    }

    auto pos = validServerData.begin();
    auto nextPos = validServerData.begin();
    nextPos++;
    for ( ; ( pos != validServerData.end() ) && ( nextPos != validServerData.end() ); ++pos, ++nextPos )
    {
        if ( !( *pos ).second->userDataEqual( *( ( *nextPos ).second ) ) )
            return false;
    }
    return true;
}
