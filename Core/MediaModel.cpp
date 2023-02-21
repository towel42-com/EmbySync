#include "MediaModel.h"
#include "MediaData.h"
#include "MergeMedia.h"

#include "Settings.h"
#include "ServerInfo.h"
#include "ServerModel.h"
#include "SABUtils/StringUtils.h"
#include "ProgressSystem.h"

#include <QJsonObject>
#include <QColor>
#include <QInputDialog>

#include <optional>
CMediaModel::CMediaModel( std::shared_ptr< CSettings > settings, std::shared_ptr< CServerModel > serverModel, QObject * parent ) :
    QAbstractTableModel( parent ),
    fSettings( settings ),
    fServerModel( serverModel ),
    fMergeSystem( new CMergeMedia )
{
    connect( this, &CMediaModel::dataChanged, this, &CMediaModel::sigMediaChanged );
    connect( this, &CMediaModel::modelReset, this, &CMediaModel::sigMediaChanged );
}

int CMediaModel::rowCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;

    return static_cast<int>( fData.size() );
}

int CMediaModel::columnsPerServer( bool includeProviders ) const
{
    auto retVal = static_cast<int>( eFirstServerColumn ) + 1;
    if ( includeProviders )
        retVal += static_cast<int>( fProviderNames.size() );
    return retVal;
}

int CMediaModel::columnCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;
    auto retVal = fServerModel->serverCnt() * columnsPerServer();
    return retVal;
}

std::list< int > CMediaModel::columnsForBaseColumn( int baseColumn ) const 
{
    std::list< int > retVal;

    auto providerInfo = getProviderInfoForColumn( baseColumn );
    if ( providerInfo )
        return { baseColumn };

    auto curr = baseColumn;
    while ( curr <= columnCount() )
    {
        retVal.push_back( curr );
        curr += columnsPerServer( false );
    }
    return retVal;
}

QString CMediaModel::serverForColumn( int column ) const
{
    auto providerInfo = getProviderInfoForColumn( column );
    if ( providerInfo )
        return providerInfo.value().first;

    auto serverNum = column / columnsPerServer( false );
    return fServerModel->getServerInfo( serverNum )->keyName();
}

std::list< int > CMediaModel::providerColumns() const
{
    std::list< int > retVal;
    for ( auto && ii : fProviderColumnsByColumn )
        retVal.push_back( ii.first );
    return retVal;
}



std::optional< std::pair< QString, QString > > CMediaModel::getProviderInfoForColumn( int column ) const
{
    auto pos = fProviderColumnsByColumn.find( column );
    if ( pos != fProviderColumnsByColumn.end() )
        return ( *pos ).second;

    return {};
}

bool CMediaModel::hasMediaToProcess() const
{
    for ( auto && ii : fData )
    {
        if ( !ii->canBeSynced() )
            continue;
        if ( !ii->validUserDataEqual() )
            return true;
    }
    return false;
}

