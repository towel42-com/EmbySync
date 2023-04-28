#ifndef __MOVIESEARCHFILTERMODEL_H
#define __MOVIESEARCHFILTERMODEL_H

#include "MovieStub.h"
#include <QSortFilterProxyModel>
#include <QString>
#include <unordered_set>
#include <optional>
#include <memory>
#include <tuple>

class CSettings;
class CMediaData;
class CMovieSearchFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;

public:
    enum ECustomRoles
    {
        eResolutionMatches = Qt::UserRole + 100
    };

    CMovieSearchFilterModel( std::shared_ptr< CSettings > settings, QObject *parent );

    void addSearchMovie( const QString &name, int year, const std::pair< int, int > &resolution, bool postLoad );

    void addMoviesToSourceModel();
    void removeSearchMovie( const QModelIndex &idx );
    void removeSearchMovie( const std::shared_ptr< CMediaData > &data );

    void removeSearchMovie( const SMovieStub &movieStub );

    SMovieStub getMovieStub( const QModelIndex &idx ) const;

    void setOnlyShowMissing( bool value );
    bool onlyShowMissing() const { return fOnlyShowMissing; }

    void setMatchResolution( bool value );
    bool matchResolution() const { return fMatchResolution; }

    virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override;
    virtual bool filterAcceptsColumn( int source_column, const QModelIndex &source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override;

    virtual QVariant data( const QModelIndex &index, int role /*= Qt::DisplayRole */ ) const override;

    QString summary() const;

    QJsonObject toJSON() const;

    void saveMissing( QWidget *parent ) const;
    private Q_SLOTS:
    void slotInvalidateFilter();

private:
    void startInvalidateTimer();
    void addStubToSourceModel( const SMovieStub &movieStub );
    std::tuple< bool, bool, std::optional< SMovieStub > > getSearchStatus( const QModelIndex &index ) const;

    std::optional< SMovieStub > inSearchForMovie( const SMovieStub &movieStub ) const;

    std::shared_ptr< CSettings > fSettings;

    std::unordered_set< SMovieStub, SNameHash, SNameCompare > fSearchForMoviesByName;
    std::unordered_set< SMovieStub, SNameYearHash, SNameYearCompare > fSearchForMoviesByNameYear;
    QTimer *fTimer{ nullptr };
    bool fOnlyShowMissing{ false };
    bool fMatchResolution{ false };
};
#endif
