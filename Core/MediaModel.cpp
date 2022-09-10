#include "MediaModel.h"
#include "MediaData.h"
#include "MergeMedia.h"

#include "Settings.h"
#include "ServerInfo.h"
#include "ProgressSystem.h"

#include <QJsonObject>
#include <QColor>

#include <optional>
CMediaModel::CMediaModel( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QAbstractTableModel( parent ),
    fSettings( settings ),
    fMergeSystem( new CMergeMedia )
{

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
    auto retVal = fSettings->serverCnt() * columnsPerServer();
    return retVal;
}

QString CMediaModel::serverNameForColumn( int column ) const
{
    auto providerInfo = getProviderInfoForColumn( column );
    if ( providerInfo )
        return providerInfo.value().first;

    auto serverNum = column / columnsPerServer( false );
    return fSettings->serverInfo( serverNum )->keyName();
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

    int column = index.column();
    auto serverName = this->serverNameForColumn( column );

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

    if ( role == ECustomRoles::eServerNameForColumnRole )
    {
        return serverName;
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
    mediaData->loadUserDataFromJSON( serverName, mediaInfo );

    fMediaMap[ serverName ][ mediaData->getMediaID( serverName ) ] = mediaData;

    fMergeSystem->addMediaInfo( serverName, mediaData );
}

QVariant CMediaModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    if ( section < 0 || section > columnCount() )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );

    if ( role != Qt::DisplayRole )
        return QAbstractTableModel::headerData( section, orientation, role );

    auto providerInfo = getProviderInfoForColumn( section );
    if ( providerInfo )
        return providerInfo.value().second;

    auto columnNum = perServerColumn( section );

    switch ( columnNum )
    {
        case eName: return "Name";
        case eType: return "Media Type";
        case eMediaID: return "ID on Server";
        case eFavorite: return "Is Favorite?";
        case ePlayed: return "Played?";
        case eLastPlayed: return "Last Played";
        case ePlayCount: return "Play Count";
        case ePlaybackPosition: return "Play Position";
    };

    return {};
}

void CMediaModel::clear()
{
    beginResetModel();
    fData.clear();
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
            beginInsertColumns( QModelIndex(), colCount, colCount + fSettings->serverCnt() - 1 );
            fProviderNames.insert( ii.first );
            for ( int jj = 0; jj < fSettings->serverCnt(); ++jj )
            {
                fProviderColumnsByColumn[ colCount + jj ] = { fSettings->serverInfo( jj )->keyName(), ii.first }; // gets duplicated lhs vs rhs
            }
            endInsertColumns();
        }
    }
}

void CMediaModel::settingsChanged()
{
    beginResetModel();
    endResetModel();
}


std::shared_ptr< CMediaData > CMediaModel::getMediaData( const QModelIndex & idx ) const
{
    if ( !idx.isValid() )
        return {};
    if ( idx.model() != this )
        return {};

    return fData[ idx.row() ];
}

void CMediaModel::updateMediaData( std::shared_ptr< CMediaData > mediaData )
{
    auto pos = fMediaToPos.find( mediaData );
    if ( pos == fMediaToPos.end() )
        return;

    int row = static_cast<int>( ( *pos ).second );
    emit dataChanged( index( row, 0 ), index( row, columnCount() - 1 ) );
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

std::shared_ptr< CMediaData > CMediaModel::loadMedia( const QString & serverName, const QJsonObject & media )
{
    auto id = media[ "Id" ].toString();
    std::shared_ptr< CMediaData > mediaData;

    auto pos = fMediaMap.find( serverName );
    if ( pos == fMediaMap.end() )
        pos = fMediaMap.insert( std::make_pair( serverName, TMediaIDToMediaData() ) ).first;

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == (*pos).second.end() )
        mediaData = std::make_shared< CMediaData >( media, fSettings );
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

std::shared_ptr< CMediaData > CMediaModel::reloadMedia( const QString & serverName, const QJsonObject & media, const QString & mediaID )
{
    auto mediaData = getMediaDataForID( serverName, mediaID );
    if ( !mediaData )
        return {};

    if ( media.find( "UserData" ) == media.end() )
        return {};


    addMediaInfo( serverName, mediaData, media );
    updateMediaData( mediaData );
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
        fMediaToPos[ ii ] = fData.size();
        fData.push_back( ii );
        updateProviderColumns( ii );
    }
    endResetModel();
    progressSystem->popState();
}

QVariant CMediaModel::getColor( const QModelIndex & index, const QString & serverName, bool background ) const
{
    if ( !index.isValid() )
        return {};

    if ( index.column() > fSettings->serverCnt() * columnsPerServer( false ) )
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
        auto serverName = this->serverNameForColumn( index.column() );

        auto isOlder = mediaData->needsUpdating( serverName );

        return isOlder ? older : newer;
    }
    return {};
}

SMediaSummary::SMediaSummary( std::shared_ptr< CMediaModel > model )
{
    for ( auto && ii : model->fData )
    {
        fTotalMedia++;
        for ( int jj = 0; jj < model->fSettings->serverCnt(); ++jj )
        {
            auto serverInfo = model->fSettings->serverInfo( jj );
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
