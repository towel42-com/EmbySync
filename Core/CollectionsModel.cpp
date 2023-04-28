#include "CollectionsModel.h"
#include "MediaData.h"
#include "MediaModel.h"

#include <QInputDialog>
#include <QDebug>

CCollectionsModel::CCollectionsModel( std::shared_ptr< CMediaModel > mediaModel ) :
    QAbstractItemModel( nullptr ),
    fMediaModel( mediaModel )
{
}

SIndexPtr *CCollectionsModel::idxPtr( void *ptr, bool isCollection ) const
{
    auto pos = fIndexPtrs.find( ptr );
    if ( pos != fIndexPtrs.end() )
        return ( *pos ).second;
    auto curr = new SIndexPtr( ptr, isCollection );
    fIndexPtrs[ ptr ] = curr;
    return curr;
}

SIndexPtr *CCollectionsModel::idxPtr( SMediaCollectionData *media ) const
{
    return idxPtr( media, false );
}

SIndexPtr *CCollectionsModel::idxPtr( CMediaCollection *mediaCollection ) const
{
    return idxPtr( mediaCollection, true );
}

CMediaCollection *CCollectionsModel::collection( const QModelIndex &idx ) const
{
    auto idxPtr = reinterpret_cast< SIndexPtr * >( idx.internalPointer() );
    if ( !idxPtr )
        return nullptr;
    if ( !idxPtr->fIsCollection )
        return nullptr;
    return reinterpret_cast< CMediaCollection * >( idxPtr->fPtr );
}

SMediaCollectionData *CCollectionsModel::media( const QModelIndex &idx ) const
{
    auto idxPtr = reinterpret_cast< SIndexPtr * >( idx.internalPointer() );
    if ( !idxPtr )
        return nullptr;
    if ( idxPtr->fIsCollection )
        return nullptr;
    return reinterpret_cast< SMediaCollectionData * >( idxPtr->fPtr );
}

QModelIndex CCollectionsModel::index( int row, int column, const QModelIndex &parent /*= QModelIndex() */ ) const
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

QModelIndex CCollectionsModel::parent( const QModelIndex &child ) const
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

int CCollectionsModel::rowCount( const QModelIndex &parent /*= QModelIndex() */ ) const
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
        return static_cast< int >( fCollections.size() );
    }
}

bool CCollectionsModel::isMedia( const QModelIndex &parent ) const
{
    return parent.isValid() && parent.parent().isValid();
}

bool CCollectionsModel::isCollection( const QModelIndex &parent ) const
{
    return !parent.isValid() || !parent.parent().isValid();
}

QString CCollectionsModel::summary() const
{
    int numMovies = 0;
    int numMissing = 0;
    for ( auto &&ii : fCollections )
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
    fCollectionsMap.clear();
    fIndexPtrs.clear();
    endResetModel();
}

