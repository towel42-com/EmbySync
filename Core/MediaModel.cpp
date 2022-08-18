#include "MediaModel.h"
#include "MediaData.h"
#include "MergeMedia.h"

#include "Settings.h"
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
    auto retVal = static_cast<int>( ePlaybackPosition ) + 1;
    if ( includeProviders )
        retVal += static_cast<int>( fProviderNames.size() );
    return retVal;
}

bool CMediaModel::isFirstColumnOfServer( int columnNum ) const
{
    if ( getProviderInfoForColumn( columnNum ) )
        return false;

    return ( columnNum % columnsPerServer( false ) ) == 0;
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
    return fSettings->serverKeyName( serverNum );
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
        if ( ii->canBeSynced() )
            continue;
        if ( !ii->userDataEqual() )
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

        if ( !mediaData->canBeSynced() )
            return fSettings->showMediaWithIssues();

        if ( !fSettings->onlyShowMediaWithDifferences() )
            return true;

        return !mediaData->userDataEqual();
    }

    // reverse for black background
    if ( role == Qt::ForegroundRole )
    {
        auto color = getColor( index, false );
        if ( !color.isValid() )
            return {};
        return color;
    }

    if ( role == Qt::BackgroundRole )
    {
        auto color = getColor( index, true );
        if ( !color.isValid() )
            return {};
        return color;
    }

    int column = index.column();
    if ( ( role == Qt::DecorationRole ) )
        int xyz = 0;
    if ( ( role == Qt::DecorationRole ) && isFirstColumnOfServer( index.column() ) )
    {
        return mediaData->getDirectionIcon();
    }

    auto serverName = this->serverNameForColumn( index.column() );
    if ( role == ECustomRoles::eServerNameForColumnRole )
    {
        return serverName;
    }

    if ( role != Qt::DisplayRole )
        return {};

    if ( !mediaData->isValidForServer( serverName ) )
        return {};

    auto providerInfo = getProviderInfoForColumn( index.column() );
    if ( providerInfo )
    {
        return mediaData->getProviderID( providerInfo.value().second );
    }

    switch ( column % columnsPerServer( false ) )
    {
        case eName: return mediaData->name() + "-" + QString::number( index.column() ) + "-" + QString::number( column );
        case eType: return mediaData->mediaType();
        case eMediaID: return mediaData->getMediaID( serverName );
        case eFavorite: return mediaData->isFavorite( serverName ) ? "Yes" : "No";
        case ePlayed: return mediaData->isPlayed( serverName ) ? "Yes" : "No";
        case eLastPlayed: return mediaData->lastPlayed( serverName ).toString();
        case ePlayCount: return QString::number( mediaData->playCount( serverName ) );
        case ePlaybackPosition: return mediaData->playbackPosition( serverName );
        default:
            return {};
    }

    return {};
}

void CMediaModel::addMediaInfo( std::shared_ptr<CMediaData> mediaData, const QJsonObject & mediaInfo, const QString & serverName )
{
    mediaData->loadUserDataFromJSON( mediaInfo, serverName );

    fMediaMap[ serverName ][ mediaData->getMediaID( serverName ) ] = mediaData;

    fMergeSystem->addMediaInfo( mediaData, serverName );
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

    auto columnNum = section % columnsPerServer( false );

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
                fProviderColumnsByColumn[ colCount + jj ] = { fSettings->serverKeyName( jj ), ii.first }; // gets duplicated lhs vs rhs
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

std::shared_ptr< CMediaData > CMediaModel::getMediaDataForID( const QString & mediaID, const QString & serverName ) const
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

std::shared_ptr< CMediaData > CMediaModel::loadMedia( const QJsonObject & media, const QString & serverName )
{
    auto id = media[ "Id" ].toString();
    std::shared_ptr< CMediaData > mediaData;

    auto pos = fMediaMap.find( serverName );
    if ( pos == fMediaMap.end() )
        pos = fMediaMap.insert( std::make_pair( serverName, TMediaIDToMediaData() ) ).first;

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == (*pos).second.end() )
        mediaData = std::make_shared< CMediaData >( CMediaData::computeName( media ), media[ "Type" ].toString() );
    else
        mediaData = ( *pos2 ).second;
    //qDebug() << isLHSServer << mediaData->name();

    mediaData->setMediaID( id, serverName );

    /*
    "UserData": {
    "IsFavorite": false,
    "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
    "PlayCount": 1,
    "PlaybackPositionTicks": 0,
    "Played": true
    }
    */

    addMediaInfo( mediaData, media, serverName );
    return mediaData;
}

std::shared_ptr< CMediaData > CMediaModel::reloadMedia( const QJsonObject & media, const QString & mediaID, const QString & serverName )
{
    auto mediaData = getMediaDataForID( mediaID, serverName );
    if ( !mediaData )
        return {};

    if ( media.find( "UserData" ) == media.end() )
        return {};


    addMediaInfo( mediaData, media, serverName );
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

QVariant CMediaModel::getColor( const QModelIndex & index, bool background ) const
{
    auto mediaData = fData[ index.row() ];

    if ( !mediaData->isValidForAllServers() )
        return fSettings->dataMissingColor( background );

    if ( !mediaData->userDataEqual() )
    {
        bool dataSame = false;

        switch ( index.column() % columnsPerServer() )
        {
            case eName:
                dataSame = false;
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

SMediaSummary::SMediaSummary( CMediaModel * model )
{
    for ( auto && ii : model->fData )
    {
        fTotalMedia++;
        for ( int jj = 0; jj < model->fSettings->serverCnt(); ++jj )
        {
            auto serverInfo = model->fSettings->serverInfo( jj );

            if ( !ii->isValidForServer( serverInfo->keyName() ) )
            {
                fMissingData[ serverInfo->friendlyName() ]++;
                continue;
            }

            if ( ii->needsUpdating( serverInfo->keyName() ) )
            {
                fNeedsUpdating[ serverInfo->friendlyName() ]++;
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
