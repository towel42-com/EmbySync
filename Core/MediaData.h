// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
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

#ifndef __MEDIADATA_H
#define __MEDIADATA_H

#include <QString>
#include <QUrlQuery>
#include <QDateTime>
#include <QIcon>

#include <functional>
#include <map>
#include <memory>
#include <optional>
class CServerInfo;
class CMediaModel;
class QJsonObject;
class CServerModel;
class CSyncSystem;
class QMenu;
struct SMediaServerData;

enum class EMediaSyncStatus
{
    eNoServerPairs,   // 1 or less servers have this media, nothing to sync show as issue media
    eMediaEqualOnValidServers,   // 2 or more servers have media, but all have the same data
    eMediaNeedsUpdating   // 2 or more servers have media and a sync is necessar
};

enum EMissingProviderIDs : uint8_t
{
    eNone = 0x00,
    eTVDBid = 0x01,
    eTMDBid = 0x02,
    eIMDBid = 0x04,
    eTVRageid = 0x08
};

class CMediaData
{
public:
    static QStringList getHeaderLabels();
    static void setMSecsToStringFunc( std::function< QString( uint64_t ) > func );
    static std::function< QString( uint64_t ) > mecsToStringFunc();

    CMediaData( const QJsonObject &mediaObj, std::shared_ptr< CServerModel > serverModel );
    CMediaData( const QString &name, int year, const QString &type );   // stub for dummy media

    void addSearchMenu( QMenu * menu ) const;
    static bool isExtra( const QJsonObject &obj );
    bool hasProviderIDs() const;
    void addProvider( const QString &providerName, const QString &providerID );

    QString name() const;
    QString originalTitle() const { return fOriginalTitle; }
    QString seriesName() const;
    QString mediaType() const;
    bool beenLoaded( const QString &serverName ) const;

    void loadData( const QString &serverName, const QJsonObject &object );
    void updateFromOther( const QString &otherServerName, std::shared_ptr< CMediaData > other );

    QUrlQuery getSearchForMediaQuery() const;

    QString getProviderID( const QString &provider );
    std::map< QString, QString > getProviders( bool addKeyIfEmpty = false ) const;
    std::map< QString, QString > getExternalUrls() const { return fExternalUrls; }

    QString externalUrlsText() const;

    QString getMediaID( const QString &serverName ) const;
    void setMediaID( const QString &serverName, const QString &id );

    void updateCanBeSynced();

    bool isValidForServer( const QString &serverName ) const;
    bool isValidForAllServers() const;
    bool canBeSynced() const;
    bool validUserDataEqual() const;
    EMediaSyncStatus syncStatus() const;

    bool needsUpdating( const QString &serverName ) const;
    template< class T >
    void needsUpdating( T ) const = delete;

    bool allPlayedEqual() const;
    bool isPlayed( const QString &serverName ) const;

    QString playbackPosition( const QString &serverName ) const;
    uint64_t playbackPositionMSecs( const QString &serverName ) const;
    uint64_t playbackPositionTicks( const QString &serverName ) const;
    QTime playbackPositionTime( const QString &serverName ) const;

    bool allPlaybackPositionTicksEqual() const;

    bool allFavoriteEqual() const;
    bool isFavorite( const QString &serverName ) const;

    QDateTime lastPlayed( const QString &serverName ) const;
    bool allLastPlayedEqual() const;

    uint64_t playCount( const QString &serverName ) const;
    bool allPlayCountEqual() const;

    QDate premiereDate() const { return fPremiereDate; }

    std::shared_ptr< SMediaServerData > userMediaData( const QString &serverName ) const;
    std::shared_ptr< SMediaServerData > newestMediaData() const;

    QIcon getDirectionIcon( const QString &serverName ) const;

    enum class ESearchSite
    {
        eRARBG,
        ePirateBay,
        eIMDB
    };
    QUrl getSearchURL( ESearchSite site ) const;

    QJsonObject toJson( bool includeSearchURL );

    bool onServer() const;
    bool isMatch( const QString &name, int year ) const;

    bool isMissingProvider( EMissingProviderIDs missingIdsType ) const;

private:
    void computeName( const QJsonObject &media );
    template< typename T >
    bool allEqual( std::function< T( std::shared_ptr< SMediaServerData > ) > func ) const
    {
        std::optional< T > prevValue;
        for ( auto &&ii : fInfoForServer )
        {
            if ( !ii.second->isValid() )
                continue;

            if ( !prevValue.has_value() )
                prevValue = func( ii.second );
            else if ( prevValue.value() != func( ii.second ) )
                return false;
        }
        return true;
    }
    QString getProviderList() const;

    QString fType;
    QString fName;
    QString fOriginalTitle;
    QString fSeriesName;   // only valid for EpisodeTypes
    std::optional< int > fSeason;   // only valid for EpisodeTypes
    std::optional< int > fEpisode;   // only valid for EpisodeTypes
    std::map< QString, QString > fProviders;
    std::map< QString, QString > fExternalUrls;
    QDate fPremiereDate;

