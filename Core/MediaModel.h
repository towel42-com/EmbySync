#ifndef __MEDIAMODEL_H
#define __MEDIAMODEL_H

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
class QJsonObject;
struct SMovieStub;

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
        eResolutionRole,
        eIsProviderColumnRole,
        eSeriesNameRole,
        eOnServerRole,
        eColumnsPerServerRole,
        ePerServerColumnRole,
        eShowInSearchMovieRole
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
        eResolution,
        eFirstServerColumn = eResolution
    };

    enum EDirSort
    {
        eNoSort = 0,
        eLeftToRight = 1,
        eEqual = 2,
        eRightToLeft = 3
    };
    CMediaModel( std::shared_ptr< CSettings > settings, std::shared_ptr< CServerModel > serverModel, QObject *parent = nullptr );

    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    virtual QString serverForColumn( int column ) const override;
    virtual std::list< int > columnsForBaseColumn( int baseColumn ) const override;
    virtual std::list< int > providerColumns() const;

    void clear();

    bool hasMediaToProcess() const;

    void settingsChanged();

    std::shared_ptr< CMediaData > getMediaData( const QModelIndex &idx ) const;
    std::shared_ptr< CMediaData > getMediaDataForID( const QString &serverName, const QString &mediaID ) const;
    std::shared_ptr< CMediaData > loadMedia( const QString &serverName, const QJsonObject &media );
    std::shared_ptr< CMediaData > reloadMedia( const QString &serverName, const QJsonObject &media, const QString &mediaID );

    void removeMedia( const QString &serverName, const std::shared_ptr< CMediaData > &media );

    void beginBatchLoad();
    void endBatchLoad();

    bool mergeMedia( std::shared_ptr< CProgressSystem > progressSystem );

    void loadMergedMedia( std::shared_ptr< CProgressSystem > progressSystem );

    void addMedia( const std::shared_ptr< CMediaData > &media, bool emitUpdate );

    using TMediaSet = std::unordered_set< std::shared_ptr< CMediaData > >;

    TMediaSet getAllMedia() const { return fAllMedia; }
    std::unordered_set< QString > getKnownShows() const;

    std::shared_ptr< CMediaData > findMedia( const QString &name, int year ) const;

    using iterator = typename TMediaSet::iterator;
    using const_iterator = typename TMediaSet::const_iterator;

    iterator begin() { return fAllMedia.begin(); }
    iterator end() { return fAllMedia.end(); }
    const_iterator begin() const { return fAllMedia.cbegin(); }
    const_iterator end() const { return fAllMedia.cend(); }

    void addMovieStub( const SMovieStub &movieStub, std::function< bool( std::shared_ptr< CMediaData > mediaData ) > equal );
    void removeMovieStub( const SMovieStub &movieStub );

    void clearAllMovieStubs();

    void setLabelMissingFromServer( bool value );
    bool labelMissingFromServer() const { return fLabelMissingFromServer; }

    void setOnlyShowPremierYear( bool value );
    bool onlyShowPremierYear() const { return fOnlyShowPremierYear; }

Q_SIGNALS:
    void sigPendingMediaUpdate();
    void sigSettingsChanged();
    void sigMediaChanged();

private:
    void removeMovieStub( const std::shared_ptr< CMediaData > &media );

    int perServerColumn( int column ) const;
    int columnsPerServer( bool includeProviders = true ) const;

    std::optional< std::pair< QString, QString > > getProviderInfoForColumn( int column ) const;

    void addMediaInfo( const QString &serverName, std::shared_ptr< CMediaData > mediaData, const QJsonObject &mediaInfo );
    void updateMediaData( std::shared_ptr< CMediaData > mediaData );

    QVariant getColor( const QModelIndex &index, const QString &serverName, bool background ) const;
    void updateProviderColumns( std::shared_ptr< CMediaData > ii );

    std::unique_ptr< CMergeMedia > fMergeSystem;

    TMediaSet fAllMedia;
    std::map< QString, TMediaIDToMediaData > fMediaMap;   // serverName -> mediaID -> mediaData

    std::vector< std::shared_ptr< CMediaData > > fData;
    std::unordered_map< QString, std::shared_ptr< CMediaData > > fDataMap;
    std::unordered_map< std::shared_ptr< CMediaData >, size_t > fMediaToPos;
    std::unordered_set< QString > fProviderNames;
    std::unordered_map< int, std::pair< QString, QString > > fProviderColumnsByColumn;
    EDirSort fDirSort{ eNoSort };

    std::shared_ptr< CServerModel > fServerModel;
    std::shared_ptr< CSettings > fSettings;
    bool fLabelMissingFromServer{ true };
    bool fOnlyShowPremierYear{ false };
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
    CMediaFilterModel( QObject *parent );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override;
};

class CMediaMissingFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;

public:
    CMediaMissingFilterModel( std::shared_ptr< CSettings > settings, QObject *parent );

    void setShowFilter( const QString &filter );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override;
    virtual bool filterAcceptsColumn( int source_column, const QModelIndex &source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override;

    virtual QVariant data( const QModelIndex &index, int role /*= Qt::DisplayRole */ ) const override;

private:
    std::shared_ptr< CSettings > fSettings;
    QRegularExpression fRegEx;
    QString fShowFilter;
};

#endif
