#include "UsersModel.h"
#include "UserData.h"
#include "Settings.h"
#include "ServerInfo.h"
#include "SyncSystem.h"

#include <QColor>
#include <set>
#include <QJsonObject>
#include <QJsonDocument>
#include <QImage>

CUsersModel::CUsersModel( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QAbstractTableModel( parent ),
    fSettings( settings )
{
    setupColumns();
}

void CUsersModel::setupColumns()
{
    // caller responsible for reset model
    fColumnToServerInfo.clear();
    fServerNumToColumn.clear();
    int columnNum = static_cast<int>( eFirstServerColumn );
    auto colsPerServer = this->columnsPerServer();
    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = fSettings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;
        for ( int jj = 0; jj < colsPerServer; ++jj )
        {
            fServerNumToColumn[ ii ] = columnNum;
            fColumnToServerInfo[ columnNum ] = std::make_pair( ii, serverInfo );

            columnNum++;
        }

        disconnect( serverInfo.get(), &CServerInfo::sigServerInfoChanged, this, &CUsersModel::slotServerInfoChanged );
        connect( serverInfo.get(), &CServerInfo::sigServerInfoChanged, this, &CUsersModel::slotServerInfoChanged );
    }
    Q_ASSERT( columnNum == columnCount() );
}

void CUsersModel::slotServerInfoChanged()
{
    int colMin = 32767;
    int colMax = -1;
    for ( auto && ii : fServerNumToColumn )
    {
        colMin = std::min( ii.second, colMin );
        colMax = std::max( ii.second, colMax );
    }
    emit headerDataChanged( Qt::Orientation::Horizontal, colMin, colMax );
}

int CUsersModel::rowCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;

    return static_cast<int>( fUsers.size() );
}

int CUsersModel::columnsPerServer() const
{
    return static_cast<int>( EServerColumns::eServerColCount );
}

int CUsersModel::columnCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;
    auto retVal = static_cast<int>( eFirstServerColumn );
    retVal += fSettings->enabledServerCnt() * columnsPerServer();
    return retVal;
}

QVariant CUsersModel::data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const
{
    if ( !index.isValid() || index.parent().isValid() || ( index.row() >= rowCount() ) )
        return {};

    auto userData = fUsers[ index.row() ];

    if ( role == ECustomRoles::eShowItemRole )
    {
        if ( !fSettings->onlyShowSyncableUsers() )
            return true;
        return userData->canBeSynced();
    }

    if ( role == ECustomRoles::eConnectedIDRole )
        return userData->connectedID();

    if ( role == ECustomRoles::eConnectedIDValidRole )
        return !userData->connectedID().isEmpty() && !userData->connectedIDNeedsUpdate();

    //// reverse for black background
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

    auto serverInfo = this->serverInfo( index );
    auto serverName = serverInfo ? serverInfo->keyName() : QString();

    if ( role == ECustomRoles::eServerNameForColumnRole )
    {
        if ( index.column() == CUsersModel::eConnectedID )
            return "<ALL>";
#ifndef NDEBUG
        if ( index.column() == CUsersModel::eAllNames )
            return "<ALL>";
#endif
        //qDebug() << "Server for column: " << index.column() << serverName;
        return serverName;
    }

    auto columnNum = perServerColumn( index.column() );

    if ( role == ECustomRoles::eIsUserNameColumnRole )
    {
        if ( index.column() < EColumns::eFirstServerColumn )
            return false;
        return columnNum == EServerColumns::eUserName;
    }

    if ( role == eSyncDirectionIconRole )
    {
        return userData->getDirectionIcon( serverName );
    }

    if ( role == Qt::DecorationRole )
    {
        if ( index.column() == CUsersModel::eConnectedID )
            return userData->globalAvatar();
        
        if ( columnNum == EServerColumns::eUserName )
        {
            if ( userData->globalAvatar().isNull() )
                return userData->getAvatar( serverName );
        }
        else if ( columnNum == EServerColumns::eIconStatus )
        {
            return userData->getAvatar( serverName, true );
        }
    }

    if ( role != Qt::DisplayRole )
        return {};

    if ( index.column() == eAllNames )
        return userData->allNames();
    else if ( index.column() == eConnectedID )
        return userData->connectedID();

    switch ( columnNum )
    {
        case EServerColumns::eUserName:
            return userData->name( serverName );
        case EServerColumns::eServerConnectedID:
            return userData->connectedID( serverName );
    }
    return {};
}

