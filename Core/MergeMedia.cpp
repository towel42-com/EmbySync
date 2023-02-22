#include "MergeMedia.h"
#include "MediaData.h"
#include "ProgressSystem.h"

#include <QString>

#include <map>
#include <optional>

#include <QDebug>

using TMediaIDToMediaData = std::map< QString, std::shared_ptr< CMediaData > >;

void CMergeMedia::addMediaInfo( const QString &serverName, std::shared_ptr< CMediaData > mediaData )
{
    fMediaMap[ serverName ][ mediaData->getMediaID( serverName ) ] = mediaData;

    auto &&providers = mediaData->getProviders( true );
    for ( auto &&ii : providers )
    {
        fProviderSearchMap[ serverName ][ ii.first ][ ii.second ] = mediaData;
    }
}

void CMergeMedia::removeMedia( const QString &serverName, const std::shared_ptr< CMediaData > &mediaData )
{
    auto pos = fMediaMap.find( serverName );
    if ( pos != fMediaMap.end() )
    {
        auto pos2 = ( *pos ).second.find( mediaData->getMediaID( serverName ) );
        ( *pos ).second.erase( pos2 );
    }

    auto &&providers = mediaData->getProviders( true );
    for ( auto &&ii : providers )
    {
        auto pos = fProviderSearchMap.find( serverName );
        if ( pos != fProviderSearchMap.end() )
        {
            auto pos2 = ( *pos ).second.find( ii.first );
            if ( pos2 != ( *pos ).second.end() )
            {
                auto pos3 = ( *pos2 ).second.find( ii.second );
                if ( pos3 != ( *pos2 ).second.end() )
                    ( *pos2 ).second.erase( pos3 );
                if ( ( *pos2 ).second.empty() )
                {
                    ( *pos ).second.erase( pos2 );
                }
            }
            if ( ( *pos ).second.empty() )
            {
                fProviderSearchMap.erase( serverName );
            }
        }
    }
}

bool CMergeMedia::merge( std::shared_ptr< CProgressSystem > progressSystem )
{
    progressSystem->resetProgress();
    progressSystem->setTitle( QObject::tr( "Merging media data" ) );
    size_t total = 0;
    for ( auto &&ii : fMediaMap )
    {
        total += ii.second.size();
    }
    progressSystem->setMaximum( static_cast< int >( total * 3 ) );

    for ( auto &&ii = fMediaMap.begin(); ii != fMediaMap.end(); ++ii )
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

void CMergeMedia::merge( std::pair< const QString, TMediaIDToMediaData > &lhs, std::pair< const QString, TMediaIDToMediaData > &rhs, std::shared_ptr< CProgressSystem > progressSystem )
{
    // qDebug() << lhs.first << rhs.first;

    merge( lhs, progressSystem );
    merge( rhs, progressSystem );
}

void CMergeMedia::merge( std::pair< const QString, TMediaIDToMediaData > &mapData, std::shared_ptr< CProgressSystem > progressSystem )
{
    // qDebug() << mapData.first;
    std::unordered_map< std::shared_ptr< CMediaData >, std::shared_ptr< CMediaData > > replacementMap;
    for ( auto &&ii : mapData.second )
    {
        if ( progressSystem->wasCanceled() )
            break;

        progressSystem->incProgress();
        auto mediaData = ii.second;
        if ( !mediaData )
            continue;

        auto mediaProviders = mediaData->getProviders( true );
        auto myMappedMedia = findMediaForProviders( mapData.first, mediaProviders );
        if ( myMappedMedia && ( myMappedMedia != mediaData ) )
        {
            replacementMap[ mediaData ] = myMappedMedia;
            continue;
        }

        for ( auto &&ii : fProviderSearchMap )
        {
            if ( ii.first == mapData.first )
                continue;

            auto otherData = findMediaForProviders( ii.first, mediaProviders );
            if ( otherData != mediaData )
                setMediaForProviders( ii.first, mediaProviders, mediaData );
        }
    }
    for ( auto &&ii : replacementMap )
    {
        auto currMediaID = ii.first->getMediaID( mapData.first );
        auto pos = mapData.second.find( currMediaID );
        mapData.second.erase( pos );

        ii.second->updateFromOther( mapData.first, ii.first );
        mapData.second[ currMediaID ] = ii.second;
    }
}

QStringList CMergeMedia::getOtherServers( const QString &serverName ) const
{
    QStringList otherServers;
    for ( auto &&ii : fProviderSearchMap )
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

    for ( auto &&ii : fMediaMap )
    {
        for ( auto &&jj : ii.second )
        {
            allMedia.insert( jj.second );
            progressSystem->incProgress();
        }
    }

    return { allMedia, fMediaMap };
}

std::shared_ptr< CMediaData > CMergeMedia::findMediaForProviders( const QString &serverName, const std::map< QString, QString > &providerIDs ) const
{
    auto pos = fProviderSearchMap.find( serverName );
    if ( pos == fProviderSearchMap.end() )
        return {};

    std::map< std::shared_ptr< CMediaData >, int > mapCount;

    for ( auto &&ii : providerIDs )
    {
        if ( ii.second.isEmpty() )
            continue;
        auto currMedia = findMediaForProvider( ( *pos ).second, ii.first, ii.second );
        if ( !currMedia )
            continue;

        mapCount[ currMedia ]++;
    }

    if ( mapCount.size() == 1 )
        return ( *mapCount.begin() ).first;

    int max = 0;
    std::shared_ptr< CMediaData > retVal;
    for ( auto &&ii : mapCount )
    {
        if ( ii.second > max )
        {
            max = ii.second;
            retVal = ii.first;
        }
    }
    return retVal;
}

std::shared_ptr< CMediaData > CMergeMedia::findMediaForProvider( const std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > > &map, const QString &provider, const QString &id ) const
{
    auto pos = map.find( provider );
    if ( pos == map.end() )
        return {};

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == ( *pos ).second.end() )
        return {};
    return ( *pos2 ).second;
}

void CMergeMedia::setMediaForProviders( const QString &serverName, const std::map< QString, QString > &providerIDs, std::shared_ptr< CMediaData > mediaData )
{
    auto pos = fProviderSearchMap.find( serverName );
    if ( pos == fProviderSearchMap.end() )
        pos = fProviderSearchMap.insert( std::make_pair( serverName, std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > >() ) ).first;

    for ( auto &&ii : providerIDs )
    {
        if ( ii.second.isEmpty() )
            continue;
        ( *pos ).second[ ii.first ][ ii.second ] = mediaData;
    }
}
