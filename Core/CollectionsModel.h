#ifndef __COLLECTIONSMODEL_H
#define __COLLECTIONSMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <memory>

class CMediaCollection;
struct SMediaCollectionData;
class CSyncSystem;
class CServerInfo;
class CMediaData;
class CMediaModel;

struct SIndexPtr
{
    SIndexPtr( void * ptr, bool isCollection );
    void * fPtr{ nullptr };
    bool fIsCollection{ false };
};

class CCollectionsModel : public QAbstractItemModel
{
    Q_OBJECT;
public:
    CCollectionsModel( std::shared_ptr< CMediaModel > mediaModel );

    virtual  QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const override;
    virtual QModelIndex parent( const QModelIndex & child ) const override;
    virtual bool hasChildren( const QModelIndex & parent ) const override;

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */ ) const override;

    std::pair< QModelIndex, std::shared_ptr< CMediaCollection > > addCollection( const QString & server, const QString & name );
    std::pair< QModelIndex, std::shared_ptr< CMediaCollection > > addCollection( const QString& serverName, const QString& name, const QString& id, const std::list< std::shared_ptr< CMediaData > >& items);
    std::shared_ptr< SMediaCollectionData > addMovie( const QString & name, int year, const QModelIndex & collectionIndex, int rank );

    CMediaCollection * collection( const QModelIndex & idx ) const;
    SMediaCollectionData * media( const QModelIndex & idx ) const;

    bool isMedia( const QModelIndex & parent ) const;
    bool isCollection( const QModelIndex & parent ) const;

    QString summary() const;
    void clear();

    void updateCollections( const QString & serverName, std::shared_ptr< CMediaModel > model );
    void createCollections( std::shared_ptr<const CServerInfo> serverInfo, std::shared_ptr< CSyncSystem > syncSystem, QWidget * parent );
public Q_SLOTS:
    void slotMediaModelDataChanged();
private:
    SIndexPtr * idxPtr( CMediaCollection * mediaCollection ) const;
    SIndexPtr * idxPtr( SMediaCollectionData * media ) const;
    SIndexPtr * idxPtr( void * media, bool isCollection ) const;

    std::vector< std::shared_ptr< CMediaCollection > > fCollections;
    std::map< QString, std::vector< std::shared_ptr< CMediaCollection > > > fCollectionsMap;
    mutable std::map< void *, SIndexPtr * > fIndexPtrs;

    std::shared_ptr< CMediaModel > fMediaModel;
};

class CCollectionsFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;
public:
    CCollectionsFilterModel(QObject* parent);

    virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
    virtual bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;

};
#endif
