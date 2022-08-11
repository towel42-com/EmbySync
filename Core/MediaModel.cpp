#include "MediaModel.h"
#include "MediaData.h"

#include "Settings.h"
#include "ProgressSystem.h"

#include <QJsonObject>
#include <QColor>

CMediaModel::CMediaModel( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QAbstractTableModel( parent ),
    fSettings( settings )
{

}

int CMediaModel::rowCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;

    return static_cast<int>( fData.size() );
}

int CMediaModel::columnCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;
    auto retVal = static_cast<int>( eRHSPlaybackPosition ) + 1;
    retVal += static_cast<int>( 2 * fProviderColumnsByName.size() );
    return retVal;
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

        if ( mediaData->isMissingOnEitherServer() )
            return fSettings->showMediaWithIssues();

        if ( !fSettings->onlyShowMediaWithDifferences() )
            return true;

        return !mediaData->userDataEqual();
    }

    if ( role == ECustomRoles::eDirValueRole )
    {
        return mediaData->getDirectionValue();
    }

    bool isLHS = index.column() <= eLHSPlaybackPosition;
    int column = index.column();
    if ( !isLHS )
        column -= eRHSName;

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

    if ( ( role == Qt::DecorationRole ) && ( ( index.column() == eLHSName ) || ( index.column() == eRHSName ) ) )
    {
        return mediaData->getDirectionIcon();
    }

    if ( role != Qt::DisplayRole )
        return {};

    auto pos = fProviderColumnsByColumn.find( index.column() );
    if ( pos != fProviderColumnsByColumn.end() )
    {
        if ( ( *pos ).second.first ) // lhs
            return mediaData->getProviderID( ( *pos ).second.second );
        else
            return mediaData->getProviderID( ( *pos ).second.second );
    }

    if ( mediaData->isMissingOnServer( isLHS ) )
        return {};

    switch ( column )
    {
        case eLHSName: return mediaData->name();
        case eLHSType: return mediaData->mediaType();
        case eLHSMediaID: return mediaData->getMediaID( isLHS );
        case eLHSFavorite: return mediaData->isFavorite( isLHS ) ? "Yes" : "No";
        case eLHSPlayed: return mediaData->isPlayed( isLHS ) ? "Yes" : "No";
        case eLHSLastPlayed: return mediaData->lastPlayed( isLHS ).toString();
        case eLHSPlayCount: return QString::number( mediaData->playCount( isLHS ) );
        case eLHSPlaybackPosition: return mediaData->playbackPosition( isLHS );
        default:
            return {};
    }

    return {};
}

void CMediaModel::addMediaInfo( std::shared_ptr<CMediaData> mediaData, const QJsonObject & mediaInfo, bool isLHSServer )
{
    mediaData->loadUserDataFromJSON( mediaInfo, isLHSServer );
    if ( isLHSServer )
        fLHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;
    else
        fRHSMedia[ mediaData->getMediaID( isLHSServer ) ] = mediaData;

    auto && providers = mediaData->getProviders( true );
    for ( auto && ii : providers )
    {
        if ( isLHSServer )
            fLHSProviderSearchMap[ ii.first ][ ii.second ] = mediaData;
        else
            fRHSProviderSearchMap[ ii.first ][ ii.second ] = mediaData;
    }
}

QVariant CMediaModel::getColor( const QModelIndex & index, bool background ) const
{
    auto mediaData = fData[ index.row() ];

    if ( mediaData->isMissingOnEitherServer() )
        return fSettings->dataMissingColor( background );

    if ( !mediaData->userDataEqual() )
    {
        bool rhsOlder = mediaData->rhsNeedsUpdating();
        auto older = fSettings->mediaDestColor( background );
        auto newer = fSettings->mediaSourceColor( background );

        bool dataSame = false;

        switch ( index.column() )
        {
            case eLHSName:
            case eRHSName:
                dataSame = false;
                break;
            case eLHSFavorite:
            case eRHSFavorite:
                dataSame = mediaData->isFavorite( true ) == mediaData->isFavorite( false );
                break;
            case eLHSPlayed:
            case eRHSPlayed:
                dataSame = mediaData->isPlayed( true ) == mediaData->isPlayed( false );
                break;
            case eLHSLastPlayed:
            case eRHSLastPlayed:
                dataSame = mediaData->lastPlayed( true ) == mediaData->lastPlayed( false );
                break;
            case eRHSPlayCount:
            case eLHSPlayCount:
                dataSame = mediaData->playCount( true ) == mediaData->playCount( false );
                break;
            case eLHSPlaybackPosition:
            case eRHSPlaybackPosition:
                dataSame = mediaData->playbackPositionTicks( true ) == mediaData->playbackPositionTicks( false );
                break;

            case eLHSMediaID:
            case eRHSMediaID:
            default:
                return {};
        }

        if ( dataSame )
            return {};

        if ( isLHSColumn( index.column() ) )
            return rhsOlder ? newer : older;
        else if ( isRHSColumn( index.column() ) )
            return rhsOlder ? older : newer;
    }
    return {};
}

