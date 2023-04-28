#include "MovieSearchFilterModel.h"
#include "MediaModel.h"
#include "MediaData.h"
#include "Settings.h"

#include "SABUtils/StringUtils.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

CMovieSearchFilterModel::CMovieSearchFilterModel( std::shared_ptr< CSettings > settings, QObject *parent ) :
    QSortFilterProxyModel( parent ),
    fSettings( settings )
{
    setDynamicSortFilter( false );
    connect( this, &QSortFilterProxyModel::sourceModelChanged, [ this ]() { connect( dynamic_cast< CMediaModel * >( sourceModel() ), &CMediaModel::sigSettingsChanged, [ this ]() { startInvalidateTimer(); } ); } );
}

void CMovieSearchFilterModel::addSearchMovie( const QString &name, int year, const std::pair< int, int > &resolution, bool postLoad )
{
    auto movieStub = SMovieStub( name, year, resolution );
    fSearchForMoviesByName.insert( movieStub );
    fSearchForMoviesByNameYear.insert( movieStub );
    if ( postLoad )
        addStubToSourceModel( movieStub );

    startInvalidateTimer();
}

void CMovieSearchFilterModel::startInvalidateTimer()
{
    if ( !fTimer )
    {
        fTimer = new QTimer( this );
        fTimer->setInterval( 50 );
        fTimer->setSingleShot( true );
        connect( fTimer, &QTimer::timeout, this, &CMovieSearchFilterModel::slotInvalidateFilter );
    }
    fTimer->stop();
    fTimer->start();
}

void CMovieSearchFilterModel::slotInvalidateFilter()
{
    invalidateFilter();
}

void CMovieSearchFilterModel::addMoviesToSourceModel()
{
    auto mediaModel = dynamic_cast< CMediaModel * >( sourceModel() );
    if ( !mediaModel )
        return;

    for ( auto &&ii : fSearchForMoviesByName )
    {
        addStubToSourceModel( ii );
    }
    startInvalidateTimer();
}

void CMovieSearchFilterModel::addStubToSourceModel( const SMovieStub &movieStub )
{
    auto mediaModel = dynamic_cast< CMediaModel * >( sourceModel() );
    if ( mediaModel )
    {
        mediaModel->addMovieStub( movieStub, [ this, movieStub ]( std::shared_ptr< CMediaData > mediaData ) { return movieStub.equal( mediaData, true, true, false ); } );
    }
}

void CMovieSearchFilterModel::removeSearchMovie( const std::shared_ptr< CMediaData > &data )
{
    auto mediaModel = dynamic_cast< CMediaModel * >( sourceModel() );
    if ( !mediaModel )
        return;

    auto movieStub = SMovieStub( data );

    removeSearchMovie( movieStub );
}

void CMovieSearchFilterModel::removeSearchMovie( const QModelIndex &idx )
{
    removeSearchMovie( getMovieStub( idx ) );
}

void CMovieSearchFilterModel::removeSearchMovie( const SMovieStub &movieStub )
{
    auto pos2 = fSearchForMoviesByName.find( movieStub );
    if ( pos2 != fSearchForMoviesByName.end() )
        fSearchForMoviesByName.erase( pos2 );

    auto pos3 = fSearchForMoviesByNameYear.find( movieStub );
    if ( pos3 != fSearchForMoviesByNameYear.end() )
        fSearchForMoviesByNameYear.erase( pos3 );

    dynamic_cast< CMediaModel * >( sourceModel() )->removeMovieStub( movieStub );

    startInvalidateTimer();
}

SMovieStub CMovieSearchFilterModel::getMovieStub( const QModelIndex &idx ) const
{
    auto name = idx.data( CMediaModel::eMediaNameRole ).toString();
    auto year = idx.data( CMediaModel::ePremiereDateRole ).toDate().year();
    auto resolution = idx.data( CMediaModel::eResolutionRole ).toPoint();

    auto movieStub = SMovieStub( name, year, resolution );
    return movieStub;
}

void CMovieSearchFilterModel::setOnlyShowMissing( bool value )
{
    fOnlyShowMissing = value;
    startInvalidateTimer();
}

void CMovieSearchFilterModel::setMatchResolution( bool value )
{
    fMatchResolution = value;
    dynamic_cast< CMediaModel * >( sourceModel() )->clearAllMovieStubs();
    addMoviesToSourceModel();
}

std::optional< SMovieStub > CMovieSearchFilterModel::inSearchForMovie( const SMovieStub &movieStub ) const
{
    bool found = false;

    auto pos1 = fSearchForMoviesByNameYear.find( movieStub );
    if ( pos1 != fSearchForMoviesByNameYear.end() )
        return ( *pos1 );

    auto pos2 = fSearchForMoviesByName.find( movieStub );
    if ( pos2 != fSearchForMoviesByName.end() )
        return ( *pos2 );

    return {};
}

bool CMovieSearchFilterModel::filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
{
    if ( !sourceModel() )
        return true;
    auto childIdx = sourceModel()->index( source_row, 0, source_parent );

    auto movieStub = getMovieStub( childIdx );

    if ( !inSearchForMovie( movieStub ).has_value() )
        return false;

    if ( !fOnlyShowMissing )
        return true;

    auto onServer = childIdx.data( CMediaModel::ECustomRoles::eOnServerRole ).toBool();
    if ( !onServer )
        return true;
    return false;
}

bool CMovieSearchFilterModel::filterAcceptsColumn( int source_column, const QModelIndex &source_parent ) const
{
    auto idx = sourceModel()->index( 0, source_column, source_parent );
    if ( !idx.isValid() )
        return false;

    return ( source_column < idx.data( CMediaModel::ECustomRoles::eColumnsPerServerRole ).toInt() ) && idx.data( CMediaModel::ECustomRoles::eShowInSearchMovieRole ).toBool();
}

