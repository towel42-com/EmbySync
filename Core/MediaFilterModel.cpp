#include "MediaFilterModel.h"
#include "MediaModel.h"

#include <QSortFilterProxyModel>

CMediaFilterModel::CMediaFilterModel( QObject *parent ) :
    QSortFilterProxyModel( parent )
{
    setDynamicSortFilter( false );
}

bool CMediaFilterModel::filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
{
    if ( !sourceModel() )
        return true;
    auto childIdx = sourceModel()->index( source_row, 0, source_parent );
    return childIdx.data( CMediaModel::eShowItemRole ).toBool();
}

void CMediaFilterModel::sort( int column, Qt::SortOrder order /*= Qt::AscendingOrder */ )
{
    QSortFilterProxyModel::sort( column, order );
}

bool CMediaFilterModel::lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const
{
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}


