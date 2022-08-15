#ifndef __USERSMODEL_H
#define __USERSMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <memory>

class CSettings;
class CUserData;

class CUsersModel : public QAbstractTableModel
{
    Q_OBJECT;
public:
    enum EColumns
    {
        eLHSName,
        eRHSName,
        eConnectedID,
        eOnLHSServer,
        eOnRHSServer
    };

    enum ECustomRoles
    {
        eShowItemRole=Qt::UserRole+1,
        eLHSNameRole,
        eRHSNameRole,
        eConnectedIDRole
    };

    CUsersModel( std::shared_ptr< CSettings > settings, QObject * parent );

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    struct SUsersSummary
    {
        int fTotal{ 0 };
        int fMissing{ 0 };
        int fSyncable{ 0 };
    };

    SUsersSummary settingsChanged();
    SUsersSummary getMediaSummary() const;

    std::shared_ptr< CUserData > userDataForName( const QString & name );
    void clear();

    std::shared_ptr< CUserData > loadUser( const QJsonObject & user, bool isLHSServer );
    std::vector< std::shared_ptr< CUserData > > getAllUsers( bool sorted ) const;
public Q_SLOTS:
    void slotSettingsChanged();
private:
    std::shared_ptr< CUserData > getUserData( const QString & name ) const;
    
    QVariant getColor( const QModelIndex & index, bool background ) const;
        
    std::map< QString, std::shared_ptr< CUserData > > fUserMap;
    std::vector< std::shared_ptr< CUserData > > fUsers;
    std::shared_ptr< CSettings > fSettings;
};

class CUsersFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;
public:
    CUsersFilterModel( QObject * parent );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;

};
#endif