QVariant CMediaModel::data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const
{
    if ( !index.isValid() || index.parent().isValid() || ( index.row() >= rowCount() ) )
        return {};


    auto mediaData = fData[ index.row() ];

    if ( role == ECustomRoles::eMediaNameRole )
        return mediaData->name();
    if ( role == ECustomRoles::eDirSortRole )
        return static_cast<int>( fDirSort );
    if ( role == ECustomRoles::ePremiereDateRole )
        return mediaData->premiereDate();
    if ( role == ECustomRoles::eIsProviderColumnRole )
    {
        auto providerInfo = getProviderInfoForColumn( index.column() );
        return providerInfo.operator bool();
    }
    if ( role == ECustomRoles::eIsNameColumnRole )
    {
        return perServerColumn( index.column() ) == eName;
    }
    if ( role == ECustomRoles::eIsPremiereDateColumnRole )
    {
        return perServerColumn( index.column() ) == ePremiereDate;
    }
    if ( role == ECustomRoles::eShowItemRole )
    {
        if ( !mediaData )
            return false;

        auto mediaSyncStatus = mediaData->syncStatus();
        if ( mediaSyncStatus == EMediaSyncStatus::eNoServerPairs )
            return fSettings->showMediaWithIssues();
        if ( mediaSyncStatus == EMediaSyncStatus::eMediaEqualOnValidServers )
            return !fSettings->onlyShowMediaWithDifferences();
        else if ( mediaSyncStatus == EMediaSyncStatus::eMediaNeedsUpdating )
            return true;
        return false;
    }
    if ( role == ECustomRoles::eSeriesNameRole )
    {
        return mediaData->seriesName();
    }
    if ( role == ECustomRoles::eOnServerRole )
    {
        return mediaData->onServer();
    }


    int column = index.column();
    auto serverName = this->serverForColumn( column );

    // reverse for black background
    if ( role == Qt::ForegroundRole )
    {
        auto color = getColor( index, serverName, false );
        if ( !color.isValid() )
            return {};
        return color;
    }

    if ( role == Qt::BackgroundRole )
    {
        auto color = getColor( index, serverName, true );
        if ( !color.isValid() )
            return {};
        return color;
    }

    if ( ( role == Qt::DecorationRole ) && ( perServerColumn( column ) == eName ) )
    {
        return mediaData->getDirectionIcon( serverName );
    }

    if ( role != Qt::DisplayRole )
        return {};

    auto providerInfo = getProviderInfoForColumn( column );
    if ( providerInfo )
    {
        return mediaData->getProviderID( providerInfo.value().second );
    }

    bool isValid = mediaData->isValidForServer( serverName );

    switch ( perServerColumn( column ) )
    {
        case eName: return isValid ? mediaData->name() : tr( "%1 - <Missing from Server>" ).arg( mediaData->name() );
        case eType: return mediaData->mediaType();
        case ePremiereDate:
        {
            if ( ( mediaData->premiereDate().month() ) == 1 && ( mediaData->premiereDate().day() == 1 ) )
                return mediaData->premiereDate().year();
            else
                return mediaData->premiereDate();
        }
        case eMediaID: return isValid ? mediaData->getMediaID( serverName ) : QString();
        case eFavorite: return isValid ? ( mediaData->isFavorite( serverName ) ? "Yes" : "No" )  : QString();
        case ePlayed: return isValid ? ( mediaData->isPlayed( serverName ) ? "Yes" : "No" )  : QString();
        case eLastPlayed: return isValid ? ( mediaData->lastPlayed( serverName ).toString() )  : QString();
        case ePlayCount: return isValid ? QString::number( mediaData->playCount( serverName ) ) : QString();
        case ePlaybackPosition: return isValid ? mediaData->playbackPosition( serverName ) : QString();
        default:
            return {};
    }

    return {};
}

int CMediaModel::perServerColumn( int column ) const
{
    if ( getProviderInfoForColumn( column ) )
        return -1;

    return column % columnsPerServer( false );
}

void CMediaModel::addMediaInfo( const QString & serverName, std::shared_ptr<CMediaData> mediaData, const QJsonObject & mediaInfo )
{
    mediaData->loadData( serverName, mediaInfo );

    fMediaMap[ serverName ][ mediaData->getMediaID( serverName ) ] = mediaData;

    fMergeSystem->addMediaInfo( serverName, mediaData );
    updateMediaData( mediaData );
}

void CMediaModel::updateMediaData( std::shared_ptr< CMediaData > mediaData )
{
    auto pos = fMediaToPos.find( mediaData );
    if ( pos == fMediaToPos.end() )
        return;

    int row = static_cast<int>( ( *pos ).second );
    emit dataChanged( index( row, 0 ), index( row, columnCount() - 1 ) );
}

void CMediaModel::beginBatchLoad()
{
    beginResetModel();
}

void CMediaModel::endBatchLoad()
{
    endResetModel();
}

QVariant CMediaModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    if ( section < 0 || section > columnCount() )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );

    if ( role != Qt::DisplayRole )
        return QAbstractTableModel::headerData( section, orientation, role );

    QString retVal;
    int columnNum = -1;
    auto providerInfo = getProviderInfoForColumn( section );
    if ( providerInfo )
        retVal = providerInfo.value().second;
    else 
    {
        columnNum = perServerColumn( section );

        switch ( columnNum )
        {
        case eName: retVal = "Name"; break;
        case eType: retVal = "Media Type"; break;
        case ePremiereDate: retVal = "Premiere Date"; break;
        case eMediaID: retVal = "ID on Server"; break;
        case eFavorite: retVal = "Is Favorite?"; break;
        case ePlayed: retVal = "Played?"; break;
        case eLastPlayed: retVal = "Last Played"; break;
        case ePlayCount: retVal = "Play Count"; break;
        case ePlaybackPosition: retVal = "Play Position"; break;
        };
    }
    //retVal = QString( "%1 - %2 - %3" ).arg( retVal ).arg( columnNum ).arg( section );
    return retVal;
}

