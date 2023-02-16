#ifndef __MEDIAMODEL_H
#define __MEDIAMODEL_H

#include "SABUtils/HashUtils.h"
#include "IServerForColumn.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <optional>

class CSettings;
class CMediaData;
class CMediaCollection;
struct SMediaCollectionData;
class CProgressSystem;
class CMergeMedia;
class CServerModel;
class CSyncSystem;
class CServerInfo;

using TMediaIDToMediaData = std::map< QString, std::shared_ptr< CMediaData > >;

class CMediaModel : public QAbstractTableModel, public IServerForColumn
{
    friend struct SMediaSummary;
    Q_OBJECT;
public:
    enum ECustomRoles
    {
        eShowItemRole = Qt::UserRole + 1,
        eMediaNameRole,
        eDirSortRole,
        ePremiereDateRole,
        eIsProviderColumnRole,
        eIsNameColumnRole,
        eIsPremiereDateColumnRole,
        eSeriesNameRole,
        eOnServerRole
    };

    enum EColumns
    {
        eName,
        ePremiereDate,
        eType,
        eMediaID,
        eFavorite,
        ePlayed,
        eLastPlayed,
        ePlayCount,
        ePlaybackPosition,
        eFirstServerColumn = ePlaybackPosition
    };

    enum EDirSort
    {
        eNoSort=0,
        eLeftToRight=1,
        eEqual=2,
        eRightToLeft=3
    };
    CMediaModel( std::shared_ptr< CSettings > settings, std::shared_ptr< CServerModel > serverModel, QObject * parent = nullptr );

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    virtual QString serverForColumn( int column ) const override;
    virtual std::list< int > columnsForBaseColumn( int baseColumn ) const override;
    virtual std::list< int > providerColumns() const;

    void clear();

    bool hasMediaToProcess() const;

    void settingsChanged();

    std::shared_ptr< CMediaData > getMediaData( const QModelIndex & idx ) const;
    std::shared_ptr< CMediaData > getMediaDataForID( const QString & serverName, const QString & mediaID ) const;
    std::shared_ptr< CMediaData > loadMedia( const QString & serverName, const QJsonObject & media );
    std::shared_ptr< CMediaData > reloadMedia( const QString & serverName, const QJsonObject & media, const QString & mediaID );

    void removeMedia( const QString & serverName, const std::shared_ptr< CMediaData > & media );

    void beginBatchLoad();
    void endBatchLoad();

    bool mergeMedia( std::shared_ptr< CProgressSystem > progressSystem );

    void loadMergedMedia( std::shared_ptr<CProgressSystem> progressSystem );

    using TMediaSet = std::unordered_set< std::shared_ptr< CMediaData > >;

    TMediaSet getAllMedia() const { return fAllMedia; }
    std::unordered_set< QString > getKnownShows() const;

    std::shared_ptr< CMediaData > findMedia( const QString & name, int year ) const;

    using iterator = typename TMediaSet::iterator;
    using const_iterator = typename TMediaSet::const_iterator;

    iterator begin() { return fAllMedia.begin(); }
    iterator end() { return fAllMedia.end(); }
    const_iterator begin() const { return fAllMedia.cbegin(); }
    const_iterator end() const { return fAllMedia.cend(); }

    void addMovieStub( const QString & name, int year );

    void addCollection( const QString & serverName, const QString & name, const QString & id, const std::list< std::shared_ptr< CMediaData > > & items );
    std::shared_ptr< CMediaCollection > findCollection( const QString & serverName, const QString & name );
Q_SIGNALS:
    void sigPendingMediaUpdate();
    void sigSettingsChanged();
    void sigMediaChanged();
private:
    int perServerColumn( int column ) const;
    int columnsPerServer( bool includeProviders = true ) const;

    std::optional< std::pair< QString, QString > > getProviderInfoForColumn( int column ) const;

    void addMediaInfo( const QString & serverName, std::shared_ptr<CMediaData> mediaData, const QJsonObject & mediaInfo );
    void updateMediaData( std::shared_ptr< CMediaData > mediaData );

    QVariant getColor( const QModelIndex & index, const QString & serverName, bool background ) const;
    void updateProviderColumns( std::shared_ptr< CMediaData > ii );

    std::unique_ptr< CMergeMedia > fMergeSystem;

