#include "ServerModel.h"

#include "Settings.h"
#include "ServerInfo.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

CServerModel::CServerModel( QObject *parent ) :
    QAbstractTableModel( parent )
{
}

void CServerModel::setSettings( CSettings *settings )
{
    fSettings = settings;
}

int CServerModel::rowCount( const QModelIndex &parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;

    return static_cast< int >( fServers.size() );
}

int CServerModel::columnCount( const QModelIndex &parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;
    return eColumnCount;
}

QVariant CServerModel::data( const QModelIndex &index, int role /*= Qt::DisplayRole */ ) const
{
    if ( !index.isValid() || index.parent().isValid() || ( index.row() >= rowCount() ) )
        return {};

    auto serverInfo = getServerInfo( index.row() );
    if ( role == Qt::DecorationRole )
    {
        if ( index.column() == EColumns::eFriendlyName )
            return serverInfo->icon();
        return {};
    }
    else if ( role == ECustomRoles::eIsPrimaryServerSet )
    {
        return !fSettings->primaryServer().isEmpty();
    }
    else if ( role == ECustomRoles::eIsPrimaryServer )
    {
        return fSettings->primaryServer() == serverInfo->displayName();
    }
    else if ( role == ECustomRoles::eEnabledRole )
    {
        return serverInfo->isEnabled();
    }

    if ( role == Qt::DisplayRole )
    {
        switch ( index.column() )
        {
            case EColumns::eFriendlyName:
                return serverInfo->displayName();
            case EColumns::eUrl:
                return serverInfo->url( true );
            case EColumns::eEnabled:
                return serverInfo->isEnabled() ? "Yes" : "No";
            case EColumns::eAPI:
                return serverInfo->apiKey();
        }
    }
    return {};
}

QVariant CServerModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    if ( section < 0 || section > columnCount() )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( role != Qt::DisplayRole )
        return QAbstractTableModel::headerData( section, orientation, role );

    switch ( section )
    {
        case EColumns::eFriendlyName:
            return tr( "Name" );
        case EColumns::eUrl:
            return tr( "Url" );
        case EColumns::eEnabled:
            return tr( "Enabled?" );
        case EColumns::eAPI:
            return tr( "API" );
    }
    return {};
}

void CServerModel::clear()
{
    beginResetModel();
    fServers.clear();
    fServerMap.clear();
    endResetModel();
}

void CServerModel::settingsChanged()
{
    beginResetModel();
    endResetModel();
}

std::shared_ptr< CServerInfo > CServerModel::getServerInfo( const QModelIndex &idx ) const
{
    if ( !idx.isValid() )
        return {};
    if ( idx.model() != this )
        return {};

    return fServers[ idx.row() ];
}

void CServerModel::save( QJsonObject &root )
{
    auto servers = QJsonDocument().array();
    for ( int ii = 0; ii < serverCnt(); ++ii )
    {
        servers.push_back( getServerInfo( ii )->toJson() );
    }

    root[ "servers" ] = servers;
}

CServerFilterModel::CServerFilterModel( QObject *parent ) :
    QSortFilterProxyModel( parent )
{
    setDynamicSortFilter( false );
}

bool CServerFilterModel::filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const
{
    if ( !sourceModel() )
        return true;
    if ( !fOnlyShowEnabled && !fOnlyShowPrimaryServer )
        return true;
    auto childIdx = sourceModel()->index( source_row, 0, source_parent );
    if ( fOnlyShowPrimaryServer && childIdx.data( CServerModel::eIsPrimaryServerSet ).toBool() )
        return childIdx.data( CServerModel::eIsPrimaryServer ).toBool();
    return childIdx.data( CServerModel::eEnabledRole ).toBool();
}

void CServerFilterModel::setOnlyShowEnabledServers( bool value )
{
    fOnlyShowEnabled = value;
    invalidateFilter();
}

void CServerFilterModel::setOnlyShowPrimaryServer( bool value )
{
    fOnlyShowPrimaryServer = value;
    invalidateFilter();
}

void CServerFilterModel::sort( int column, Qt::SortOrder order /*= Qt::AscendingOrder */ )
{
    QSortFilterProxyModel::sort( column, order );
}

bool CServerFilterModel::lessThan( const QModelIndex &source_left, const QModelIndex &source_right ) const
{
    return QSortFilterProxyModel::lessThan( source_left, source_right );
}

bool CServerModel::canAllServersSync() const
{
    for ( auto &&ii : fServers )
    {
        if ( !ii->canSync() )
            return false;
    }
    return true;
}

bool CServerModel::canAnyServerSync() const
{
    for ( auto &&ii : fServers )
    {
        if ( ii->canSync() )
            return true;
    }
    return false;
}

void CServerModel::setServers( const std::vector< std::shared_ptr< CServerInfo > > &servers )
{
    bool changed = serversChanged( servers, fServers );
    changed = changed || serversChanged( fServers, servers );

    if ( !changed )
        return;

    fChanged = true;
    fServers = servers;
    fServerMap.clear();
    updateServerMap();
    updateFriendlyServerNames();
    emit sigServersLoaded();
}

int CServerModel::enabledServerCnt() const
{
    int retVal = 0;
    for ( int ii = 0; ii < serverCnt(); ++ii )
    {
        if ( fServers[ ii ]->isEnabled() )
            retVal++;
    }
    return retVal;
}

bool CServerModel::serversChanged( const TServerVector &lhs, const TServerVector &rhs ) const
{
    if ( lhs.size() != rhs.size() )
        return true;

    bool retVal = false;
    for ( auto &&ii : lhs )
    {
        std::shared_ptr< CServerInfo > found;
        for ( auto &&jj : rhs )
        {
            if ( ii->keyName() == jj->keyName() )
            {
                found = jj;
                break;
            }
        }
        if ( found )
        {
            if ( *ii != *found )
            {
                retVal = true;
                break;
            }
        }
        else
        {
            retVal = true;
            break;
        }
    }
    return retVal;
}