void CMediaModel::clear()
{
    beginResetModel();
    fData.clear();
    fDataMap.clear();
    fMediaToPos.clear();
    fProviderColumnsByColumn.clear();
    fProviderNames.clear();

    fMergeSystem->clear();
    fAllMedia.clear();
    fMediaMap.clear();
    endResetModel();
}

void CMediaModel::updateProviderColumns( std::shared_ptr< CMediaData > mediaData )
{
    for ( auto && ii : mediaData->getProviders() )
    {
        int colCount = this->columnCount();
        auto pos = fProviderNames.find( ii.first );
        if ( pos == fProviderNames.end() )
        {
            auto serverModel = fServerModel;
            beginInsertColumns( QModelIndex(), colCount, colCount + serverModel->serverCnt() - 1 );
            fProviderNames.insert( ii.first );
            for ( int jj = 0; jj < serverModel->serverCnt(); ++jj )
            {
                fProviderColumnsByColumn[ colCount + jj ] = { serverModel->getServerInfo( jj )->keyName(), ii.first }; // gets duplicated lhs vs rhs
            }
            endInsertColumns();
        }
    }
}

void CMediaModel::settingsChanged()
{
    beginResetModel();
    endResetModel();
    emit sigSettingsChanged();
}


std::shared_ptr< CMediaData > CMediaModel::getMediaData( const QModelIndex & idx ) const
{
    if ( !idx.isValid() )
        return {};
    if ( idx.model() != this )
        return {};

    return fData[ idx.row() ];
}

std::shared_ptr< CMediaData > CMediaModel::getMediaDataForID( const QString & serverName, const QString & mediaID ) const
{
    auto pos = fMediaMap.find( serverName );
    if ( pos == fMediaMap.end() )
        return {};

    auto pos2 = ( *pos ).second.find( mediaID );
    if ( pos2 != ( *pos ).second.end() )
        return ( *pos2 ).second;
    return {};
}

std::shared_ptr< CMediaData > CMediaModel::loadMedia( const QString & serverName, const QJsonObject & media )
{
    auto id = media[ "Id" ].toString();
    std::shared_ptr< CMediaData > mediaData;

    auto pos = fMediaMap.find( serverName );
    if ( pos == fMediaMap.end() )
        pos = fMediaMap.insert( std::make_pair( serverName, TMediaIDToMediaData() ) ).first;

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == (*pos).second.end() )
        mediaData = std::make_shared< CMediaData >( media, fServerModel );
    else
        mediaData = ( *pos2 ).second;
    //qDebug() << isLHSServer << mediaData->name();

    mediaData->setMediaID( serverName, id );

    /*
    "UserData": {
    "IsFavorite": false,
    "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
    "PlayCount": 1,
    "PlaybackPositionTicks": 0,
    "Played": true
    }
    */

    addMediaInfo( serverName, mediaData, media );
    return mediaData;
}

void CMediaModel::removeMedia( const QString & serverName, const std::shared_ptr< CMediaData > & mediaData )
{
    auto pos = fMediaMap.find( serverName );
    if ( pos != fMediaMap.end() )
    {
        auto mediaID = mediaData->getMediaID( serverName );
        auto pos2 = ( *pos ).second.find( mediaID );
        if ( pos2 != ( *pos ).second.end() )
            ( *pos ).second.erase( pos2 );

        auto pos3 = fMediaToPos.find( mediaData );
        if ( pos3 != fMediaToPos.end() )
            fMediaToPos.erase( pos3 );
    }
    fMergeSystem->removeMedia( serverName, mediaData );
}

std::shared_ptr< CMediaData > CMediaModel::reloadMedia( const QString & serverName, const QJsonObject & media, const QString & mediaID )
{
    auto mediaData = getMediaDataForID( serverName, mediaID );
    if ( !mediaData )
        return {};

    if ( media.find( "UserData" ) == media.end() )
        return {};


    addMediaInfo( serverName, mediaData, media );
    emit sigPendingMediaUpdate();
    return mediaData;
}

bool CMediaModel::mergeMedia( std::shared_ptr< CProgressSystem > progressSystem )
{
    if ( fMergeSystem->merge( progressSystem ) )
    {
        std::tie( fAllMedia, fMediaMap ) = fMergeSystem->getMergedData( progressSystem );

        loadMergedMedia( progressSystem );
    }
    else
    {
        clear();
    }
    return !progressSystem->wasCanceled();
}