void CCollectionsModel::createCollections( std::shared_ptr< const CServerInfo > serverInfo, std::shared_ptr< CSyncSystem > syncSystem, QWidget *parent )
{
    for ( auto &&ii : fCollections )
    {
        if ( ii->isUnNamed() )
        {
            bool aOK = false;
            auto collectionName = QInputDialog::getText( parent, tr( "Unnamed Collection" ), tr( "What do you want to name the Collection?" ), QLineEdit::Normal, ii->fileBaseName(), &aOK );
            if ( !aOK || collectionName.isEmpty() )
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
    for ( auto &&ii : fCollections )
        changed = ii->updateMedia( fMediaModel ) || changed;
    if ( changed )
    {
        beginResetModel();
        endResetModel();
    }
}

int CCollectionsModel::columnCount( const QModelIndex &parent /*= QModelIndex()*/ ) const
{
    if ( !parent.isValid() || ( parent.column() == 0 ) )
        return 4;
    return 0;
}

QVariant CCollectionsModel::data( const QModelIndex &index, int role /*= Qt::DisplayRole */ ) const
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

bool CCollectionsModel::hasChildren( const QModelIndex &parent ) const
{
    return QAbstractItemModel::hasChildren( parent );
}

QVariant CCollectionsModel::headerData( int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */ ) const
{
    if ( ( orientation != Qt::Orientation::Horizontal ) || ( role != Qt::DisplayRole ) )
        return QAbstractItemModel::headerData( section, orientation, role );
    switch ( section )
    {
        case 0:
            return tr( "Name" );
        case 1:
            return tr( "Year" );
        case 2:
            return tr( "Collection Exists" );
        case 3:
            return tr( "Missing Media" );
    }
    return {};
}

SIndexPtr::SIndexPtr( void *ptr, bool isCollection ) :
    fPtr( ptr ),
    fIsCollection( isCollection )
{
    Q_ASSERT( ptr );
}

std::pair< QModelIndex, std::shared_ptr< CMediaCollection > > CCollectionsModel::addCollection( const QString &serverName, const QString &name, const QString &id, const std::list< std::shared_ptr< CMediaData > > &items )
{
    beginInsertRows( QModelIndex(), rowCount(), rowCount() );

    //qDebug() << name;
    auto pos = static_cast< int >( fCollectionsMap[ serverName ].size() );

    auto collection = std::make_shared< CMediaCollection >( serverName, name, id, pos );
    collection->setItems( items );
    fCollectionsMap[ serverName ].push_back( collection );
    fCollections.push_back( collection );

    endInsertRows();

    auto idx = index( rowCount() - 1, 0, {} );
    Q_ASSERT( collection.get() == this->collection( idx ) );
    return { idx, collection };
}

std::pair< QModelIndex, std::shared_ptr< CMediaCollection > > CCollectionsModel::addCollection( const QString &server, const QString &name )
{
    return addCollection( server, name, {}, {} );
}

std::shared_ptr< SMediaCollectionData > CCollectionsModel::addMovie( const QString &name, int year, const std::pair< int, int > &resolution, const QModelIndex &collectionIndex, int rank )
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
    auto retVal = collection->addMovie( name, year, resolution, rank );
    if ( sizeIncreased )
        endInsertRows();
    else
        emit dataChanged( index( rank - 1, 0, collectionIndex ), index( rank - 1, columnCount( collectionIndex ) - 1, collectionIndex ) );
    Q_ASSERT( hasChildren( collectionIndex ) );

    auto rc = rowCount( collectionIndex );
    if ( sizeIncreased )
    {
        Q_ASSERT( ( rank == -1 ) || ( rc == ( rank ) ) );
    }
    else
        Q_ASSERT( ( rank + 1 ) <= rc );
    return retVal;
}

void CCollectionsModel::updateCollections( const QString & /*serverName*/, std::shared_ptr< CMediaModel > model )
{
    auto update = false;
    for ( auto &&ii : fCollections )
    {
        if ( ii->collectionExists() )
            continue;

        for ( auto &&jj : fCollections )
        {
            if ( ii->collectionExists() && ( ii->name() == jj->name() ) )
            {
                update = ii->updateWithRealCollection( jj ) || update;
            }
        }
    }

    beginResetModel();
    endResetModel();
}

CCollectionsFilterModel::CCollectionsFilterModel( QObject *parent ) :
    QSortFilterProxyModel( parent )
{
    setDynamicSortFilter( false );
}

bool CCollectionsFilterModel::filterAcceptsRow( int /*source_row*/, const QModelIndex & /*source_parent*/ ) const
{
    return true;
}

void CCollectionsFilterModel::sort( int column, Qt::SortOrder order /*= Qt::AscendingOrder */ )
{
    QSortFilterProxyModel::sort( column, order );
}

bool CCollectionsFilterModel::lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const
{
    if ( ( this->sortColumn() == 0 ) && source_left.parent().isValid() && source_right.parent().isValid() )
    {
        auto leftYear = source_left.model()->index( source_left.row(), 1, source_left.parent() );
        auto rightYear = source_right.model()->index( source_right.row(), 1, source_right.parent() );
        return QSortFilterProxyModel::lessThan( leftYear, rightYear );
    }
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}
