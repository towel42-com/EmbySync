#include "CollectionModel.h"
#include "MediaModel.h"
#include "MediaData.h"
#include "MediaCollection.h"
#include "SyncSystem.h"

#include <QFile>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMessageBox>
#include <QTimer>

CCollectionModel::CCollectionModel( std::shared_ptr< CMediaModel > mediaModel ) : 
    QAbstractItemModel( nullptr ),
    fMediaModel( mediaModel ),
    fRootItem( new SCollectionItem )
{
}

SCollectionItem * CCollectionModel::collectionItem( const QModelIndex & idx ) const
{
    auto item = reinterpret_cast<SCollectionItem *>( idx.internalPointer() );
    return item;
}

QModelIndex CCollectionModel::index( int row, int column, const QModelIndex & parentIdx /*= QModelIndex() */ ) const
{
    QModelIndex retVal;
    if ( parentIdx.isValid() )
    {
        if ( isCollection( parentIdx ) )
        {
            auto parent = collectionItem( parentIdx );
            if ( !parent )
                return {};

            auto childObj = parent->child( row ).get();
            if ( !childObj )
                int xyz = 0;
            retVal = createIndex( row, column, childObj );
        }
    }
    else
    {
        auto collection = fRootItem->collection();
        auto childObj = collection->child( row ).get();
        if ( !childObj )
            int xyz = 0;
        retVal = createIndex( row, column, childObj );
    }
    return retVal;
}

QModelIndex CCollectionModel::createIndex( int row, int column, SCollectionItem * data ) const
{
    return QAbstractItemModel::createIndex( row, column, data );
}

void CCollectionModel::setSyncSystem( std::shared_ptr<CSyncSystem> syncSystem )
{
    fSyncSystem = syncSystem;
    connect( fSyncSystem.get(), &CSyncSystem::sigUpdateCollection, this, &CCollectionModel::slotUpdateCollection );
}

QModelIndex CCollectionModel::parent( const QModelIndex & child ) const
{
    if ( !child.isValid() )
        return {};
    
    auto item = this->collectionItem( child );
    Q_ASSERT( item );
    if ( !item  )
        return {};

    auto parentItem = item->parent();
    if ( !parentItem )
        parentItem = fRootItem->collection().get();

    Q_ASSERT( parentItem );
    if ( !parentItem || parentItem->isRoot() )
        return {};
    auto parentRow = parentItem ? parentItem->collectionNum() : 0;
    auto retVal = createIndex( parentRow, 0, parentItem->container() );
    return retVal;
}

int CCollectionModel::rowCount( const QModelIndex & parent /*= QModelIndex() */ ) const
{
    if ( parent.isValid() )
    {
        if ( parent.column() != 0 )
            return 0;

        if ( this->isMedia( parent ) )
            return 0;

        auto parentItem = collectionItem( parent );
        if ( !parentItem )
            return 0;

        auto retVal = parentItem->childCount();
        return retVal;
    }
    else
    {
        if ( !fRootItem )
            return 0;
        return fRootItem->childCount();
    }
}

bool CCollectionModel::isMedia( const QModelIndex & idx ) const
{
    auto ptr = this->collectionItem( idx );
    return ptr && ptr->isMedia();
}

bool CCollectionModel::isCollection( const QModelIndex & idx ) const
{
    auto ptr = this->collectionItem( idx );
    return ptr && ptr->isCollection();
}

QString CCollectionModel::summary() const
{
    int numMovies = fRootItem->numMovies();
    int numMissing = fRootItem->numMissing();
    int numCollections = fRootItem->numCollections();
    int numCollectionsMissing = fRootItem->numCollectionsMissing();

    return tr( "Searching for: %1 Movies (%2 missing) - %3 Collections (%4 missing)" ).arg( numMovies ).arg( numMissing ).arg( numCollections ).arg( numCollectionsMissing );
}

void CCollectionModel::clear()
{
    beginResetModel();
    fRootItem = std::make_shared< SCollectionItem >();
    endResetModel();
}

void CCollectionModel::updateCollection( std::shared_ptr< CMediaCollection > collection )
{
}

void CCollectionModel::updateCollections( const QString & serverName, std::shared_ptr< CMediaModel > model )
{
    auto update = fRootItem->updateFromServer( serverName, model );
    if ( update )
    {
        beginResetModel();
        endResetModel();
    }
}

void CCollectionModel::createCollections()
{
    fCollectionCreationList = fRootItem->getCollectionsNeedingCreation();
    QTimer::singleShot( 0,
                        [this]()
                        {
                            if ( fCollectionCreationList.empty() )
                                return;

                            auto curr = fCollectionCreationList.front();
                            fCollectionCreationList.pop_front();
                            fSyncSystem->createCollection( fServerInfo, curr );
                        } );
}


Q_DECLARE_METATYPE( std::shared_ptr< CMediaData > );
Q_DECLARE_SMART_POINTER_METATYPE( std::shared_ptr );

void CCollectionModel::slotUpdateCollection( const QString & serverName, const QString & name, const QString & id, const QVariantList & variantItems, const QString & parentID )
{
    std::list< std::shared_ptr< CMediaData > > items;
    for ( auto && ii : variantItems )
    {
        auto curr = ii.value< std::shared_ptr< CMediaData > >();
        items.push_back( curr );
    }

    auto updatedCollection = fMediaModel->updateCollection( serverName, name, id, items, parentID );
    if ( updatedCollection )
    {
        updateCollection( updatedCollection );
    }
}

