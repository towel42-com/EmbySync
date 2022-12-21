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
#include "ServerModel.h"
#include "MediaModel.h"
#include "SyncSystem.h"
#include "SABUtils/StringUtils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QObject>
#include <QVariant>

#include <chrono>
#include <optional>
#include <QDebug>

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

CMediaData::CMediaData( const QJsonObject & mediaObj, std::shared_ptr< CServerModel > serverModel )
{
    computeName( mediaObj );
    fType = mediaObj[ "Type" ].toString();

    for ( auto && serverInfo : *serverModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;
        fInfoForServer[ serverInfo->keyName() ] = std::make_shared< SMediaServerData >();
    }
}

CMediaData::CMediaData( const QString & name, int year, const QString & type )
{
    fName = name;
    fType = type;
    fPremiereDate = QDate( year, 1, 1 );
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

QString CMediaData::seriesName() const
{
    return fSeriesName;
}

QString CMediaData::mediaType() const
{
    return fType;
}

void CMediaData::computeName( const QJsonObject & media )
{
    auto name = fName = media[ "Name" ].toString();
    if ( media[ "Type" ] == "Episode" )
    {
        //auto tmp = QJsonDocument( media );
        //qDebug() << tmp.toJson();

        fSeriesName = media[ "SeriesName" ].toString();
        auto season = media[ "SeasonName" ].toString();
        auto pos = season.lastIndexOf( ' ' );
        bool aOK = false;
        if ( pos != -1 )
        {
            fSeason = season.mid( pos + 1 ).toInt( &aOK );
            if ( aOK )
                season = QString( "S%1" ).arg( fSeason.value(), 2, 10, QChar( '0' ) );
        }
        if ( !aOK )
        {
            season.clear();
            fSeason.reset();
        }

        fEpisode = media[ "IndexNumber" ].toInt();
        if ( fEpisode.value() == 0 )
            fEpisode.reset();

        auto episode = fEpisode.has_value() ? QString( "E%1" ).arg( fEpisode.value(), 2, 10, QChar( '0' ) ) : QString();
        fName = QString( "%1 - %2%3" ).arg( fSeriesName ).arg( season ).arg( episode );
        auto episodeName = media[ "EpisodeTitle" ].toString();
        if ( !episodeName.isEmpty() )
            fName += QString( " - %1" ).arg( episodeName );
        if ( !name.isEmpty() )
            fName += QString( " - %1" ).arg( name );
    }
}

void CMediaData::loadData( const QString & serverName, const QJsonObject & media )
{
    //qDebug().noquote().nospace() << QJsonDocument( media ).toJson( QJsonDocument::Indented );

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

    fPremiereDate = media[ "PremiereDate" ].toVariant().toDate();
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

QUrl CMediaData::getSearchURL() const
{
    QUrl retVal( "https://rarbg.to/torrents.php" );
    QString searchKey;

    auto pos = fProviders.find( "imdb" );
    if ( pos != fProviders.end() )
    {
        searchKey = ( *pos ).second;
    }
    if ( searchKey.isEmpty() )
        searchKey = fName;

    if ( this->mediaType() == "Episode" )
    {
        searchKey = fSeriesName;
        if ( fSeason.has_value() )
            searchKey += " " + QString( "S%1" ).arg( fSeason.value(), 2, 10, QChar( '0' ) );

        if ( fEpisode.has_value() )
            searchKey += " " + QString( "E%1" ).arg( fEpisode.value(), 2, 10, QChar( '0' ) );
    }
    if ( fPremiereDate.isValid() )
    {
        searchKey += " " + QString::number( fPremiereDate.year() );
    }
    QUrlQuery query;
    query.addQueryItem( "search", searchKey.replace( " ", "+" ) );
    retVal.setQuery( query );
    return retVal;
}

QJsonObject CMediaData::toJson( bool includeSearchURL )
{
    QJsonObject retVal;

    retVal[ "type" ] = fType;
    retVal[ "name" ] = fName;
    if ( !fSeriesName.isEmpty() )
        retVal[ "seriesname" ] = fSeriesName;
    if ( fSeason.has_value() )
        retVal[ "season" ] = fSeason.value();
    if ( fEpisode.has_value() )
        retVal[ "season" ] = fEpisode.value();
    retVal[ "premiere_date" ] = fPremiereDate.toString( "MM/dd/yyyy" );

    QJsonArray serverInfos;
    for ( auto && ii : fInfoForServer )
    {
        if ( !ii.second->isValid() )
            continue;;
        auto serverInfo = ii.second->toJson();
        serverInfo[ "server_url" ] = ii.first;
        serverInfos.push_back( serverInfo );
    }
    retVal[ "server_infos" ] = serverInfos;

    if ( includeSearchURL )
        retVal[ "searchurl" ] = getSearchURL().toString();

    return retVal;
}

bool CMediaData::onServer() const
{
    return !this->fInfoForServer.empty();
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

bool CMediaData::isMatch( const QString & name, int year ) const
{
    bool isMatch = ( ( year >= premiereDate().year() - 1 ) && ( year <= premiereDate().year() + 1 ) );

    if ( !isMatch )
        return false;

    if ( name.toLower().trimmed() == fName.toLower().trimmed() )
        return true;
    if ( NSABUtils::NStringUtils::isSimilar( fName, name, true ) )
        return true;
    return false;
}

CMediaCollection::CMediaCollection( const QString & serverName, const QString & name, const QString & id, int pos ) :
    fName( name ),
    fServerName( serverName ),
    fPosition( pos )
{
    fCollectionInfo = std::make_shared< SCollectionServerInfo >( id );
    fCollectionInfo->setId( id );
}

bool SCollectionServerInfo::missingMedia() const
{
    for ( auto && ii : fItems )
    {
        if ( !ii->fData || !ii->fData->onServer() )
            return true;
    }
    return false;
}

int SCollectionServerInfo::numMissing() const
{
    int retVal = 0;
    for ( auto && ii : fItems )
    {
        if ( !ii->fData || !ii->fData->onServer() )
            ++retVal;
    }
    return retVal;
}



void SCollectionServerInfo::createCollection( std::shared_ptr<const CServerInfo> serverInfo, const QString & collectionName, std::shared_ptr< CSyncSystem > syncSystem )
{
    if ( collectionExists() )
        return;

    std::list< std::shared_ptr< CMediaData > > media;
    for ( auto && ii : fItems )
    {
        if ( ii->fData )
            media.emplace_back( ii->fData );
    }

    syncSystem->createCollection( serverInfo, collectionName, media );
}

QVariant CMediaCollection::data( int column, int role ) const
{
    if ( role == Qt::DisplayRole )
    {
        switch ( column )
        {
            case 0: return fName;
            case 1: return {};
            case 2: return fCollectionInfo->collectionExists() ? QObject::tr( "Yes" ) : QString( "No" );
            case 3: return missingMedia() ? QObject::tr( "Yes" ) : QString();
        }
    }
    return {};
}

std::shared_ptr< SMediaCollectionData > CMediaCollection::addMovie( int rank, const QString & name, int year )
{
    return fCollectionInfo->addMovie( rank, name, year, this );
}

void CMediaCollection::setItems( const std::list< std::shared_ptr< CMediaData > > & items )
{
    fCollectionInfo->fItems.clear();
    fCollectionInfo->fItems.reserve( items.size() );
    for ( auto && ii : items )
    {
        auto curr = std::make_shared< SMediaCollectionData >( ii, this );
        fCollectionInfo->fItems.push_back( curr );
    }
}

bool CMediaCollection::updateWithRealCollection( std::shared_ptr< CMediaCollection > realCollection )
{
    (void)realCollection;
    return false;
}

SCollectionServerInfo::SCollectionServerInfo( const QString & id ) :
    fCollectionID( id )
{

}

bool SCollectionServerInfo::updateMedia( std::shared_ptr< CMediaModel > mediaModel )
{
    bool retVal = false;
    for ( auto && ii : fItems )
    {
        if ( ii->fData && !ii->fData->onServer() )
        {
            retVal = ii->updateMedia( mediaModel ) || retVal;
        }
    }
    return retVal;
}

std::shared_ptr< SMediaCollectionData > SCollectionServerInfo::addMovie( int rank, const QString & name, int year, CMediaCollection * parent )
{
    auto retVal = std::make_shared< SMediaCollectionData >( std::make_shared< CMediaData >( name, year, "Movie" ), parent );
    if ( ( rank - 1 ) >= fItems.size() )
    {
        fItems.resize( rank );
    }
    fItems[ rank - 1 ] = retVal;
    for ( size_t ii = 0; ii < fItems.size(); ++ii )
    {
        if ( !fItems[ ii ] )
        {
            auto tmp = std::make_shared < SMediaCollectionData >( nullptr, parent );
            fItems[ ii ] = tmp;
        }

    }
    return retVal;
}

QVariant SMediaCollectionData::data( int column, int role ) const
{
    if ( !fData )
        return {};

    if ( role == Qt::DisplayRole )
    {
        switch ( column )
        {
            case 0: return fData->name();
            case 1: return fData->premiereDate().year();
            case 3: return fData->onServer() ? QString() : QObject::tr( "Yes" );
        }
    }
    return {};
}

bool SMediaCollectionData::updateMedia( std::shared_ptr< CMediaModel > mediaModel )
{
    if ( fData->onServer() )
        return false;

    auto data = mediaModel->findMedia( fData->name(), fData->premiereDate().year() );
    if ( data )
    {
        fData = data;
        return true;
    }
    return false;
}
