#include "UsersModel.h"
#include "UserData.h"
#include "Settings.h"

#include <QColor>

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
    return 3;
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

    if ( role == ECustomRoles::eNameRole )
        return userData->name();

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
        case eName: return userData->name();
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
        case eName: return "Name";
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

void CUsersModel::setUsers( const std::list< std::shared_ptr< CUserData > > & users )
{
    beginResetModel();
    clear();
    fUsers = { users.begin(), users.end() };
    for ( size_t ii = 0; ii < fUsers.size(); ++ii )
        fUsersToPos[ fUsers[ ii ] ] = ii;
    endResetModel();
}

void CUsersModel::addUser( std::shared_ptr< CUserData > userData )
{
    auto pos = fUsersToPos.find( userData );
    if ( pos == fUsersToPos.end() )
    {
        beginInsertRows( QModelIndex(), static_cast<int>( fUsers.size() ), static_cast<int>( fUsers.size() ) );
        fUsersToPos[ userData ] = fUsers.size();
        fUsers.push_back( userData );
        endInsertRows();
    }
}

std::shared_ptr< CUserData > CUsersModel::userDataForName( const QString & name )
{
    for ( auto && ii : fUsers )
    {
        if ( ii->name() == name )
            return ii;
    }
    return {};
}

void CUsersModel::clear()
{
    beginResetModel();
    fUsers.clear();
    fUsersToPos.clear();
    endResetModel();
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



