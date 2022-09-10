#ifndef __MEDIAMODEL_H
#define __MEDIAMODEL_H

#include "SABUtils/HashUtils.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <optional>

class CSettings;
class CMediaData;
class CProgressSystem;
class CMergeMedia;

using TMediaIDToMediaData = std::map< QString, std::shared_ptr< CMediaData > >;

class CMediaModel : public QAbstractTableModel
{
    friend struct SMediaSummary;
    Q_OBJECT;
public:
    enum ECustomRoles
    {
        eShowItemRole = Qt::UserRole + 1,
        eMediaNameRole,
        eDirSortRole,
        eServerNameForColumnRole = Qt::UserRole + 10000
    };

    enum EColumns
    {
        eName,
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
    CMediaModel( std::shared_ptr< CSettings > settings, QObject * parent = nullptr );

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    void clear();

    bool hasMediaToProcess() const;

    void settingsChanged();

    std::shared_ptr< CMediaData > getMediaData( const QModelIndex & idx ) const;
    void updateMediaData( std::shared_ptr< CMediaData > mediaData );
    std::shared_ptr< CMediaData > getMediaDataForID( const QString & serverName, const QString & mediaID ) const;
    std::shared_ptr< CMediaData > loadMedia( const QString & serverName, const QJsonObject & media );
    std::shared_ptr< CMediaData > reloadMedia( const QString & serverName, const QJsonObject & media, const QString & mediaID );

    bool mergeMedia( std::shared_ptr< CProgressSystem > progressSystem );

    void loadMergedMedia( std::shared_ptr<CProgressSystem> progressSystem );

    std::unordered_set< std::shared_ptr< CMediaData > > getAllMedia() const { return fAllMedia; }
Q_SIGNALS:
    void sigPendingMediaUpdate();
private:
    int perServerColumn( int column ) const;
    int columnsPerServer( bool includeProviders = true ) const;
    QString serverNameForColumn( int columnNum ) const;

    std::optional< std::pair< QString, QString > > getProviderInfoForColumn( int column ) const;

    void addMediaInfo( const QString & serverName, std::shared_ptr<CMediaData> mediaData, const QJsonObject & mediaInfo );

    QVariant getColor( const QModelIndex & index, const QString & serverName, bool background ) const;
    void updateProviderColumns( std::shared_ptr< CMediaData > ii );

    std::unique_ptr< CMergeMedia > fMergeSystem;

    std::unordered_set< std::shared_ptr< CMediaData > > fAllMedia;
    std::map< QString, TMediaIDToMediaData > fMediaMap; // serverName -> mediaID -> mediaData

    std::vector< std::shared_ptr< CMediaData > > fData;
    std::shared_ptr< CSettings > fSettings;
    std::unordered_map< std::shared_ptr< CMediaData >, size_t > fMediaToPos;
    std::unordered_set< QString > fProviderNames;
    std::unordered_map< int, std::pair< QString, QString > > fProviderColumnsByColumn;
    EDirSort fDirSort{ eNoSort };
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
#endif