int CUsersModel::serverNum( int columnNum ) const
{
    auto pos = fColumnToServerInfo.find( columnNum );
    if ( pos == fColumnToServerInfo.end() )
        return -1;
    return ( *pos ).second.first;
}

std::shared_ptr< const CServerInfo > CUsersModel::serverInfo( const QModelIndex & index ) const
{
    auto pos = fColumnToServerInfo.find( index.column() );
    if ( pos == fColumnToServerInfo.end() )
        return {};
    return ( *pos ).second.second;
}

int CUsersModel::perServerColumn( int column ) const
{
    if ( column >= EColumns::eFirstServerColumn )
        column -= EColumns::eFirstServerColumn;
    return column % columnsPerServer();
}

QVariant CUsersModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    if ( section < 0 || section > columnCount() )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );

    if ( role == Qt::DisplayRole )
    {
        if ( section == eConnectedID )
            return tr( "Connected ID" );
        else if ( section == eAllNames )
            return tr( "All Names" );
    }

    auto columnNum = perServerColumn( section );

    auto pos = fColumnToServerInfo.find( section );
    if ( pos == fColumnToServerInfo.end() )
        return {};

    if ( role == Qt::DisplayRole )
    {
        switch ( columnNum )
        {
            case EServerColumns::eServerConnectedID:
                return tr( "Connected ID" );
            case EServerColumns::eUserName:
                return tr( "%2" ).arg( ( *pos ).second.second->displayName() );
            case EServerColumns::eIconStatus:
                return tr( "Icon Status" );
        }
    }
    else if ( role == Qt::DecorationRole )
    {
        switch ( columnNum )
        {
            case EServerColumns::eServerConnectedID:
                return {};
            case EServerColumns::eUserName:
                return ( *pos ).second.second->icon();
            case EServerColumns::eIconStatus:
                return {};
        }
    }
    return {};
}

bool CUsersModel::hasUsersWithConnectedIDNeedingUpdate() const
{
    for ( auto && ii : fUsers )
    {
        if ( ii->connectedIDNeedsUpdate() )
            return true;
    }
    return false;
}

std::list< std::shared_ptr< CUserData > > CUsersModel::usersWithConnectedIDNeedingUpdate() const
{
    std::list< std::shared_ptr< CUserData > > retVal;
    for ( auto && ii : fUsers )
    {
        if ( ii->connectedIDNeedsUpdate() )
            retVal.push_back( ii );
    }
    return retVal;
}

void CUsersModel::loadAvatars( std::shared_ptr< CSyncSystem > syncSystem ) const
{
    for ( auto && user : fUsers )
    {
        for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
        {
            auto server = fSettings->serverInfo( ii );
            if ( !server->isEnabled() )
                continue;
            auto serverName = server->keyName();
            if ( user->hasImageTagInfo( serverName ) )
                syncSystem->requestGetUserImage( serverName, user->getUserID( serverName ) );
        }
    }
}

QVariant CUsersModel::getColor( const QModelIndex & index, bool background ) const
{
    if ( !index.isValid() )
        return {};

    auto userData = fUsers[ index.row() ];
    if ( index.column() == eConnectedID )
    {
        if ( userData->connectedIDNeedsUpdate() )
            return fSettings->dataMissingColor( background );
    }

    if ( index.column() <= eFirstServerColumn )
        return {};

    auto pos = fColumnToServerInfo.find( index.column() );
    if ( pos == fColumnToServerInfo.end() )
        return {};

    if ( !userData->onServer( (*pos).second.second->keyName() ) )
    {
        return fSettings->dataMissingColor( background );
    }

    // its on the server
    bool dataSame = false;
    switch ( perServerColumn( index.column() ) )
    {
        case eUserName:
            dataSame = userData->allUserNamesTheSame();
            break;
        case eServerConnectedID:
            dataSame = userData->allConnectIDTheSame();
            break;
        case eIconStatus:
            dataSame = userData->allIconsTheSame();
            break;
    }

    if ( dataSame )
        return {};

    auto older = fSettings->mediaDestColor( background );
    auto newer = fSettings->mediaSourceColor( background );
    auto serverInfo = this->serverInfo( index );
    auto serverName = serverInfo ? serverInfo->keyName() : QString();
    auto isOlder = userData->needsUpdating( serverName );

    return isOlder ? older : newer;
}

