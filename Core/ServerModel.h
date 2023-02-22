#ifndef __SERVERMODEL_H
#define __SERVERMODEL_H

#include "SABUtils/HashUtils.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

#include <memory>

class CSettings;
class CServerInfo;

class CServerModel : public QAbstractTableModel
{
    Q_OBJECT;

public:
    enum EColumns
    {
        eFriendlyName,
        eUrl,
        eEnabled,
        eAPI,
        eColumnCount
    };

    enum ECustomRoles
    {
        eEnabledRole = Qt::UserRole + 1,
        eIsPrimaryServer,
        eIsPrimaryServerSet
    };

    CServerModel( QObject *parent = nullptr );
    void setSettings( CSettings *settings );

    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    void clear();
    void settingsChanged();

    bool loadServer( const QJsonObject &obj, QString &errorMsg, bool lastServer );
    void save( QJsonObject &json );

    std::shared_ptr< CServerInfo > getServerInfo( const QModelIndex &idx ) const;
    std::shared_ptr< CServerInfo > getServerInfo( int serverNum ) const;

    void setServers( const std::vector< std::shared_ptr< CServerInfo > > &servers );

    int enabledServerCnt() const;
    int serverCnt() const;

    bool canAllServersSync() const;
    bool canAnyServerSync() const;

    int getServerPos( const QString &serverName ) const;
    std::shared_ptr< const CServerInfo > findServerInfo( const QString &serverName ) const;
    std::shared_ptr< const CServerInfo > findServerInfo( const QString &serverName );

    void updateServerInfo( const QString &serverName, const QJsonObject &serverData );
    void setServerIcon( const QString &serverName, const QByteArray &data, const QString &type );

    std::shared_ptr< CServerInfo > enableServer( const QString &serverName, bool disableOthers, QString &errorMsg );   // returns the enabled server

    using TServerVector = std::vector< std::shared_ptr< CServerInfo > >;
    using iterator = typename TServerVector::iterator;
    using const_iterator = typename TServerVector::const_iterator;

    iterator begin() { return fServers.begin(); }
    iterator end() { return fServers.end(); }
    const_iterator begin() const { return fServers.cbegin(); }
    const_iterator end() const { return fServers.cend(); }
Q_SIGNALS:
    void sigServersLoaded();

private:
    bool serversChanged( const TServerVector &lhs, const TServerVector &rhs ) const;
    void updateServerMap();
    void updateFriendlyServerNames();
    void setURL( const QString &serverName, const QString &url );
    void setAPIKey( const QString &serverName, const QString &apiKey );
    std::pair< std::shared_ptr< CServerInfo >, int > findServerInfoInternal( const QString &serverName );
    void changeServerDisplayName( const QString &serverName, const QString &oldServerName );

    TServerVector fServers;
    std::map< QString, std::pair< std::shared_ptr< CServerInfo >, size_t > > fServerMap;
    CSettings *fSettings{ nullptr };
    bool fChanged{ false };
};

class CServerFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT;

public:
    CServerFilterModel( QObject *parent );

    void setOnlyShowEnabledServers( bool showOnlyEnabled );
    void setOnlyShowPrimaryServer( bool showOnlyPrimary );

    virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override;
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    virtual bool lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const override;

private:
    bool fOnlyShowEnabled{ false };
    bool fOnlyShowPrimaryServer{ false };
};
#endif
