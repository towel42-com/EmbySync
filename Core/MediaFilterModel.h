#ifndef __MEDIAFILTERMODEL_H
#define __MEDIAFILTERMODEL_H

#include <QSortFilterProxyModel>

class CMediaFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;

public:
    CMediaFilterModel( QObject *parent );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override;
};

#endif