void CMediaModel::loadMergedMedia( std::shared_ptr<CProgressSystem> progressSystem )
{
    progressSystem->pushState();
    progressSystem->setTitle( tr( "Loading merged media data" ) );
    progressSystem->setMaximum( static_cast<int>( fAllMedia.size() ) );
    progressSystem->setValue( 0 );
    beginResetModel();
    fData.reserve( fAllMedia.size() );
    for ( auto && ii : fAllMedia )
    {
        addMedia( ii, false );
    }
    endResetModel();
    progressSystem->popState();
}

void CMediaModel::addMedia( const std::shared_ptr<CMediaData> & media, bool emitUpdate )
{
    if ( emitUpdate )
        beginInsertRows(QModelIndex(), static_cast<int>(fData.size()), static_cast<int>(fData.size()));
    fMediaToPos[media] = fData.size();
    fData.push_back(media);
    fDataMap[SDummyMovie::nameKey(media->name())] = media;
    fDataMap[SDummyMovie::nameKey(media->originalTitle())] = media;
    updateProviderColumns(media);
    if ( emitUpdate )
        endInsertRows();
}

void CMediaModel::addMovieStub( const QString & name, int year )
{
    auto pos = fDataMap.find(SDummyMovie::nameKey(name));
    if (pos != fDataMap.end())
        return;

    for ( auto && ii : fData )
    {
        if ( ( SDummyMovie::nameKey( ii->name() ) == SDummyMovie::nameKey(name ) ) )
            return;

        if ((SDummyMovie::nameKey(ii->originalTitle()) == SDummyMovie::nameKey(name)))
            return;
    }

    auto mediaData = std::make_shared< CMediaData >( name, year, "Movie" );
    addMedia(mediaData, true);
}

std::shared_ptr< CMediaCollection > CMediaModel::findCollection( const QString & serverName, const QString & name )
{
    auto pos = fCollections.find( serverName );
    if ( pos == fCollections.end() )
        return {};
    for ( auto && ii : ( *pos ).second )
    {
        if ( ii->name() == name )
            return ii;
    }
    return {};
}

std::unordered_set< QString >  CMediaModel::getKnownShows() const
{
    std::unordered_set< QString > knownShows;
    for ( auto && ii : fAllMedia )
    {
        if ( ii->mediaType() != "Episode" )
            continue;;
        knownShows.insert( ii->seriesName() );
    }
    return knownShows;
}

std::shared_ptr< CMediaData > CMediaModel::findMedia( const QString & name, int year ) const
{
    for ( auto && ii : fAllMedia )
    {
        if (ii->name().contains("Freaks"))
            int xyz = 0;
        if ( ii->isMatch( name, year ) )
            return ii;
    }
    return {};
}

QVariant CMediaModel::getColor( const QModelIndex & index, const QString & serverName, bool background ) const
{
    if ( !index.isValid() )
        return {};

    if ( index.column() > fServerModel->serverCnt() * columnsPerServer( false ) )
        return {};
    auto mediaData = fData[ index.row() ];

    if ( !mediaData->isValidForServer( serverName ) )
    {
        switch ( perServerColumn( index.column() ) )
        {
        case eName:
        case eType:
            return {};
        case eMediaID:
        case eFavorite:
        case ePlayed:
        case eLastPlayed:
        case ePlayCount:
        case ePlaybackPosition:
            return fSettings->dataMissingColor( background );
            break;
        default:
            return {};
        }
    }

    if ( !mediaData->validUserDataEqual() )
    {
        bool dataSame = false;

        switch ( perServerColumn( index.column() ) )
        {
            case eName:
                dataSame = !mediaData->canBeSynced();
                break;
            case eFavorite:
                dataSame = mediaData->allFavoriteEqual();
                break;
            case ePlayed:
                dataSame = mediaData->allPlayedEqual();
                break;
            case eLastPlayed:
                dataSame = mediaData->allLastPlayedEqual();
                break;
            case ePlayCount:
                dataSame = mediaData->allPlayCountEqual();
                break;
            case ePlaybackPosition:
                dataSame = mediaData->allPlaybackPositionTicksEqual();
                break;
            case eMediaID:
            default:
                return {};
        }


        if ( dataSame )
            return {};

        auto older = fSettings->mediaDestColor( background );
        auto newer = fSettings->mediaSourceColor( background );
        auto serverName = this->serverForColumn( index.column() );

        auto isOlder = mediaData->needsUpdating( serverName );

        return isOlder ? older : newer;
    }
    return {};
}

