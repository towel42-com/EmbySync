#ifndef __USERSMODEL_H
#define __USERSMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <memory>
#include "SABUtils/HashUtils.h"

#include <unordered_set>
#include <vector>

class CSettings;
class CUserData;
class CServerInfo;
class CSyncSystem;
class CUsersModel : public QAbstractTableModel
{
    Q_OBJECT;
public:
    enum EColumns
    {
        eConnectedID,
        eAllNames,
        eFirstServerColumn
    };

    enum EServerColumns
    {
        eUserName,
        eServerConnectedID,
        eIconStatus,
        eServerColCount
    };

    enum ECustomRoles
    {
        eShowItemRole=Qt::UserRole+1,
        eConnectedIDRole,
        eConnectedIDValidRole,
        eIsUserNameColumnRole,
        eSyncDirectionIconRole,
        eServerNameForColumnRole = Qt::UserRole + 10000
    };

    CUsersModel( std::shared_ptr< CSettings > settings, QObject * parent=nullptr );

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    std::shared_ptr< const CServerInfo > serverInfo( const QModelIndex & index ) const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    struct SUsersSummary
    {
        int fTotal{ 0 };
        int fMissing{ 0 };
        int fSyncable{ 0 };
    };

    SUsersSummary settingsChanged();
    SUsersSummary getMediaSummary() const;

    std::shared_ptr< CUserData > userDataForName( const QString & name ) const;
    std::shared_ptr< CUserData > userData( const QModelIndex & idx ) const;
    std::shared_ptr< CUserData > findUser( const QString & serverName, const QString & userID ) const;
    QModelIndex indexForUser( std::shared_ptr< CUserData > user, int column = 0 ) const;

    void setUserImage( const QString & serverName, const QString & userID, const QByteArray & data );

    void updateUserConnectID( const QString & serverName, const QString & userID, const QString & connectID );

    void clear();

    std::shared_ptr< CUserData > loadUser( const QString & serverName, const QJsonObject & user );
    std::vector< std::shared_ptr< CUserData > > getAllUsers( bool sorted ) const;

    bool hasUsersWithConnectedIDNeedingUpdate() const;
    std::list< std::shared_ptr< CUserData > > usersWithConnectedIDNeedingUpdate() const;

    void loadAvatars( std::shared_ptr< CSyncSystem > syncSystem ) const;
public Q_SLOTS:
    void slotSettingsChanged();
    void slotServerInfoChanged();

private:
    int columnsPerServer() const;
    int perServerColumn( int column ) const;

    void setupColumns();
    int serverNum( int columnNum ) const;
    std::shared_ptr< CUserData > getUserData( const QString & name, bool exhaustiveSearch=false ) const;

    QVariant getColor( const QModelIndex & index, bool background ) const;
        
    std::map< QString, std::shared_ptr< CUserData > > fUserMap;
    std::vector< std::shared_ptr< CUserData > > fUsers;
    std::shared_ptr< CSettings > fSettings;

    std::unordered_map< int, std::pair< int, std::shared_ptr< const CServerInfo > > > fColumnToServerInfo;
    std::map< int, int > fServerNumToColumn;
};

class CUsersFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;
public:
    CUsersFilterModel( bool forUserSelection, QObject * parent );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;
    virtual bool filterAcceptsColumn( int source_row, const QModelIndex & source_parent ) const override;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
private:
    bool fForUserSelection{ false };
};

#endif