void CCollectionModel::slotMediaModelDataChanged()
{
    auto changed = fRootItem->updateMedia( fMediaModel );
    if ( changed )
    {
        beginResetModel();
        endResetModel();
    }
}

int CCollectionModel::columnCount( const QModelIndex & parent /*= QModelIndex()*/  ) const
{
    if ( !parent.isValid() || ( parent.column() == 0 ) )
        return 5;
    return 0;
}

QVariant CCollectionModel::data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const
{
    if ( !index.isValid() )
        return {};

    if ( role == Qt::DisplayRole )
    {
        auto item = collectionItem( index );
        if ( !item || item->isRoot() )
            return {};

        return item->data( fServerInfo, index.column(), role );
    }
    return {};
}

bool CCollectionModel::hasChildren( const QModelIndex & parent ) const
{
    return QAbstractItemModel::hasChildren( parent );
}


QVariant CCollectionModel::headerData( int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */ ) const
{
    if ( ( orientation != Qt::Orientation::Horizontal ) || ( role != Qt::DisplayRole ) )
        return QAbstractItemModel::headerData( section, orientation, role );
    switch ( section )
    {
        case 0: return tr( "Name" );
        case 1: return tr( "Year" );
        case 2: return tr( "Collection Exists" );
        case 3: return tr( "Missing Media" );
        case 4: return tr( "ID on Server" );
    }
    return {};
}

std::pair< QModelIndex, std::shared_ptr< SCollectionItem > > CCollectionModel::addCollection( const QString & server, const QString & name, const QModelIndex & parentIndex )
{
    auto parent = collectionItem( parentIndex );
    Q_ASSERT( !parentIndex.isValid() || ( parentIndex.isValid() && parent ) );
    if ( parentIndex.isValid() && !parent )
        return {};

    if ( !parentIndex.isValid() && !parent )
        parent = fRootItem.get();

    if ( !parent )
        return {};

    auto numChildren = parent->childCount();
    beginInsertRows( parentIndex, numChildren, numChildren  );
    auto curr = std::make_shared< CMediaCollection >( server, name, QString(), numChildren, parent );
    auto retVal = parent->addChild( curr );
    endInsertRows();

    auto idx = index( numChildren, 0, parentIndex );
    Q_ASSERT( retVal.get() == this->collectionItem( idx ) );
    Q_ASSERT( curr.get() == this->collectionItem( idx )->collection().get() );
    return { idx, retVal };
}

std::shared_ptr< SCollectionItem > CCollectionModel::addMovie( int rank, const QString & name, int year, const QModelIndex & parentIdx )
{
    Q_ASSERT( parentIdx.isValid() );
    if ( !parentIdx.isValid() )
        return {};

    auto parentItem = collectionItem( parentIdx );
    Q_ASSERT( parentItem );
    if ( !parentItem )
        return {};

    Q_ASSERT( parentItem->isCollection() );
    if ( !parentItem->isCollection() )
        return {};

    bool sizeIncreased = ( rank >= parentItem->childCount() );
    if ( sizeIncreased )
        beginInsertRows( parentIdx, parentItem->childCount(), rank - 1 );
    auto retVal = parentItem->collection()->addMovie( rank, name, year );
    if ( sizeIncreased )
        endInsertRows();
    else
        emit dataChanged( index( rank - 1, 0, parentIdx ), index( rank - 1, columnCount( parentIdx ) - 1, parentIdx ) );
    Q_ASSERT( hasChildren( parentIdx ) );
    
    auto rc = rowCount( parentIdx );
    if ( sizeIncreased )
    {
        Q_ASSERT( rc == ( rank ) );
    }
    else
        Q_ASSERT( ( rank + 1 ) <= rc );
    return retVal;
}

bool CCollectionModel::loadFromFile( const QString & fileName, const QString & serverName, QWidget * parentWidget )
{
    QFile fi( fileName );
    if ( !fi.open( QFile::ReadOnly ) )
    {
        return false;
    }

    auto data = fi.readAll();
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical( parentWidget, tr( "Error Reading File" ), tr( "Error: %1 @ %2" ).arg( error.errorString() ).arg( error.offset ) );
        return false;
    }
    if ( !doc.isArray() )
    {
        QMessageBox::critical( parentWidget, tr( "Error Reading File" ), tr( "Error: Should be array of objects" ) );
        return false;
    }

    clear();
    auto collections = doc.array();
    for ( auto && curr : collections )
    {
        loadCollection( serverName, curr.toObject(), {} );
    }
    return true;
}


int depth( const QModelIndex & parentIndex )
{
    if ( parentIndex.isValid() )
        return depth( parentIndex.parent() ) + 1;
    return 0;
}

void CCollectionModel::loadCollection( const QString & serverName, const QJsonObject & curr, const QModelIndex & parentIndex )
{
    auto collectionName = curr[ "collection" ].toString();
    qDebug().noquote().nospace() << QString( 4 * depth( parentIndex ), ' ' ) << collectionName;
    auto idx = addCollection( serverName, collectionName, parentIndex ).first;

    auto collections = curr[ "collections" ].toArray();
    for ( auto && collection : collections )
    {
        loadCollection( serverName, collection.toObject(), idx );
    }

    auto movies = curr[ "movies" ].toArray();
    for ( auto && movie : movies )
    {
        loadMovie( movie.toObject(), idx );
    }
}

void CCollectionModel::loadMovie( const QJsonObject & movie, const QModelIndex & parentIndex )
{
    auto rank = movie[ "rank" ].toInt();
    auto movieName = movie[ "name" ].toString();
    auto year = movie[ "year" ].toInt();
    addMovie( rank, movieName, year, parentIndex );
}