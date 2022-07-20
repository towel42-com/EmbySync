#include "MediaModel.h"
#include "MediaData.h"

#include "Settings.h"

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

        if ( !fSettings->onlyShowMediaWithDifferences() )
            return true;

        if ( mediaData->hasMissingInfo() )
            return false;

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

    if ( role != Qt::DisplayRole )
        return {};

    if ( index.column() == eDirection )
    {
        return mediaData->getDirectionLabel();
    }

    auto pos = fProviderColumnsByColumn.find( index.column() );
    if ( pos != fProviderColumnsByColumn.end() )
    {
        if ( ( *pos ).second.first ) // lhs
            return mediaData->getProviderID( ( *pos ).second.second );
        else
            return mediaData->getProviderID( ( *pos ).second.second );
    }

    switch ( column )
    {
        case eLHSName: return mediaData->name();
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

QVariant CMediaModel::getColor( const QModelIndex & index, bool background ) const
{
    auto mediaData = fData[ index.row() ];

    if ( mediaData->hasMissingInfo() )
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
    if ( role != Qt::DisplayRole )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );

    if ( section == eDirection )
    {
        switch ( fDirSort )
        {
            case EDirSort::eNoSort:
                return QString();
            case EDirSort::eLeftToRight:
                return ">>>";
            case EDirSort::eRightToLeft:
                return "<<<";
            case EDirSort::eEqual:
                return "   =";
        }

        return "";
    }

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

CMediaModel::EDirSort CMediaModel::nextDirSort()
{
    if ( fDirSort == eRightToLeft )
        fDirSort = eNoSort;
    fDirSort = static_cast<EDirSort>( fDirSort + 1 );
    emit headerDataChanged( Qt::Horizontal, EColumns::eDirection, EColumns::eDirection );
    return fDirSort;
}

void CMediaModel::clear()
{
    beginResetModel();
    fData.clear();
    fMediaToPos.clear();
    fProviderColumnsByColumn.clear();
    fProviderColumnsByName.clear();
    endResetModel();
}

void CMediaModel::setMedia( const std::unordered_set< std::shared_ptr< CMediaData > > & media )
{
    beginResetModel();
    fData.reserve( media.size() );
    for ( auto && ii : media )
    {
        fMediaToPos[ ii ] = fData.size();
        fData.push_back( ii );
        updateProviderColumns( ii );
    }
    endResetModel();
}

void CMediaModel::addMedia( std::shared_ptr< CMediaData > mediaData )
{
    beginInsertRows( QModelIndex(), static_cast<int>( fData.size() ), static_cast<int>( fData.size() ) );
    fData.push_back( mediaData );
    updateProviderColumns( mediaData );
    endInsertRows();
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
    if ( column < CMediaModel::eDirection )
        return true;
    if ( column == CMediaModel::eDirection )
        return false;
    if ( column <= CMediaModel::eRHSPlaybackPosition )
        return false;

    auto pos = fProviderColumnsByColumn.find( column );
    if ( pos == fProviderColumnsByColumn.end() )
        return false;
    return ( *pos ).second.first;
}

bool CMediaModel::isRHSColumn( int column ) const
{
    if ( column == CMediaModel::eDirection )
        return false;
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
        if ( ii->hasMissingInfo() )
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

CMediaFilterModel::CMediaFilterModel( QObject * parent ) :
    QSortFilterProxyModel( parent )
{

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
    if ( column == CMediaModel::EColumns::eDirection )
    {
        dynamic_cast< CMediaModel * >( sourceModel() )->nextDirSort();
        order = Qt::AscendingOrder;
    }

    QSortFilterProxyModel::sort( column, order );
}

bool CMediaFilterModel::lessThan( const QModelIndex & source_left, const QModelIndex & source_right ) const
{
    if ( sortColumn() == CMediaModel::EColumns::eDirection )
    {
        bool retVal1 = dirLessThan( source_left, source_right );
        bool retVal2 = dirLessThan( source_right, source_left );
        if ( retVal1 == retVal2 )
            int xyz = 0;
        Q_ASSERT( retVal1 != retVal2 );

        return retVal1;
      //switch ( dirSort )
        //{
        //    case CMediaModel::EDirSort::eLeftToRight:
        //    {
        //        if ( ( leftValue == CMediaModel::EDirSort::eLeftToRight ) && ( rightValue != CMediaModel::EDirSort::eLeftToRight ) )
        //             return true;
        //        break;
        //    }
        //    case CMediaModel::EDirSort::eRightToLeft:
        //    {
        //        if ( ( leftValue == CMediaModel::EDirSort::eRightToLeft ) && ( rightValue != CMediaModel::EDirSort::eRightToLeft ) )
        //            return true;
        //        break;
        //    }
        //    case CMediaModel::EDirSort::eEqual:
        //    {
        //        if ( ( leftValue == CMediaModel::EDirSort::eEqual ) && ( rightValue != CMediaModel::EDirSort::eEqual ) )
        //            return true;
        //        break;
        //    }
        //    case CMediaModel::EDirSort::eNoSort:
        //        break;
        //}
        //return source_left.row() < source_right.row();
    }
    else
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