SMediaSummary::SMediaSummary( std::shared_ptr< CMediaModel > model )
{
    auto serverModel = model->fServerModel;
    for ( auto && ii : model->fData )
    {
        fTotalMedia++;
        for ( auto && serverInfo : *serverModel )
        {
            if ( !serverInfo->isEnabled() )
                continue;

            if ( !ii->isValidForServer( serverInfo->keyName() ) )
            {
                fMissingData[ serverInfo->displayName() ]++;
                continue;
            }

            if ( ii->needsUpdating( serverInfo->keyName() ) )
            {
                fNeedsUpdating[ serverInfo->displayName() ]++;
            }
        }
    }
}


QString SMediaSummary::getSummaryText() const
{
    QStringList allMsgs = { QObject::tr( "%1 Total Items" ).arg( fTotalMedia ) };

    QStringList missingStringList;
    for ( auto && ii : fMissingData )
    {
        missingStringList << QObject::tr( "%1 from %2" ).arg( ii.second ).arg( ii.first );
    }
    auto missingString = missingStringList.join( ", " );
    if ( !missingString.isEmpty() )
    {
        allMsgs << QObject::tr( "Missing Media: %1" ).arg( missingString );
    }

    QStringList mediaUpdateStringList;
    for ( auto && ii : fNeedsUpdating )
    {
        mediaUpdateStringList << QObject::tr( "%1 from %2" ).arg( ii.second ).arg( ii.first );
    }
    auto mediaUpdateString = mediaUpdateStringList.join( ", " );
    if ( !mediaUpdateString.isEmpty() )
        allMsgs << QObject::tr( "Media Needing Update: %1" ).arg( mediaUpdateString );

    auto retVal = QObject::tr( "Media Summary: %1" ).arg( allMsgs.join( ", " ) );

    return retVal;
}

CMediaFilterModel::CMediaFilterModel( QObject * parent ) :
    QSortFilterProxyModel( parent )
{
    setDynamicSortFilter( false );
}

bool CMediaFilterModel::filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const
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

bool CMediaFilterModel::lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const
{
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}