QVariant CMediaModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    if ( section < 0 || section > columnCount() )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );

    if ( role != Qt::DisplayRole )
        return QAbstractTableModel::headerData( section, orientation, role );

    auto pos = fProviderColumnsByColumn.find( section );
    if ( pos != fProviderColumnsByColumn.end() )
        return ( *pos ).second.second;
    section = ( section >= eRHSName ) ? section - eRHSName : section;
    switch ( section )
    {
        case eLHSName: return "Name";
        case eLHSMediaID: return "ID on Server";
        case eLHSFavorite: return "Is Favorite?";
        case eLHSPlayed: return "Played?";
        case eLHSLastPlayed: return "Last Played";
        case eLHSPlayCount: return "Play Count";
        case eLHSPlaybackPosition: return "Play Position";
    };

    return {};
}

void CMediaModel::clear()
{
    beginResetModel();
    fData.clear();
    fMediaToPos.clear();
    fProviderColumnsByColumn.clear();
    fProviderColumnsByName.clear();

    fLHSMedia.clear();
    fRHSMedia.clear();
    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();
    fAllMedia.clear();
    endResetModel();
}

void CMediaModel::updateProviderColumns( std::shared_ptr< CMediaData > mediaData )
{
    for ( auto && ii : mediaData->getProviders() )
    {
        int colCount = this->columnCount();
        auto pos = fProviderColumnsByName.find( ii.first );
        if ( pos == fProviderColumnsByName.end() )
        {
            beginInsertColumns( QModelIndex(), colCount, colCount + 1 );
            fProviderColumnsByName.insert( ii.first );
            fProviderColumnsByColumn[ colCount ] = { true, ii.first }; // gets duplicated lhs vs rhs
            fProviderColumnsByColumn[ colCount + 1 ] = { false, ii.first };
            endInsertColumns();
        }
        colCount = this->columnCount();
    }
}

bool CMediaModel::isLHSColumn( int column ) const
{
    if ( column < CMediaModel::eRHSName )
        return true;
    if ( column <= CMediaModel::eRHSPlaybackPosition )
        return false;

    auto pos = fProviderColumnsByColumn.find( column );
    if ( pos == fProviderColumnsByColumn.end() )
        return false;
    return ( *pos ).second.first;
}

bool CMediaModel::isRHSColumn( int column ) const
{
    return !isLHSColumn( column );
}

CMediaModel::SMediaSummary CMediaModel::settingsChanged()
{
    beginResetModel();
    endResetModel();
    return getMediaSummary();
}


