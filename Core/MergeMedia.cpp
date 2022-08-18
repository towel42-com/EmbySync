#include "MergeMedia.h"
#include "MediaData.h"
#include "ProgressSystem.h"

#include <QString>

#include <map>
#include <optional>

#include <QDebug>

using TMediaIDToMediaData = std::map< QString, std::shared_ptr< CMediaData > >;

void CMergeMedia::addMediaInfo( std::shared_ptr<CMediaData> mediaData, const QString & serverName )
{
    fMediaMap[ serverName ][ mediaData->getMediaID( serverName ) ] = mediaData;

    auto && providers = mediaData->getProviders( true );
    for ( auto && ii : providers )
    {
        fProviderSearchMap[ serverName ][ ii.first ][ ii.second ] = mediaData;
    }
}

bool CMergeMedia::merge( std::shared_ptr< CProgressSystem > progressSystem )
{
    progressSystem->resetProgress();
    progressSystem->setTitle( QObject::tr( "Merging media data" ) );
    size_t total = 0;
    for ( auto && ii : fMediaMap )
    {
        total += ii.second.size();
    }
    progressSystem->setMaximum( static_cast<int>( total * 3 ) );

    for ( auto && ii = fMediaMap.begin(); ii != fMediaMap.end(); ++ii )
    {
        if ( progressSystem->wasCanceled() )
            break;

        for ( auto jj = ii; jj != fMediaMap.end(); ++jj )
        {
            if ( ii == jj )
                continue;

            merge( ( *ii ), ( *jj ), progressSystem );
            if ( progressSystem->wasCanceled() )
                break;

            merge( ( *jj ), ( *ii ), progressSystem );
            if ( progressSystem->wasCanceled() )
                break;
        }
    }

    fProviderSearchMap.clear();

    if ( progressSystem->wasCanceled() )
        clear();
    return !progressSystem->wasCanceled();
}

void CMergeMedia::merge( std::pair< const QString, TMediaIDToMediaData > & lhs, std::pair< const QString, TMediaIDToMediaData > & rhs, std::shared_ptr< CProgressSystem > progressSystem )
{
    //qDebug() << lhs.first << rhs.first;

    merge( lhs, progressSystem );
    merge( rhs, progressSystem );
}

void CMergeMedia::merge( std::pair< const QString, TMediaIDToMediaData > & mapData, std::shared_ptr< CProgressSystem > progressSystem )
{
    //qDebug() << mapData.first;
    std::unordered_map< std::shared_ptr< CMediaData >, std::shared_ptr< CMediaData > > replacementMap;
    for ( auto && ii : mapData.second )
    {
        if ( progressSystem->wasCanceled() )
            break;

        progressSystem->incProgress();
        auto mediaData = ii.second;
        if ( !mediaData )
            continue;

        auto mediaProviders = mediaData->getProviders( true );
        auto myMappedMedia = findMediaForProviders( mediaProviders, mapData.first );
        if ( myMappedMedia && ( myMappedMedia != mediaData ) )
        {
            replacementMap[ mediaData ] = myMappedMedia;
            continue;
        }

        for ( auto && ii : fProviderSearchMap )
        {
            if ( ii.first == mapData.first )
                continue;

            auto otherData = findMediaForProviders( mediaProviders, ii.first );
            if ( otherData != mediaData )
                setMediaForProviders( mediaProviders, mediaData, ii.first );
        }
    }
    for ( auto && ii : replacementMap )
    {
        auto currMediaID = ii.first->getMediaID( mapData.first );
        auto pos = mapData.second.find( currMediaID );
        mapData.second.erase( pos );

        ii.second->updateFromOther( ii.first, mapData.first );
        mapData.second[ currMediaID ] = ii.second;
    }
}

QStringList CMergeMedia::getOtherServers( const QString & serverName ) const
{
    QStringList otherServers;
    for ( auto && ii : fProviderSearchMap )
    {
        if ( ii.first != serverName )
            otherServers << ii.first;
    }
    return otherServers;
}

void CMergeMedia::clear()
{
    fMediaMap.clear();
    fProviderSearchMap.clear();
}

std::pair< std::unordered_set< std::shared_ptr< CMediaData > >, std::map< QString, TMediaIDToMediaData > > CMergeMedia::getMergedData( std::shared_ptr< CProgressSystem > progressSystem ) const
{
    std::unordered_set< std::shared_ptr< CMediaData > > allMedia;

    for ( auto && ii : fMediaMap )
    {
        for ( auto && jj : ii.second )
        {
            allMedia.insert( jj.second );
            progressSystem->incProgress();
        }
    }

    return { allMedia, fMediaMap };
}

std::shared_ptr< CMediaData > CMergeMedia::findMediaForProviders( const std::map< QString, QString > & providerIDs, const QString & serverName ) const
{
    auto pos = fProviderSearchMap.find( serverName );
    if ( pos == fProviderSearchMap.end() )
        return {};

    std::optional< std::shared_ptr< CMediaData > > retVal;
    for ( auto && ii : providerIDs )
    {
        if ( ii.second.isEmpty() )
            continue;
        auto currMedia = findMediaForProvider( ( *pos ).second, ii.first, ii.second );
        if ( !currMedia )
            continue;
        if ( !retVal.has_value() )
            retVal = currMedia;
        else if ( retVal.value() != currMedia )
            return {};
    }
    if ( retVal.has_value() )
        return retVal.value();
    return {};
}


std::shared_ptr<CMediaData> CMergeMedia::findMediaForProvider( const std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > > & map, const QString & provider, const QString & id ) const
{
    auto pos = map.find( provider );
    if ( pos == map.end() )
        return {};

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == ( *pos ).second.end() )
        return {};
    return ( *pos2 ).second;
}

void CMergeMedia::setMediaForProviders( const std::map< QString, QString > & providerIDs, std::shared_ptr< CMediaData > mediaData, const QString & serverName )
{
    auto pos = fProviderSearchMap.find( serverName );
    if ( pos == fProviderSearchMap.end() )
        pos = fProviderSearchMap.insert( std::make_pair( serverName, std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > >() ) ).first;

    for ( auto && ii : providerIDs )
    {
        if ( ii.second.isEmpty() )
            continue;
        ( *pos ).second[ ii.first ][ ii.second ] = mediaData;
    }
}