CMediaMissingFilterModel::CMediaMissingFilterModel( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QSortFilterProxyModel( parent ),
    fSettings( settings )
{
    fRegEx = fSettings->ignoreShowRegEx();
    setDynamicSortFilter( false );
    connect( this, &QSortFilterProxyModel::sourceModelChanged,
        [ this ]()
        {
            connect( dynamic_cast< CMediaModel * >( sourceModel() ), &CMediaModel::sigSettingsChanged, 
                [ this ]()
                {
                    fRegEx = fSettings->ignoreShowRegEx();
                    invalidateFilter();
                } );
        } );
}

void CMediaMissingFilterModel::setShowFilter( const QString & filter )
{
    fShowFilter = filter;
    invalidateFilter();
}

bool CMediaMissingFilterModel::filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const
{
    if ( !fRegEx.isValid() )
        return true;

    if ( !sourceModel() )
        return true;
    auto childIdx = sourceModel()->index( source_row, 0, source_parent );
    auto seriesName = childIdx.data( CMediaModel::eSeriesNameRole ).toString();
    if ( seriesName.isEmpty() )
        return false;

    if ( !fShowFilter.isEmpty() )
        return seriesName == fShowFilter;

    auto match = fRegEx.match( seriesName );
    bool isMatch = ( match.hasMatch() && ( match.captured( 0 ).length() == seriesName.length() ) );
    return !isMatch;
}

bool CMediaMissingFilterModel::filterAcceptsColumn( int source_column, const QModelIndex & source_parent ) const
{
    auto idx = sourceModel()->index( 0, source_column, source_parent );
    return !idx.data( CMediaModel::ECustomRoles::eIsProviderColumnRole ).toBool();
}

void CMediaMissingFilterModel::sort( int column, Qt::SortOrder order /*= Qt::AscendingOrder */ )
{
    QSortFilterProxyModel::sort( column, order );
}

bool CMediaMissingFilterModel::lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const
{
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}

QVariant CMediaMissingFilterModel::data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const
{
    if ( ( role != Qt::ForegroundRole ) && ( role != Qt::BackgroundRole ) )
        return QSortFilterProxyModel::data( index, role );

    auto premiereDate = index.data( CMediaModel::ECustomRoles::ePremiereDateRole ).toDate();
    if ( premiereDate > QDate::currentDate() )
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

//////
CMovieSearchFilterModel::CMovieSearchFilterModel( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QSortFilterProxyModel( parent ),
    fSettings( settings )
{
    setDynamicSortFilter( false );
    connect( this, &QSortFilterProxyModel::sourceModelChanged,
             [this]()
             {
                 connect( dynamic_cast<CMediaModel *>( sourceModel() ), &CMediaModel::sigSettingsChanged,
                          [this]()
                          {
                              invalidateFilter();
                          } );
             } );
}


void CMovieSearchFilterModel::addSearchMovie( const QString & name, int year, bool invalidate )
{
    fSearchForMovies.insert( { name, year } );
    if ( invalidate )
        invalidateFilter();
}

void CMovieSearchFilterModel::finishedAddingSearchMovies()
{
    invalidateFilter();
}

void CMovieSearchFilterModel::addMissingMoviesToSourceModel()
{
    auto mediaModel = dynamic_cast<CMediaModel *>( sourceModel() );
    if ( !mediaModel )
        return;

    for ( auto && ii : fSearchForMovies )
    {
        mediaModel->addMovieStub( ii.fName, ii.fYear );
    }
}

bool CMovieSearchFilterModel::inSearchForMovie(const QString& name, int year) const
{
    auto pos = fSearchForMovies.find({ name, year });
    return pos != fSearchForMovies.end();
}

bool CMovieSearchFilterModel::inSearchForMovie(const QString& name) const
{
    for( auto && ii : fSearchForMovies )
    {
        if (ii.isMovie(name))
            return true;
    }
    return false;
}

bool CMovieSearchFilterModel::filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const
{
    if ( !sourceModel() )
        return true;
    auto childIdx = sourceModel()->index( source_row, 0, source_parent );
    auto name = childIdx.data( CMediaModel::eMediaNameRole ).toString();
    auto date = childIdx.data( CMediaModel::ePremiereDateRole ).toDate();
    if ( name.isEmpty() || !date.isValid() )
        return false;

    auto year = date.year();
    if (!inSearchForMovie(name, year) && !inSearchForMovie(name))
        return false;

    auto onServer = childIdx.data( CMediaModel::ECustomRoles::eOnServerRole ).toBool();
    if ( !onServer )
        return true;
    return false;
}

bool CMovieSearchFilterModel::filterAcceptsColumn( int source_column, const QModelIndex & source_parent ) const
{
    auto idx = sourceModel()->index( 0, source_column, source_parent );
    if ( idx.isValid() )
    {
        bool retVal = ( source_column != 1 ) && !idx.data( CMediaModel::ECustomRoles::eIsProviderColumnRole ).toBool();
        if ( retVal )
            retVal = idx.data( CMediaModel::ECustomRoles::eIsNameColumnRole ).toBool() || idx.data( CMediaModel::ECustomRoles::eIsPremiereDateColumnRole ).toBool();
        //qDebug() << source_column << retVal;
        return retVal;
    }
    return true;
}

void CMovieSearchFilterModel::sort( int column, Qt::SortOrder order /*= Qt::AscendingOrder */ )
{
    QSortFilterProxyModel::sort( column, order );
}

bool CMovieSearchFilterModel::lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const
{
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}

QVariant CMovieSearchFilterModel::data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const
{
    if ( ( role != Qt::ForegroundRole ) && ( role != Qt::BackgroundRole ) )
        return QSortFilterProxyModel::data( index, role );

    auto onServer = index.data( CMediaModel::ECustomRoles::eOnServerRole ).toBool();
    if ( onServer )
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

QString CMovieSearchFilterModel::summary() const
{
    return tr( "Searching for: %1 Missing: %2" ).arg( static_cast<int>( fSearchForMovies.size() ) ).arg( rowCount() );
}

CCollectionsModel::CCollectionsModel( std::shared_ptr< CMediaModel > mediaModel ) : 
    QAbstractItemModel( nullptr ),
    fMediaModel( mediaModel )
{
}

SIndexPtr * CCollectionsModel::idxPtr( void * ptr, bool isCollection ) const
{
    auto pos = fIndexPtrs.find( ptr );
    if ( pos != fIndexPtrs.end() )
        return ( *pos ).second;
    auto curr = new SIndexPtr( ptr, isCollection );
    fIndexPtrs[ ptr ] = curr;
    return curr;
}

SIndexPtr * CCollectionsModel::idxPtr( SMediaCollectionData * media ) const
{
    return idxPtr( media, false );
}

SIndexPtr * CCollectionsModel::idxPtr( CMediaCollection * mediaCollection ) const
{
    return idxPtr( mediaCollection, true );
}

CMediaCollection * CCollectionsModel::collection( const QModelIndex & idx ) const
{
    auto idxPtr = reinterpret_cast<SIndexPtr*>( idx.internalPointer() );
    if ( !idxPtr )
        return nullptr;
    if ( !idxPtr->fIsCollection )
        return nullptr;
    return reinterpret_cast<CMediaCollection *>( idxPtr->fPtr );
}

SMediaCollectionData * CCollectionsModel::media( const QModelIndex & idx ) const
{
    auto idxPtr = reinterpret_cast<SIndexPtr *>( idx.internalPointer() );
    if ( !idxPtr )
        return nullptr;
    if ( idxPtr->fIsCollection )
        return nullptr;
    return reinterpret_cast<SMediaCollectionData *>( idxPtr->fPtr );
}

QModelIndex CCollectionsModel::index( int row, int column, const QModelIndex & parent /*= QModelIndex() */ ) const
{
    if ( parent.isValid() )
    {
        if ( isMedia( parent ) )
            return {};
        if ( isCollection( parent ) )
        {
            auto parentCollection = collection( parent );
            if ( !parentCollection )
                return {};

            return createIndex( row, column, idxPtr( parentCollection->child( row ).get() ) );
        }
        return {};
    }
    else
        return createIndex( row, column, idxPtr( fCollections[ row ].get() ) );
}

QModelIndex CCollectionsModel::parent( const QModelIndex & child ) const
{
    if ( !child.isValid() )
        return {};
    
    auto collection = this->collection( child );
    auto media = this->media( child );
    Q_ASSERT( collection || media );

    if ( collection )
        return {};

    if ( !media )
        return {};
    return createIndex( media->fCollection->collectionNum(), 0, idxPtr( fCollections[ media->fCollection->collectionNum() ].get() ) );
}

int CCollectionsModel::rowCount( const QModelIndex & parent /*= QModelIndex() */ ) const
{
    if ( parent.isValid() )
    {
        if ( parent.column() != 0 )
            return 0;

        if ( this->isMedia( parent ) )
            return 0;

        auto parentCollection = collection( parent );
        if ( !parentCollection )
            return 0;

        auto retVal = parentCollection->childCount();
        return retVal;
    }
    else
    {
        return static_cast<int>( fCollections.size() );
    }
}

bool CCollectionsModel::isMedia( const QModelIndex & parent ) const
{
    return parent.isValid() && parent.parent().isValid();
}

bool CCollectionsModel::isCollection( const QModelIndex & parent ) const
{
    return !parent.isValid() || !parent.parent().isValid();
}

QString CCollectionsModel::summary() const
{
    int numMovies = 0;
    int numMissing = 0;
    for ( auto && ii : fCollections )
    {
        numMovies += ii->numMovies();
        numMissing += ii->numMissing();
    }

    return tr( "Searching for: %1 Missing: %2" ).arg( numMovies ).arg( numMissing );
}

void CCollectionsModel::clear()
{
    beginResetModel();
    fCollections.clear();
    fIndexPtrs.clear();
    endResetModel();
}

void CCollectionsModel::updateCollections( const QString & serverName, std::shared_ptr< CMediaModel > model )
{
    auto update = false;
    for ( auto && ii : fCollections )
    {
        auto realCollection = fMediaModel->findCollection( serverName, ii->name() );
        if ( realCollection )
            update = ii->updateWithRealCollection( realCollection ) || update;
    }
    if ( update )
    {
        beginResetModel();
        endResetModel();

    }
}

void CCollectionsModel::createCollections( std::shared_ptr<const CServerInfo> serverInfo, std::shared_ptr< CSyncSystem > syncSystem, QWidget * parent )
{
    for ( auto && ii : fCollections )
    {
        if ( ii->isUnNamed() )
        {
            bool aOK = false;
            auto collectionName = QInputDialog::getText(parent, tr("Unnamed Collection"), tr("What do you want to name the Collection?"), QLineEdit::Normal, ii->fileBaseName(), &aOK);
            if (!aOK || collectionName.isEmpty())
                return;
            ii->setName( collectionName );
        }
        if ( !ii->collectionExists() )
        {
            ii->createCollection( serverInfo, syncSystem );
        }
    }
}

void CCollectionsModel::slotMediaModelDataChanged()
{
    bool changed = false;
    for ( auto && ii : fCollections )
        changed = ii->updateMedia( fMediaModel ) || changed;
    if ( changed )
    {
        beginResetModel();
        endResetModel();
    }
}

int CCollectionsModel::columnCount( const QModelIndex & parent /*= QModelIndex()*/  ) const
{
    if ( !parent.isValid() || ( parent.column() == 0 ) )
        return 4;
    return 0;
}

QVariant CCollectionsModel::data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const
{
    if ( !index.isValid() )
        return {};

    if ( role == Qt::DisplayRole )
    {
        if ( isCollection( index ) )
        {
            auto collection = this->collection( index );
            if ( !collection )
                return {};
            return collection->data( index.column(), role );
        }
        else if ( isMedia( index ) )
        {
            auto media = this->media( index );
            if ( !media )
                return {};
            return media->data( index.column(), role );
        }
    }
    return {};
}

bool CCollectionsModel::hasChildren( const QModelIndex & parent ) const
{
    return QAbstractItemModel::hasChildren( parent );
}


QVariant CCollectionsModel::headerData( int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */ ) const
{
    if ( ( orientation != Qt::Orientation::Horizontal ) || ( role != Qt::DisplayRole ) )
        return QAbstractItemModel::headerData( section, orientation, role );
    switch ( section )
    {
        case 0: return tr( "Name" );
        case 1: return tr( "Year" );
        case 2: return tr( "Collection Exists" );
        case 3: return tr( "Missing Media" );
    }
    return {};
}

void CMediaModel::addCollection( const QString & serverName, const QString & name, const QString & id, const std::list< std::shared_ptr< CMediaData > > & items )
{
    auto collection = std::make_shared< CMediaCollection >( serverName, name, id, static_cast<int>( fCollections[ serverName ].size() ) );
    fCollections[ serverName ].push_back( collection );
    collection->setItems( items );
}

std::pair< QModelIndex, std::shared_ptr< CMediaCollection > > CCollectionsModel::addCollection( const QString & server, const QString & name )
{
    beginInsertRows( QModelIndex(), rowCount(), rowCount() );
    auto curr = std::make_shared< CMediaCollection >( server, name, QString(), static_cast< int >( fCollections.size() ) );
    fCollections.push_back( curr );
    endInsertRows();
    auto idx = index( rowCount() - 1, 0, {} );

    Q_ASSERT( curr.get() == this->collection( idx ) );
    return { idx, curr };
}

std::shared_ptr< SMediaCollectionData > CCollectionsModel::addMovie( const QString & name, int year, const QModelIndex & collectionIndex, int rank )
{
    Q_ASSERT( collectionIndex.isValid() );
    if ( !collectionIndex.isValid() )
        return {};

    auto collection = this->collection( collectionIndex );
    Q_ASSERT( collection );
    if ( !collection )
        return {};

    bool sizeIncreased = ( rank == -1 ) || ( rank >= collection->childCount() );
    if ( sizeIncreased )
        beginInsertRows( collectionIndex, collection->childCount(), ( rank > 0 ) ? ( rank - 1 ) : collection->childCount() );
    auto retVal = collection->addMovie( name, year, rank );
    if ( sizeIncreased )
        endInsertRows();
    else
        emit dataChanged( index( rank - 1, 0, collectionIndex ), index( rank - 1, columnCount( collectionIndex ) - 1, collectionIndex ) );
    Q_ASSERT( hasChildren( collectionIndex ) );
    
    auto rc = rowCount( collectionIndex );
    if ( sizeIncreased )
    {
        Q_ASSERT( (rank == -1 ) || ( rc == ( rank ) ) );
    }
    else
        Q_ASSERT( ( rank + 1 ) <= rc );
    return retVal;
}

SIndexPtr::SIndexPtr( void * ptr, bool isCollection ) :
    fPtr( ptr ),
    fIsCollection( isCollection )
{
    Q_ASSERT( ptr );
}

QString SDummyMovie::nameKey(const QString& name)
{
    static std::unordered_map< QString, QString > sCache;
    auto pos = sCache.find(name);
    if (pos != sCache.end())
        return (*pos).second;

    auto retVal = name.toLower().remove( QRegularExpression( "[^a-zA-Z0-9 ]" ) );

    auto startsWith = QStringList()
        << "the"
        << "national lampoons"
        << "monty pythons"
        ;
    for( auto && ii : startsWith )
    {
        if ( retVal.startsWith( ii ) )
            retVal = retVal.mid( ii.length() );
    }

    retVal = retVal.trimmed().remove(QRegularExpression(R"(episode \d+)"));
    retVal = retVal.trimmed().remove(QRegularExpression(R"(episode [ivx]+)"));
    retVal = retVal.trimmed().replace( QRegularExpression( "[ ]{2,}" ), " " );

    sCache[name] = retVal;
    return retVal;
}