void CServerModel::updateServerMap()
{
    for ( size_t ii = 0; ii < fServers.size(); ++ii )
    {
        auto server = fServers[ ii ];
        fServerMap[ server->keyName() ] = { server, ii };
    }
}

bool CServerModel::loadServer( const QJsonObject &obj, QString &errorMsg, bool lastServer )
{
    auto serverInfo = CServerInfo::fromJson( obj, errorMsg );
    if ( !serverInfo )
        return false;

    if ( this->findServerInfo( serverInfo->keyName() ) )
    {
        errorMsg = QString( "Server %1(%2)' already exists" ).arg( serverInfo->displayName() ).arg( serverInfo->keyName() );
        return false;
    }

    beginInsertRows( QModelIndex(), static_cast< int >( fServers.size() ), static_cast< int >( fServers.size() ) );
    fServers.push_back( serverInfo );
    fServerMap[ serverInfo->keyName() ] = { serverInfo, fServers.size() - 1 };
    endInsertRows();

    updateFriendlyServerNames();
    if ( lastServer )
        emit sigServersLoaded();
    return true;
}

int CServerModel::getServerPos( const QString &serverName ) const
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return static_cast< int >( ( *pos ).second.second );
    return -1;
}

std::shared_ptr< CServerInfo > CServerModel::getServerInfo( int serverNum ) const
{
    if ( serverNum < 0 )
        return {};
    if ( serverNum >= serverCnt() )
        return {};

    return fServers[ serverNum ];
}

std::shared_ptr< const CServerInfo > CServerModel::findServerInfo( const QString &serverName ) const
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return ( *pos ).second.first;
    return {};
}

std::shared_ptr< const CServerInfo > CServerModel::findServerInfo( const QString &serverName )
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return ( *pos ).second.first;
    return {};
}

std::shared_ptr< CServerInfo > CServerModel::enableServer( const QString &serverName, bool disableOthers, QString &msg )
{
    std::shared_ptr< CServerInfo > retVal;
    for ( auto &&ii : fServers )
    {
        auto isServer = ii->isServer( serverName );
        if ( isServer )
        {
            ii->setIsEnabled( true );
        }
        else if ( disableOthers )
            ii->setIsEnabled( false );
        if ( isServer && retVal )
        {
            msg = QString( "Multiple servers match '%1'." ).arg( serverName );
            return false;
        }
        if ( isServer )
            retVal = ii;
    }
    return retVal;
}

void CServerModel::updateServerInfo( const QString &serverName, const QJsonObject &serverData )
{
    auto serverInfo = findServerInfoInternal( serverName );
    if ( !serverInfo.first )
        return;

    serverInfo.first->update( serverData );
    emit dataChanged( index( serverInfo.second, 0 ), index( serverInfo.second, EColumns::eColumnCount - 1 ) );
}

void CServerModel::setServerIcon( const QString &serverName, const QByteArray &data, const QString &type )
{
    auto serverInfo = findServerInfoInternal( serverName );
    if ( !serverInfo.first )
        return;

    serverInfo.first->setIcon( data, type );
    emit dataChanged( index( serverInfo.second, 0 ), index( serverInfo.second, 0 ) );
}

std::pair< std::shared_ptr< CServerInfo >, int > CServerModel::findServerInfoInternal( const QString &serverName )
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return { ( *pos ).second.first, static_cast< int >( ( *pos ).second.second ) };
    return {};
}

int CServerModel::serverCnt() const
{
    return static_cast< int >( fServers.size() );
}

void CServerModel::setAPIKey( const QString &serverName, const QString &apiKey )
{
    auto serverInfo = this->findServerInfoInternal( serverName );
    fChanged = serverInfo.first->setAPIKey( apiKey ) || fChanged;
    emit dataChanged( index( serverInfo.second, EColumns::eAPI ), index( serverInfo.second, EColumns::eAPI ) );
}

void CServerModel::changeServerDisplayName( const QString &newServerName, const QString &oldServerName )
{
    auto serverInfo = this->findServerInfoInternal( oldServerName );
    if ( serverInfo.first )
    {
        if ( serverInfo.first->setDisplayName( newServerName, false ) )
            fChanged = true;

        auto pos = fServerMap.find( oldServerName );
        if ( pos != fServerMap.end() )
            fServerMap.erase( pos );
        emit dataChanged( index( serverInfo.second, EColumns::eFriendlyName ), index( serverInfo.second, EColumns::eFriendlyName ) );
        updateServerMap();
    }
}

void CServerModel::setURL( const QString &serverName, const QString &url )
{
    auto serverInfo = this->findServerInfoInternal( serverName );
    if ( serverInfo.first->setUrl( url ) )
    {
        fChanged = true;
        emit dataChanged( index( serverInfo.second, EColumns::eUrl ), index( serverInfo.second, EColumns::eUrl ) );
    }
}

void CServerModel::updateFriendlyServerNames()
{
    std::multimap< QString, std::shared_ptr< CServerInfo > > servers;
    for ( int ii = 0; ii < serverCnt(); ++ii )
    {
        auto serverInfo = fServers[ ii ];
        if ( serverInfo->displayNameGenerated() )   // generatedName
            servers.insert( { serverInfo->displayName(), serverInfo } );
    }

    for ( auto &&ii : servers )
    {
        auto cnt = servers.count( ii.first );
        ii.second->autoSetDisplayName( cnt > 1 );
    }
    updateServerMap();
}