    TMediaSet fAllMedia;
    std::map< QString, TMediaIDToMediaData > fMediaMap; // serverName -> mediaID -> mediaData
    std::map< QString, std::vector< std::shared_ptr< CMediaCollection > > > fCollections; // serverName -> collectionData

    std::vector< std::shared_ptr< CMediaData > > fData;
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CServerModel > fServerModel;
    std::unordered_map< std::shared_ptr< CMediaData >, size_t > fMediaToPos;
    std::unordered_set< QString > fProviderNames;
    std::unordered_map< int, std::pair< QString, QString > > fProviderColumnsByColumn;
    EDirSort fDirSort{ eNoSort };
};

struct SIndexPtr
{
    SIndexPtr( void * ptr, bool isCollection );
    void * fPtr{ nullptr };
    bool fIsCollection{ false };
};

class CCollectionsModel : public QAbstractItemModel
{
    Q_OBJECT;
public:
    CCollectionsModel( std::shared_ptr< CMediaModel > mediaModel );

    virtual  QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const override;
    virtual QModelIndex parent( const QModelIndex & child ) const override;
    virtual bool hasChildren( const QModelIndex & parent ) const override;

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */ ) const override;

    std::pair< QModelIndex, std::shared_ptr< CMediaCollection > > addCollection( const QString & server, const QString & name );
    std::shared_ptr< SMediaCollectionData > addMovie( int rank, const QString & name, int year, const QModelIndex & collectionIndex );

    CMediaCollection * collection( const QModelIndex & idx ) const;
    SMediaCollectionData * media( const QModelIndex & idx ) const;

    bool isMedia( const QModelIndex & parent ) const;
    bool isCollection( const QModelIndex & parent ) const;

    QString summary() const;
    void clear();

    void updateCollections( const QString & serverName, std::shared_ptr< CMediaModel > model );
    void createCollections( std::shared_ptr<const CServerInfo> serverInfo, std::shared_ptr< CSyncSystem > syncSystem );
public Q_SLOTS:
    void slotMediaModelDataChanged();
private:
    std::vector< std::shared_ptr< CMediaCollection > > fCollections;
    
    SIndexPtr * idxPtr( CMediaCollection * mediaCollection ) const;
    SIndexPtr * idxPtr( SMediaCollectionData * media ) const;
    SIndexPtr * idxPtr( void * media, bool isCollection ) const;

    mutable std::map< void *, SIndexPtr * > fIndexPtrs;

    std::shared_ptr< CMediaModel > fMediaModel;
};

struct SMediaSummary
{
    SMediaSummary( std::shared_ptr< CMediaModel > model );

    int fTotalMedia{ 0 };

    QString getSummaryText() const;
    std::map< QString, int > fNeedsUpdating;
    std::map< QString, int > fMissingData;
};

class CMediaFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;
public:
    CMediaFilterModel( QObject * parent );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const override;

};

class CMediaMissingFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;
public:
    CMediaMissingFilterModel( std::shared_ptr< CSettings > settings, QObject * parent );

    void setShowFilter( const QString & filter );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;
    virtual bool filterAcceptsColumn( int source_column, const QModelIndex & source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const override;

    virtual QVariant data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const override;
private:
    std::shared_ptr< CSettings > fSettings;
    QRegularExpression fRegEx;
    QString fShowFilter;
};

struct SDummyMovie
{
    QString fName;
    int fYear{ 0 };

    bool operator==( const SDummyMovie & r ) const
    {
        return fName.toLower().trimmed() == r.fName.toLower().trimmed();
    }
};

namespace std
{
    template <>
    struct hash< SDummyMovie >
    {
        std::size_t operator()( const SDummyMovie & k ) const
        {
            std::size_t h1 = 0; // std::hash< int >{}( k.second );
            std::size_t h2 = qHash( k.fName.toLower().trimmed() );
            return h1 & ( h2 << 1 );
        }
    };
}


class CMovieSearchFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;
public:
    CMovieSearchFilterModel( std::shared_ptr< CSettings > settings, QObject * parent );

    void addSearchMovie( const QString & name, int year, bool invalidate );
    void finishedAddingSearchMovies();

    void addMissingMoviesToSourceModel();;
    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;
    virtual bool filterAcceptsColumn( int source_column, const QModelIndex & source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const override;

    virtual QVariant data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const override;

    QString summary() const;
private:
    std::shared_ptr< CSettings > fSettings;
    std::unordered_set< SDummyMovie > fSearchForMovies;
};
#endif