CUsersModel::SUsersSummary CUsersModel::settingsChanged()
{
    beginResetModel();
    endResetModel();
    return getMediaSummary();
}

CUsersModel::SUsersSummary CUsersModel::getMediaSummary() const
{
    SUsersSummary retVal;

    for ( auto && ii : fUsers )
    {
        retVal.fTotal++;
        if ( ii->canBeSynced() )
            retVal.fMissing++;
        else
            retVal.fSyncable++;
    }
    return retVal;
}

std::shared_ptr< CUserData > CUsersModel::userDataForName( const QString & name ) const
{
    for ( auto && ii : fUsers )
    {
        if ( ii->isUser( name ) )
            return ii;
    }
    return {};
}

std::shared_ptr< CUserData > CUsersModel::userData( const QModelIndex & idx ) const
{
    if ( !idx.isValid() )
        return {};
    if ( ( idx.row() < 0 ) || ( idx.row() >= fUsers.size() ) )
        return {};
    return fUsers[ idx.row() ];
}

QModelIndex CUsersModel::indexForUser( std::shared_ptr< CUserData > user, int column ) const
{
    if ( !user )
        return {};
    for ( size_t ii = 0; ii < fUsers.size(); ++ii )
    {
        if ( fUsers[ ii ] == user )
        {
            return index( static_cast<int>( ii ), column, {} );
        }
    }
    return {};
}

void CUsersModel::setUserImage( const QString & serverName, const QString & userID, const QByteArray & data )
{
    auto image = QImage::fromData( data );
    if ( !image.isNull() )
    {
        auto user = findUser( serverName, userID );
        if ( !user )
            return;

        user->setAvatar( serverName, fSettings->serverCnt(), image );
        emit dataChanged( indexForUser( user, 0 ), indexForUser( user, columnCount() - 1 ) );
    }

}

std::shared_ptr< CUserData > CUsersModel::findUser( const QString & serverName, const QString & userID ) const
{
    for ( auto && ii : fUsers )
    {
        if ( ii->isUser( serverName, userID ) )
            return ii;
    }
    return {};
}

void CUsersModel::updateUserConnectID( const QString & serverName, const QString & userID, const QString & connectID )
{
    auto user = findUser( serverName, userID );
    if ( !user )
        return;

    user->setConnectedID( serverName, connectID );
    emit dataChanged( indexForUser( user, 0 ), indexForUser( user, columnCount() - 1 ) );
}

void CUsersModel::slotSettingsChanged()
{
    clear();
}

void CUsersModel::clear()
{
    beginResetModel();
    fUsers.clear();
    fUserMap.clear();
    setupColumns();
    endResetModel();
}

std::shared_ptr< CUserData > CUsersModel::getUserData( const QString & name, bool exhaustiveSearch ) const
{
    if ( name.isEmpty() )
        return {};

    auto pos = fUserMap.find( name );
    if ( pos != fUserMap.end() )
        return ( *pos ).second;
    if ( exhaustiveSearch )
    {
        for ( auto && ii : fUsers )
        {
            if ( ii->isUser( name ) )
                return ii;
        }
    }

    return {};
}

std::vector< std::shared_ptr< CUserData > > CUsersModel::getAllUsers( bool sorted ) const
{
    if ( sorted )
    {
        std::vector< std::shared_ptr< CUserData > > retVal;
        retVal.reserve( fUserMap.size() );
        for ( auto && ii : fUserMap )
        {
            retVal.push_back( ii.second );
        }
        return std::move( retVal );
    }
    else
    {
        return fUsers;
    }
}

