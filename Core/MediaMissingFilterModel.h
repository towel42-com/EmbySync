#ifndef __MEDIAMISSINGFILTERMODEL_H
#define __MEDIAMISSINGFILTERMODEL_H

#include <QSortFilterProxyModel>
#include "Settings.h"

class CMediaMissingFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;

public:
    CMediaMissingFilterModel( std::shared_ptr< CSettings > settings, QObject *parent );

    void setShowFilter( const TFilterMap &filter );
    virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override;
    virtual bool filterAcceptsColumn( int source_column, const QModelIndex &source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override;

    virtual QVariant data( const QModelIndex &index, int role /*= Qt::DisplayRole */ ) const override;

private:
    bool showEpisode( const QModelIndex &srcIdx ) const;
    std::shared_ptr< CSettings > fSettings;
    std::optional< QRegularExpression > fRegEx;
    std::optional< TFilterMap > fShowFilterMap;
};

#endif
