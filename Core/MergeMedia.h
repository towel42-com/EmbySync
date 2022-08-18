#ifndef __MERGEMEDIA_H
#define __MERGEMEDIA_H

#include <QString>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>

class CMediaData;
using TMediaIDToMediaData = std::map< QString, std::shared_ptr< CMediaData > >;

class CProgressSystem;
class CMergeMedia
{
public:
    CMergeMedia() {};
    ~CMergeMedia() {};

    void addMediaInfo( std::shared_ptr<CMediaData> mediaData, const QString & serverName );

    bool merge( std::shared_ptr< CProgressSystem > progressSystem );
    void clear();

    std::pair< std::unordered_set< std::shared_ptr< CMediaData > >, std::map< QString, TMediaIDToMediaData > > getMergedData( std::shared_ptr< CProgressSystem > progressSystem ) const;
private:
    void merge( std::pair< const QString, TMediaIDToMediaData > & lhs, std::pair< const QString, TMediaIDToMediaData > & rhs, std::shared_ptr< CProgressSystem > progressSystem );
    void merge( std::pair< const QString, TMediaIDToMediaData > & mapData, const QStringList & otherServers, std::shared_ptr< CProgressSystem > progressSystem );

    QStringList getOtherServers( const QString & serverName ) const;

    std::shared_ptr< CMediaData > findMediaForProviders( const std::map< QString, QString > & providerIDs, const QString & serverName ) const;
    std::shared_ptr<CMediaData>   findMediaForProvider( const std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > > & map, const QString & provider, const QString & id ) const;
    void setMediaForProviders( const std::map< QString, QString > & providerIDs, std::shared_ptr< CMediaData > mediaData, const QString & serverName );

    std::map< QString, TMediaIDToMediaData > fMediaMap; // serverName -> mediaID -> mediaData

    // provider name -> provider ID -> mediaData
    std::map < QString, std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > > >  fProviderSearchMap; // servername -> provider name, to map of id to mediadata

};

#endif
