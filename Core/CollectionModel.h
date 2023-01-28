#ifndef __COLLECTIONMODEL_H
#define __COLLECTIONMODEL_H

#include <QAbstractItemModel>
class CMediaCollection;
struct SCollectionItem;
class CSyncSystem;
class CServerInfo;
class CMediaModel;
class QJsonObject;

class CCollectionModel : public QAbstractItemModel
{
    Q_OBJECT;
public:
    CCollectionModel( std::shared_ptr< CMediaModel > mediaModel );

    virtual  QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const override;
    virtual QModelIndex parent( const QModelIndex & child ) const override;
    virtual bool hasChildren( const QModelIndex & parent ) const override;
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role /* = Qt::DisplayRole */ ) const override;

    void clear();
    QString summary() const;

    std::pair< QModelIndex, std::shared_ptr< SCollectionItem > > addCollection( const QString & server, const QString & name, const QModelIndex & parentIndex );
    void updateCollections( const QString & serverName, std::shared_ptr< CMediaModel > model );
    void updateCollection( std::shared_ptr< CMediaCollection > collection );

    std::shared_ptr< SCollectionItem > addMovie( int rank, const QString & name, int year, const QModelIndex & collectionIndex );
    void createCollections();


    bool loadFromFile( const QString & fileName, const QString & serverName, QWidget * parentWidget );
    QModelIndex createIndex( int row, int column, SCollectionItem * data ) const;

    void setServerInfo( std::shared_ptr<const CServerInfo> serverInfo ) { fServerInfo = serverInfo; }
    void setSyncSystem( std::shared_ptr<CSyncSystem> syncSystem );
public Q_SLOTS:
    void slotMediaModelDataChanged();
    void slotUpdateCollection( const QString & serverName, const QString & name, const QString & id, const QVariantList & items, const QString & parentID );
private:
    void loadCollection( const QString & serverName, const QJsonObject & curr, const QModelIndex & parentIdx );
    void loadMovie( const QJsonObject & movie, const QModelIndex & parentIdx );

    SCollectionItem * collectionItem( const QModelIndex & idx ) const;

    bool isMedia( const QModelIndex & parent ) const;
    bool isCollection( const QModelIndex & parent ) const;

    std::shared_ptr< CMediaModel > fMediaModel;
    std::shared_ptr< SCollectionItem > fRootItem;
    std::shared_ptr<const CServerInfo> fServerInfo;
    std::shared_ptr<CSyncSystem> fSyncSystem;

    std::list< std::shared_ptr< CMediaCollection > > fCollectionCreationList;
};

#endif
