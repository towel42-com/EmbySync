#include "UsersModel.h"
#include "UserData.h"
#include "Settings.h"

#include <QColor>
#include <set>
#include <QJsonObject>

CUsersModel::CUsersModel( std::shared_ptr< CSettings > settings, QObject * parent ) :
    QAbstractTableModel( parent ),
    fSettings( settings )
{

}

int CUsersModel::rowCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;

    return static_cast<int>( fUsers.size() );
}

int CUsersModel::columnCount( const QModelIndex & parent /* = QModelIndex() */ ) const
{
    if ( parent.isValid() )
        return 0;
    return static_cast<int>( eFirstServerColumn ) + fSettings->serverCnt() + 1;
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

    if ( role != Qt::DisplayRole )
        return {};

    if ( index.column() == eDisplayName )
        return userData->displayName();
    else if ( index.column() == eConnectedID )
        return userData->connectedID();
    return userData->name( fSettings->serverKeyName( serverNum( index.column() ) ) );
}

int CUsersModel::serverNum( int columnNum ) const
{
    if ( columnNum < eFirstServerColumn )
        return -1;
    return columnNum - ( eFirstServerColumn + 1 );
}

QVariant CUsersModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    if ( section < 0 || section > columnCount() )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( role != Qt::DisplayRole )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );

    if ( section == eConnectedID )
        return tr( "Connected ID" );
    else if ( section == eDisplayName )
        return tr( "All Names" );
    return fSettings->friendlyServerName( serverNum( section ) );
}

QVariant CUsersModel::getColor( const QModelIndex & index, bool background ) const
{
    if ( index.column() <= eFirstServerColumn )
        return {};

    auto userData = fUsers[ index.row() ];
    if ( !userData->onServer( fSettings->serverKeyName( serverNum( index.column() ) ) ) )
    {
        return fSettings->dataMissingColor( background );
    }

    return {};
}

void CUsersModel::slotSettingsChanged()
{
    beginResetModel();
    endResetModel();
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

std::shared_ptr< CUserData > CUsersModel::userDataForName( const QString & name )
{
    for ( auto && ii : fUsers )
    {
        if ( ii->isUser( name ) )
            return ii;
    }
    return {};
}

std::shared_ptr< CUserData > CUsersModel::userData( const QModelIndex & idx )
{
    if ( !idx.isValid() )
        return {};
    if ( ( idx.row() < 0 ) || ( idx.row() >= fUsers.size() ) )
        return {};
    return fUsers[ idx.row() ];
}
void CUsersModel::clear()
{
    beginResetModel();
    fUsers.clear();
    fUserMap.clear();
    endResetModel();
}

std::shared_ptr< CUserData > CUsersModel::getUserData( const QString & name ) const
{
    auto pos = fUserMap.find( name );
    if ( pos == fUserMap.end() )
        return {};

    auto userData = ( *pos ).second;
    if ( !userData )
        return {};
    return userData;
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

std::shared_ptr< CUserData > CUsersModel::loadUser( const QJsonObject & user, const QString & serverName )
{
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
        userData = getUserData( currName );

    if ( !userData )
    {
        userData = std::make_shared< CUserData >( currName, connectedID, userID, serverName );
        beginInsertRows( QModelIndex(), static_cast<int>( fUsers.size() ), static_cast<int>( fUsers.size() ) );
        fUsers.push_back( userData );
        fUserMap[ userData->sortName( fSettings ) ] = userData;
        endInsertRows();
    }
    else
    {
        userData->setName( currName, serverName );
        userData->setUserID( userID, serverName );

        int ii = 0;
        for ( ; ii < static_cast<int>( fUsers.size() ); ++ii )
        {
            if ( fUsers[ ii ] == userData )
                break;
        }
        Q_ASSERT( ii != fUsers.size() );
        dataChanged( index( ii, 0 ), index( ii, columnCount() - 1 ) );
    }

    return userData;
}

CUsersFilterModel::CUsersFilterModel( QObject * parent ) :
    QSortFilterProxyModel( parent )
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