void CMovieSearchFilterModel::sort( int column, Qt::SortOrder order /*= Qt::AscendingOrder */ )
{
    QSortFilterProxyModel::sort( column, order );
}

bool CMovieSearchFilterModel::lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const
{
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}

QVariant CMovieSearchFilterModel::data( const QModelIndex &index, int role /*= Qt::DisplayRole */ ) const
{
    if ( ( role != Qt::ForegroundRole ) && ( role != Qt::BackgroundRole ) && ( role != Qt::DisplayRole ) && ( role != eResolutionMatches ) )
        return QSortFilterProxyModel::data( index, role );

    bool onServer = false;
    bool resolutionMatches = false;
    std::optional< SMovieStub > searchStub;
    std::tie( onServer, resolutionMatches, searchStub ) = getSearchStatus( index );
    if ( role == eResolutionMatches )
        return resolutionMatches;

    if ( role == Qt::DisplayRole )
    {
        auto retVal = QSortFilterProxyModel::data( index, role );
        auto perServerColumn = index.data( CMediaModel::ECustomRoles::ePerServerColumnRole ).toInt();
        if ( searchStub.has_value() && ( perServerColumn == CMediaModel::EColumns::eResolution ) && onServer )
        {
            retVal = retVal.toString() + QString( " Searching for: %1" ).arg( searchStub.value().resolution() );
        }
        return retVal;
    }

    if ( onServer && resolutionMatches )
        return {};

    // reverse for black background
    if ( role == Qt::ForegroundRole )
    {
        auto color = fSettings->dataMissingColor( false );
        if ( !color.isValid() )
            return {};
        return color;
    }

    if ( role == Qt::BackgroundRole )
    {
        auto color = fSettings->dataMissingColor( true );
        if ( !color.isValid() )
            return {};
        return color;
    }
    return {};
}

std::tuple< bool, bool, std::optional< SMovieStub > > CMovieSearchFilterModel::getSearchStatus( const QModelIndex &index ) const
{
    auto movieStub = getMovieStub( index );
    auto searchStub = inSearchForMovie( movieStub );
    auto onServer = index.data( CMediaModel::ECustomRoles::eOnServerRole ).toBool();
    bool resolutionMatches = true;
    if ( onServer && fMatchResolution )
    {
        resolutionMatches = movieStub.equal( searchStub.value(), true, true, true );
    }

    return std::make_tuple( onServer, resolutionMatches, searchStub );
}

QString CMovieSearchFilterModel::summary() const
{
    int missing = 0;
    int diffResolution = 0;
    for ( int ii = 0; ii < rowCount(); ++ii )
    {
        auto idx = index( ii, 0, QModelIndex() );
        if ( !idx.data( CMediaModel::ECustomRoles::eOnServerRole ).toBool() )
            missing++;
        else if ( !idx.data( eResolutionMatches ).toBool() )
            diffResolution++;
    }
    return tr( "Searching for: %1 Missing: %2 Different Resolution: %3" ).arg( static_cast< int >( fSearchForMoviesByName.size() ) ).arg( missing ).arg( diffResolution );
}

QJsonObject CMovieSearchFilterModel::toJSON() const
{
    QJsonArray moviesArray;
    for ( auto &&ii : fSearchForMoviesByName )
    {
        auto curr = ii.toJSON();
        moviesArray.push_back( curr );
    }
    QJsonObject retVal;
    retVal[ "movies" ] = moviesArray;
    return retVal;
}

void CMovieSearchFilterModel::saveMissing( QWidget *parent ) const
{
    auto fileName = QFileDialog::getSaveFileName( parent, tr( "FileName" ), QString(), "JSON Files (*.json);;CSV Files (*.csv);;All Files (*.*)" );
    if ( fileName.isEmpty() )
        return;

    auto isJSON = QFileInfo( fileName ).suffix().toLower() == "json";
    QJsonArray jsonMovies;
    std::list< QStringList > stringMovies;
    stringMovies.push_back(
        QStringList() << "Movie Name"
                      << "Year"
                      << "Reason" );
    int rowCount = this->rowCount();
    for ( int ii = 0; ii < rowCount; ++ii )
    {
        auto name = index( ii, 0 ).data().toString();
        auto year = index( ii, 1 ).data().toInt();

        auto status = getSearchStatus( index( ii, 0 ) );
        bool onServer = std::get< 0 >( status );
        bool resolutionOK = std::get< 1 >( status );
        if ( onServer && resolutionOK )
            continue;
        QString reason;
        if ( !onServer )
            reason = "Missing";
        else if ( !resolutionOK )
            reason = "Better Resolution";

        if ( isJSON )
        {
            QJsonObject curr;
            curr[ "name" ] = name;
            curr[ "year" ] = year;
            curr[ "reason" ] = reason;
            jsonMovies.push_back( curr );
        }
        else
        {
            stringMovies.push_back( QStringList() << name << QString::number( year ) << reason );
        }
    }

    QFile fi( fileName );
    if ( !fi.open( QFile::WriteOnly | QFile::Text ) )
    {
        QMessageBox::critical( parent, tr( "Could not create file" ), tr( "Could not create/open file for writing '%1'" ).arg( fileName ) );
        return;
    }

    QTextStream ts( &fi );

    if ( isJSON )
    {
        QJsonObject root;
        root[ "movies" ] = jsonMovies;

        QJsonDocument doc( root );
        ts << doc.toJson( QJsonDocument::JsonFormat::Indented );
    }
    else
    {
        for ( auto &&ii : stringMovies )
        {
            ts << NSABUtils::NStringUtils::toCSV( ii ) << Qt::endl;
        }
    }
}
