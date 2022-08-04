#ifndef __MEDIAMODEL_H
#define __MEDIAMODEL_H

#include "SABUtils/HashUtils.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>

class CSettings;
class CMediaData;

class CMediaModel : public QAbstractTableModel
{
    Q_OBJECT;
public:
    enum ECustomRoles
    {
        eShowItemRole = Qt::UserRole + 1,
        eMediaNameRole,
        eDirSortRole,
        eDirValueRole
    };

    enum EColumns
    {
        eLHSName,
        eLHSMediaID,
        eLHSFavorite,
        eLHSPlayed,
        eLHSLastPlayed,
        eLHSPlayCount,
        eLHSPlaybackPosition,
        eDirection,
        eRHSName,
        eRHSMediaID,
        eRHSFavorite,
        eRHSPlayed,
        eRHSLastPlayed,
        eRHSPlayCount,
        eRHSPlaybackPosition
    };

    enum EDirSort
    {
        eNoSort=0,
        eLeftToRight=1,
        eEqual=2,
        eRightToLeft=3
    };
    CMediaModel( std::shared_ptr< CSettings > settings, QObject * parent );

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    EDirSort nextDirSort();

    void clear();
    void setMedia( const std::list< std::shared_ptr< CMediaData > > & media );

    void addMedia( std::shared_ptr< CMediaData > mediaData ); 

    bool isLHSColumn( int column ) const;
    bool isRHSColumn( int column ) const;

    struct SMediaSummary
    {
        int fTotalMedia{ 0 };
        int fNeedsSyncing{ 0 };
        int fMissingData{ 0 };
        int fRHSNeedsUpdating{ 0 };
        int fLHSNeedsUpdating{ 0 };
    };

    SMediaSummary settingsChanged();
    SMediaSummary getMediaSummary() const;

    std::shared_ptr< CMediaData > getMediaData( const QModelIndex & idx ) const;
    void updateMediaData( std::shared_ptr< CMediaData > mediaData );
private:
    QVariant getColor( const QModelIndex & index, bool background ) const;
    void updateProviderColumns( std::shared_ptr< CMediaData > ii );

    std::vector< std::shared_ptr< CMediaData > > fData;
    std::shared_ptr< CSettings > fSettings;
    std::unordered_map< std::shared_ptr< CMediaData >, size_t > fMediaToPos;
    std::unordered_set< QString > fProviderColumnsByName;
    std::unordered_map< int, std::pair< bool, QString > > fProviderColumnsByColumn;
    EDirSort fDirSort{ eNoSort };
};

class CMediaFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;
public:
    CMediaFilterModel( QObject * parent );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const override;

    bool dirLessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const;

};
#endif