std::shared_ptr< CUserData > CUsersModel::loadUser( const QString & serverName, const QJsonObject & user )
{
    //qDebug().noquote().nospace() << QJsonDocument( user ).toJson();

    auto currName = user[ "Name" ].toString();
    auto userID = user[ "Id" ].toString();
    if ( currName.isEmpty() || userID.isEmpty() )
        return {};

    auto linkType = user[ "ConnectLinkType" ].toString();
    QString connectedID;
    if ( linkType == "LinkedUser" )
        connectedID = user[ "ConnectUserName" ].toString();

    auto userData = getUserData( connectedID );
    if ( !userData )
        userData = getUserData( currName, true );

    if ( !userData )
    {
        userData = std::make_shared< CUserData >( serverName, userID );
        userData->setName( serverName, currName );
        userData->setUserID( serverName, userID );
        userData->setConnectedID( serverName, connectedID );

        beginInsertRows( QModelIndex(), static_cast<int>( fUsers.size() ), static_cast<int>( fUsers.size() ) );
        fUsers.push_back( userData );
        fUserMap[ userData->sortName( fSettings ) ] = userData;
        endInsertRows();
    }

    userData->setName( serverName, currName );
    userData->setUserID( serverName, userID );
    userData->setConnectedID( serverName, connectedID );
    userData->setLastModified( serverName, { user[ "DateCreated" ].toVariant().toDateTime(), user[ "LastActivityDate" ].toVariant().toDateTime(), user[ "LastLoginDate" ].toVariant().toDateTime() } );

    emit dataChanged( indexForUser( userData, 0 ), indexForUser( userData, columnCount() - 1 ) );

    if ( user.contains( "PrimaryImageTag" ) )
    {
        userData->setImageTagInfo( serverName, user[ "PrimaryImageTag" ].toString(), user.contains( "PrimaryImageAspectRatio" ) ? user[ "PrimaryImageAspectRatio" ].toDouble() : 1.0 );
    }
        
    return userData;
}

CUsersFilterModel::CUsersFilterModel( bool forUserSelection, QObject * parent ) :
    QSortFilterProxyModel( parent ),
    fForUserSelection( forUserSelection )
{
    setDynamicSortFilter( true );
}

bool CUsersFilterModel::filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const
{
    if ( !sourceModel() )
        return true;
    auto childIdx = sourceModel()->index( source_row, 0, source_parent );
    return childIdx.data( CUsersModel::eShowItemRole ).toBool();
}

bool CUsersFilterModel::filterAcceptsColumn( int source_col, const QModelIndex & source_parent ) const
{
    if ( !sourceModel() )
        return true;
#ifdef NDEBUG
    if ( source_col == CUsersModel::eAllNames )
        return false;
#endif

    if ( fForUserSelection )
    {
        if ( source_col < CUsersModel::eFirstServerColumn )
            return true;
        auto idx = sourceModel()->index( 0, source_col, source_parent );
        return idx.data( CUsersModel::eIsUserNameColumnRole ).toBool();
    }
    else
    {
        if ( source_col < CUsersModel::eFirstServerColumn )
            return false;
        return true;
    }
}

QVariant CUsersFilterModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    auto retVal = QSortFilterProxyModel::headerData( section, orientation, role );
    if ( !fForUserSelection )
    {
        auto index = this->index( 0, section );
        auto isUserNameColumn = index.data( CUsersModel::eIsUserNameColumnRole ).toBool();
        if ( isUserNameColumn )
        {
            if ( role == Qt::DisplayRole )
            {
                retVal = tr( "User Name" );
                //auto srcColumn = this->mapToSource( index ).column();
                //retVal = tr( "%1 - %2 - %3" ).arg( srcColumn ).arg( section ).arg( retVal.toString() );
            }
            else if ( role == Qt::DecorationRole )
                retVal = QVariant();
        }
    }
    return retVal;
}

QVariant CUsersFilterModel::data( const QModelIndex & index, int role /*= Qt::DisplayRole */ ) const
{
    auto retVal = QSortFilterProxyModel::data( index, role );
    if ( !fForUserSelection && ( role != CUsersModel::eIsUserNameColumnRole ) )
    {
        auto isUserNameColumn = index.data( CUsersModel::eIsUserNameColumnRole ).toBool();
        if ( isUserNameColumn )
        {
            if ( role == Qt::DecorationRole )
                return index.data( CUsersModel::eSyncDirectionIconRole );
        }
    }
    return retVal;
}