    bool fCanBeSynced{ false };
    std::map< QString, std::shared_ptr< SMediaServerData > > fInfoForServer;

    static std::function< QString( uint64_t ) > sMSecsToStringFunc;
};

class CMediaCollection;
struct SMediaCollectionData
{
    SMediaCollectionData( std::shared_ptr< CMediaData > data, CMediaCollection *collection ) :
        fData( data ),
        fCollection( collection )
    {
    }
    QVariant data( int column, int role ) const;
    ;
    bool updateMedia( std::shared_ptr< CMediaModel > mediaModel );
    std::shared_ptr< CMediaData > fData;
    CMediaCollection *fCollection{ nullptr };
};

struct SCollectionServerInfo
{
    SCollectionServerInfo( const QString &id );

    bool updateMedia( std::shared_ptr< CMediaModel > mediaModel );

    int childCount() const { return static_cast< int >( fItems.size() ); }
    int numMissing() const;

    std::shared_ptr< SMediaCollectionData > child( int pos ) const   // may be null
    {
        if ( ( pos < 0 ) || ( pos >= fItems.size() ) )
            return {};
        return fItems[ pos ];
    }

    void createCollection( std::shared_ptr< const CServerInfo > serverInfo, const QString &collectionName, std::shared_ptr< CSyncSystem > syncSystem );

    bool collectionExists() const { return !fCollectionID.isEmpty(); }
    void setId( const QString &id ) { fCollectionID = id; }
    bool missingMedia() const;

    std::shared_ptr< SMediaCollectionData > addMovie( const QString &name, int year, CMediaCollection *parent, int rank );

    QString fCollectionID;
    std::vector< std::shared_ptr< SMediaCollectionData > > fItems;
};

class CMediaCollection
{
public:
    CMediaCollection( const QString &serverName, const QString &name, const QString &id, int pos );
    int childCount() const { return fCollectionInfo ? fCollectionInfo->childCount() : 0; }
    std::shared_ptr< SMediaCollectionData > child( int pos ) const { return fCollectionInfo->child( pos ); }
    QVariant data( int column, int role ) const;

    std::shared_ptr< SMediaCollectionData > addMovie( const QString &name, int year, int rank );
    void setItems( const std::list< std::shared_ptr< CMediaData > > &items );
    bool updateMedia( std::shared_ptr< CMediaModel > mediaModel ) { return fCollectionInfo->updateMedia( mediaModel ); }
    bool missingMedia() const { return fCollectionInfo->missingMedia(); }

    int numMovies() const { return childCount(); }
    int numMissing() const { return fCollectionInfo ? fCollectionInfo->numMissing() : 0; }
    int collectionNum() const { return fPosition; }

    QString name() const { return fName; }

    bool updateWithRealCollection( std::shared_ptr< CMediaCollection > realCollection );
    bool collectionExists() const { return fCollectionInfo->collectionExists(); }
    void createCollection( std::shared_ptr< const CServerInfo > serverInfo, std::shared_ptr< CSyncSystem > syncSystem ) { fCollectionInfo->createCollection( serverInfo, fName, syncSystem ); }
    bool isUnNamed() const { return name() == "<Unnamed Collection>"; }
    void setFileName( const QString &fileName ) { fFileName = fileName; }
    QString fileBaseName() const;
    void setName( const QString &name ) { fName = name; }

private:
    QString fServerName;
    QString fFileName;
    QString fName;
    int fPosition{ -1 };
    std::shared_ptr< SCollectionServerInfo > fCollectionInfo;
};

namespace NJSON
{
    class CMovie
    {
    public:
        CMovie( const QJsonValue &curr );

        QString name() const { return fName; }
        int rank() const { return fRank; }
        int year() const { return fYear; }

        void setRank( int rank ) { fRank = rank; }

    private:
        QString fName;
        int fRank{ -1 };
        int fYear{ -1 };
    };

    class CCollection
    {
    public:
        CCollection( const QJsonValue & curr );
        
        QString name() const { return fName; }
        const std::list< std::shared_ptr< CMovie > > & movies() const { return fMovies; }
    private:
        QString fName;
        std::list< std::shared_ptr< CMovie > > fMovies;
    };

    class CCollections
    {
    public:
        CCollections() {}
        static std::optional< std::shared_ptr< CCollections > > fromJSON( const QString &fileName, QString *msg = nullptr );

        const std::list< std::shared_ptr< CCollection > > &collections() const { return fCollections; }
        const std::list< std::shared_ptr< CMovie > > &movies() const { return fMovies; }

    private:
        std::list< std::shared_ptr< CCollection > > fCollections;
        std::list< std::shared_ptr< CMovie > > fMovies;
    };
}
#endif
