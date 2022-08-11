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
    return static_cast<int>( eOnRHSServer ) + 1;
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
        return userData->onLHSServer() && userData->onRHSServer();
    }

    if ( role == ECustomRoles::eLHSNameRole )
        return userData->name( true );
    else if ( role == ECustomRoles::eRHSNameRole )
        return userData->name( false );
    else if ( role == ECustomRoles::eConnectedIDRole )
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

    switch ( index.column() )
    {
        case eLHSName: return userData->name( true );
        case eRHSName: return userData->name( false );
        case eConnectedID: return userData->connectedID();
        case eOnLHSServer: return userData->onLHSServer() ? "Yes" : "No";
        case eOnRHSServer: return userData->onRHSServer() ? "Yes" : "No";
        default:
            return {};
    }

    return {};
}

QVariant CUsersModel::getColor( const QModelIndex & index, bool background ) const
{
    auto userData = fUsers[ index.row() ];

    if ( ( index.column() == eOnLHSServer ) && !userData->onLHSServer() )
    {
        return fSettings->dataMissingColor( background );
    }

    if ( ( index.column() == eOnRHSServer ) && !userData->onRHSServer() )
    {
        return fSettings->dataMissingColor( background );
    }

    return {};
}

QVariant CUsersModel::headerData( int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole */ ) const
{
    if ( section < 0 || section > columnCount() )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( role != Qt::DisplayRole )
        return QAbstractTableModel::headerData( section, orientation, role );
    if ( orientation != Qt::Horizontal )
        return QAbstractTableModel::headerData( section, orientation, role );

    switch ( section )
    {
        case eLHSName: return "LHS Name";
        case eRHSName: return "RHS Name";
        case eConnectedID: return "Connected ID";
        case eOnLHSServer: return "On LHS Server?";
        case eOnRHSServer: return "On RHS Server?";
    };

    return {};
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
        if ( !ii->onLHSServer() || !ii->onRHSServer() )
        {
            retVal.fMissing++;
            continue;
        }
        retVal.fSyncable++;
    }
    return retVal;
}

std::shared_ptr< CUserData > CUsersModel::userDataForName( const QString & name )
{
    for ( auto && ii : fUsers )
    {
        if ( ( ii->name( true ) == name ) || ( ii->name( false ) == name ) || ( ii->connectedID() == name ) )
            return ii;
    }
    return {};
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
        retVal.reserve( fUsers.size() );
        std::map< QString, std::shared_ptr< CUserData > > sortedUsers;
        for ( auto && ii : fUsers )
        {
            if ( ii->name( true ).isEmpty() && ii->name( false ).isEmpty() && ii->connectedID().isEmpty() )
                continue;
            if ( !ii->name( true ).isEmpty() )
                sortedUsers[ ii->name( true ) ] = ii;
            else if ( !ii->name( false ).isEmpty() )
                sortedUsers[ ii->name( false ) ] = ii;
            else if ( !ii->connectedID().isEmpty() )
                sortedUsers[ ii->connectedID() ] = ii;
        }
        for ( auto && ii : sortedUsers )
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

std::shared_ptr< CUserData > CUsersModel::loadUser( const QJsonObject & user, bool isLHSServer )
{
    auto currName = user[ "Name" ].toString();
    auto id = user[ "Id" ].toString();
    if ( currName.isEmpty() || id.isEmpty() )
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
        userData = std::make_shared< CUserData >( currName, connectedID, isLHSServer );
        beginInsertRows( QModelIndex(), static_cast<int>( fUsers.size() ), static_cast<int>( fUsers.size() ) );
        fUsers.push_back( userData );
        if ( !connectedID.isEmpty() )
            fUserMap[ userData->connectedID() ] = userData;
        else
            fUserMap[ userData->name( isLHSServer ) ] = userData;
        endInsertRows();
    }
    int ii = 0;
    for ( ; ii < static_cast< int >( fUsers.size() ); ++ii )
    {
        if ( fUsers[ ii ] == userData )
            break;
    }
    Q_ASSERT( ii != fUsers.size() );
    userData->setName( currName, isLHSServer );
    userData->setUserID( id, isLHSServer );
    dataChanged( index( ii, 0 ), index( ii, columnCount() - 1 ) );
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