CMediaModel::SMediaSummary CMediaModel::getMediaSummary() const
{
    SMediaSummary retVal;

    for ( auto && ii : fData )
    {
        retVal.fTotalMedia++;
        if ( ii->isMissingOnEitherServer() )
        {
            retVal.fMissingData++;
            continue;
        }

        if ( ii->lhsNeedsUpdating() )
        {
            retVal.fNeedsSyncing++;
            retVal.fLHSNeedsUpdating++;
        }

        if ( ii->rhsNeedsUpdating() )
        {
            retVal.fNeedsSyncing++;
            retVal.fRHSNeedsUpdating++;
        }
    }
    return retVal;
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

std::shared_ptr< CMediaData > CMediaModel::getMediaDataForID( const QString & mediaID, bool isLHSServer ) const
{
    QString name;
    auto && map = isLHSServer ? fLHSMedia : fRHSMedia;
    auto pos = map.find( mediaID );
    if ( pos != map.end() )
        return ( *pos ).second;
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

bool CMediaFilterModel::dirLessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const
{
    auto dirSort = static_cast<CMediaModel::EDirSort>( source_left.data( CMediaModel::ECustomRoles::eDirSortRole ).toInt() );

    auto leftValue = static_cast<CMediaModel::EDirSort>( source_left.data( CMediaModel::ECustomRoles::eDirValueRole ).toInt() );
    auto rightValue = static_cast<CMediaModel::EDirSort>( source_right.data( CMediaModel::ECustomRoles::eDirValueRole ).toInt() );

    if ( leftValue == dirSort )
    {
        if ( rightValue != dirSort )
            return true;
    }

    if ( rightValue == dirSort )
    {
        if ( leftValue != dirSort )
            return false;
    }

    if ( leftValue != rightValue )
        return leftValue < rightValue;

    return source_left.row() < source_right.row();
}

std::shared_ptr< CMediaData > CMediaModel::loadMedia( const QJsonObject & media, bool isLHSServer )
{
    auto id = media[ "Id" ].toString();
    std::shared_ptr< CMediaData > mediaData;
    auto pos = ( isLHSServer ? fLHSMedia.find( id ) : fRHSMedia.find( id ) );
    if ( pos == ( isLHSServer ? fLHSMedia.end() : fRHSMedia.end() ) )
        mediaData = std::make_shared< CMediaData >( CMediaData::computeName( media ), media[ "Type" ].toString() );
    else
        mediaData = ( *pos ).second;
    //qDebug() << isLHSServer << mediaData->name();

    mediaData->setMediaID( id, isLHSServer );

    /*
    "UserData": {
    "IsFavorite": false,
    "LastPlayedDate": "2022-01-15T20:28:39.0000000Z",
    "PlayCount": 1,
    "PlaybackPositionTicks": 0,
    "Played": true
    }
    */

    addMediaInfo( mediaData, media, isLHSServer );
    return mediaData;
}

std::shared_ptr< CMediaData > CMediaModel::reloadMedia( const QJsonObject & media, const QString & mediaID, bool isLHSServer )
{
    auto mediaData = getMediaDataForID( mediaID, isLHSServer );
    if ( !mediaData )
        return {};

    if ( media.find( "UserData" ) == media.end() )
        return {};


    addMediaInfo( mediaData, media, isLHSServer );
    updateMediaData( mediaData );
    emit sigPendingMediaUpdate();
    return mediaData;
}

void CMediaModel::mergeMediaData( TMediaIDToMediaData & lhs, TMediaIDToMediaData & rhs, bool lhsIsLHS, std::shared_ptr< CProgressSystem > progressSystem )
{
    mergeMediaData( lhs, lhsIsLHS, progressSystem );
    mergeMediaData( rhs, !lhsIsLHS, progressSystem );
}

bool CMediaModel::mergeMedia( std::shared_ptr< CProgressSystem > progressSystem )
{
    progressSystem->resetProgress();
    progressSystem->setTitle( tr( "Merging media data" ) );
    progressSystem->setMaximum( static_cast<int>( fLHSMedia.size() * 3 + fRHSMedia.size() * 3 ) );


    mergeMediaData( fLHSMedia, fRHSMedia, true, progressSystem );
    mergeMediaData( fRHSMedia, fLHSMedia, false, progressSystem );

    if ( !progressSystem->wasCanceled() )
    {
        loadMergedData( progressSystem );
    }
    else
    {
        clear();
    }
    fLHSProviderSearchMap.clear();
    fRHSProviderSearchMap.clear();


    return !progressSystem->wasCanceled();
}

void CMediaModel::loadMergedData( std::shared_ptr< CProgressSystem > progressSystem )
{
    for ( auto && ii : fLHSMedia )
    {
        fAllMedia.insert( ii.second );
        progressSystem->incProgress();
    }

    for ( auto && ii : fRHSMedia )
    {
        fAllMedia.insert( ii.second );
        progressSystem->incProgress();
    }

    progressSystem->pushState();
    progressSystem->setTitle( tr( "Loading merged media data" ) );
    progressSystem->setMaximum( static_cast< int >( fAllMedia.size() ) );
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

std::shared_ptr< CMediaData > CMediaModel::findMediaForProvider( const QString & provider, const QString & id, bool lhs ) const
{
    auto && map = lhs ? fLHSProviderSearchMap : fRHSProviderSearchMap;
    auto pos = map.find( provider );
    if ( pos == map.end() )
        return {};

    auto pos2 = ( *pos ).second.find( id );
    if ( pos2 == ( *pos ).second.end() )
        return {};
    return ( *pos2 ).second;
}


void CMediaModel::setMediaForProvider( const QString & providerName, const QString & providerID, std::shared_ptr< CMediaData > mediaData, bool isLHS )
{
    ( isLHS ? fLHSProviderSearchMap : fRHSProviderSearchMap )[ providerName ][ providerID ] = mediaData;
}

void CMediaModel::mergeMediaData( TMediaIDToMediaData & lhs, bool lhsIsLHS, std::shared_ptr< CProgressSystem > progressSystem )
{
    std::unordered_map< std::shared_ptr< CMediaData >, std::shared_ptr< CMediaData > > replacementMap;
    for ( auto && ii : lhs )
    {
        if ( progressSystem->wasCanceled() )
            break;

        progressSystem->incProgress();
        auto mediaData = ii.second;
        if ( !mediaData )
            continue;
        QStringList dupeProviderForMedia;
        for ( auto && jj : mediaData->getProviders( true ) )
        {
            auto providerName = jj.first;
            auto providerID = jj.second;

            auto myMappedMedia = findMediaForProvider( providerName, providerID, lhsIsLHS );
            if ( myMappedMedia != mediaData )
            {
                replacementMap[ mediaData ] = myMappedMedia;
                continue;
            }

            auto otherData = findMediaForProvider( providerName, providerID, !lhsIsLHS );
            if ( otherData != mediaData )
                setMediaForProvider( providerName, providerID, mediaData, !lhsIsLHS );
        }
    }
    for ( auto && ii : replacementMap )
    {
        auto currMediaID = ii.first->getMediaID( lhsIsLHS );
        auto pos = lhs.find( currMediaID );
        lhs.erase( pos );
        ii.second->updateFromOther( ii.first, lhsIsLHS );

        lhs[ currMediaID ] = ii.second;
    }
}
