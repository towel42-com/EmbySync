#include "MediaMissingFilterModel.h"
#include "MediaModel.h"

CMediaMissingFilterModel::CMediaMissingFilterModel( std::shared_ptr< CSettings > settings, QObject *parent ) :
    QSortFilterProxyModel( parent ),
    fSettings( settings )
{
    fRegEx = fSettings->ignoreShowRegEx();
    setDynamicSortFilter( false );
    connect(
        this, &QSortFilterProxyModel::sourceModelChanged,
        [ this ]()
        {
            connect(
                dynamic_cast< CMediaModel * >( sourceModel() ), &CMediaModel::sigSettingsChanged,
                [ this ]()
                {
                    beginFilterChange();
                    fRegEx = fSettings->ignoreShowRegEx();
                    endFilterChange();
                } );
        } );
}

void CMediaMissingFilterModel::setShowFilter( const TFilterMap &filter )
{
    beginFilterChange();
    fShowFilterMap = filter;
    endFilterChange();

    auto col = sortColumn();
    auto order = sortOrder();
    sort( col, order );
}

bool CMediaMissingFilterModel::filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
{
    if ( !sourceModel() )
        return true;
    auto srcIdx = sourceModel()->index( source_row, 0, source_parent );
    return showEpisode( srcIdx );
}

bool CMediaMissingFilterModel::filterAcceptsColumn( int source_column, const QModelIndex &source_parent ) const
{
    auto idx = sourceModel()->index( 0, source_column, source_parent );
    return !idx.data( CMediaModel::ECustomRoles::eIsProviderColumnRole ).toBool();
}

void CMediaMissingFilterModel::sort( int column, Qt::SortOrder order /*= Qt::AscendingOrder */ )
{
    QSortFilterProxyModel::sort( column, order );
}

bool CMediaMissingFilterModel::lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const
{
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}

bool CMediaMissingFilterModel::showEpisode( const QModelIndex &idx ) const
{
    if ( !idx.isValid() )
        return false;

    auto seriesID = idx.data( CMediaModel::eSeriesIDRole ).toString();
    if ( seriesID.isEmpty() )
        return false;

    auto seriesName = idx.data( CMediaModel::eSeriesNameRole ).toString();
    if ( seriesName.isEmpty() )
        return false;

    int seasonNum = idx.data( CMediaModel::eSeasonNumRole ).toInt();

    return SShowFilter::showEpisode( fShowFilterMap, seriesID, seriesName, seasonNum, fRegEx );
}

QVariant CMediaMissingFilterModel::data( const QModelIndex &index, int role /*= Qt::DisplayRole */ ) const
{
    if ( ( role != Qt::ForegroundRole ) && ( role != Qt::BackgroundRole ) && ( role != CMediaModel::eShowItemRole ) )
        return QSortFilterProxyModel::data( index, role );

    if ( role == CMediaModel::eShowItemRole )
    {
        return showEpisode( index );
    }

    bool futureDate = ( index.data( CMediaModel::ECustomRoles::ePremiereDateRole ).toDate() > QDate::currentDate() );
    if ( futureDate )
        return QSortFilterProxyModel::data( index, role );

    // reverse for black background
    auto color = fSettings->dataMissingColor( (Qt::ItemDataRole)role );
    if ( !color.isValid() )
        return {};
    return color;
}
