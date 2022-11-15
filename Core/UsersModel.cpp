#include "UsersModel.h"
#include "UserData.h"
#include "UserServerData.h"
#include "Settings.h"
#include "ServerInfo.h"
#include "SyncSystem.h"
#include "ServerModel.h"

#include <QColor>
#include <set>
#include <QJsonObject>
#include <QJsonDocument>
#include <QImage>

CUsersModel::CUsersModel( std::shared_ptr< CSettings > settings, std::shared_ptr< CServerModel > serverModel, QObject * parent ) :
    QAbstractTableModel( parent ),
    fSettings( settings ),
    fServerModel( serverModel )
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
    int serverNum = 0;
    for ( auto && serverInfo : *fServerModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;
        for ( int jj = 0; jj < colsPerServer; ++jj )
        {
            fServerNumToColumn[ serverNum ] = columnNum;
            fColumnToServerInfo[ columnNum ] = std::make_pair( serverNum, serverInfo );

            columnNum++;
        }

        disconnect( serverInfo.get(), &CServerInfo::sigServerInfoChanged, this, &CUsersModel::slotServerInfoChanged );
        connect( serverInfo.get(), &CServerInfo::sigServerInfoChanged, this, &CUsersModel::slotServerInfoChanged );
        serverNum++;
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

int CUsersModel::userCnt() const
{
    return rowCount();
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
    retVal += fServerModel->enabledServerCnt() * columnsPerServer();
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

    if ( role == eIsMissingOnServerBGColor )
    {
        auto color = getColor( index, true, true );
        if ( !color.isValid() )
            return {};
        return color;
    }

    if ( role == eIsMissingOnServerBGColor )
    {
        auto color = getColor( index, false, true );
        if ( !color.isValid() )
            return {};
        return color;
    }

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

    if ( role == Qt::CheckStateRole )
    {
        switch ( columnNum )
        {
            case EServerColumns::eEnableAutoLogin:
                return userData->enableAutoLogin( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            case EServerColumns::ePlayDefaultAudioTrack:
                return userData->playDefaultAudioTrack( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            case EServerColumns::eDisplayMissingEpisodes:
                return userData->displayMissingEpisodes( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            case EServerColumns::eEnableLocalPassword:
                return userData->enableLocalPassword( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            case EServerColumns::eHidePlayedInLatest:
                return userData->hidePlayedInLatest( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            case EServerColumns::eRememberAudioSelections:
                return userData->rememberAudioSelections( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            case EServerColumns::eRememberSubtitleSelections:
                return userData->rememberSubtitleSelections( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            case EServerColumns::eEnableNextEpisodeAutoPlay:
                return userData->enableNextEpisodeAutoPlay( serverName ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
            default:
                break;
        };
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
        case EServerColumns::eServerConnectedIDType:
            return userData->connectedIDType( serverName );
        case EServerColumns::ePrefix:
            return userData->prefix( serverName );
        case EServerColumns::eEnableAutoLogin:
            return userData->enableAutoLogin( serverName ) ? "Yes" : "No";

        case EServerColumns::eAudioLanguagePreference:
            return userData->audioLanguagePreference( serverName );
        case EServerColumns::ePlayDefaultAudioTrack:
            return userData->playDefaultAudioTrack( serverName ) ? "Yes" : "No";
        case EServerColumns::eSubtitleLanguagePreference:
            return userData->subtitleLanguagePreference( serverName );
        case EServerColumns::eDisplayMissingEpisodes:
            return userData->displayMissingEpisodes( serverName ) ? "Yes" : "No";
        case EServerColumns::eSubtitleMode:
            return userData->subtitleMode( serverName );
        case EServerColumns::eEnableLocalPassword:
            return userData->enableLocalPassword( serverName ) ? "Yes" : "No";
        case EServerColumns::eOrderedViews:
            return userData->orderedViews( serverName );
        case EServerColumns::eLatestItemsExcludes:
            return userData->latestItemsExcludes( serverName );
        case EServerColumns::eMyMediaExcludes:
            return userData->myMediaExcludes( serverName );
        case EServerColumns::eHidePlayedInLatest:
            return userData->hidePlayedInLatest( serverName ) ? "Yes" : "No";
        case EServerColumns::eRememberAudioSelections:
            return userData->rememberAudioSelections( serverName ) ? "Yes" : "No";
        case EServerColumns::eRememberSubtitleSelections:
            return userData->rememberSubtitleSelections( serverName ) ? "Yes" : "No";
        case EServerColumns::eEnableNextEpisodeAutoPlay:
            return userData->enableNextEpisodeAutoPlay( serverName ) ? "Yes" : "No";
        case EServerColumns::eResumeRewindSeconds:
            return userData->resumeRewindSeconds( serverName );
        case EServerColumns::eIntroSkipMode:
            return userData->introSkipMode( serverName );

        case EServerColumns::eIconStatus:
            return std::get< 1 >( userData->getAvatarInfo( serverName ) );
        case EServerColumns::eDateCreated:
            return userData->getDateCreated( serverName );
        case EServerColumns::eLastActivityDate:
            return userData->getLastActivityDate( serverName );
        case EServerColumns::eLastLoginDate:
            return userData->getLastLoginDate( serverName );
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
    int column = index.column();
    return serverInfo( column );
}

std::shared_ptr<const CServerInfo> CUsersModel::serverInfo( int column ) const
{
    auto pos = fColumnToServerInfo.find( column );
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
            case EServerColumns::eServerConnectedIDType:
                return tr( "Connected ID Type" );
            case EServerColumns::ePrefix:
                return tr( "Prefix" );
            case EServerColumns::eEnableAutoLogin:
                return tr( "Enable Auto Login" );
            
            case EServerColumns::eAudioLanguagePreference:
                return tr( "Audio Language Preference" );
            case EServerColumns::ePlayDefaultAudioTrack:
                return tr( "Play Default Audio Track" );
            case EServerColumns::eSubtitleLanguagePreference:
                return tr( "Subtitle Language Preference" );
            case EServerColumns::eDisplayMissingEpisodes:
                return tr( "Display Missing Episodes" );
            case EServerColumns::eSubtitleMode:
                return tr( "Subtitle Mode" );
            case EServerColumns::eEnableLocalPassword:
                return tr( "Enable Local Password" );
            case EServerColumns::eOrderedViews:
                return tr( "Ordered Views" );
            case EServerColumns::eLatestItemsExcludes:
                return tr( "Latest Items Excludes" );
            case EServerColumns::eMyMediaExcludes:
                return tr( "My Media Excludes" );
            case EServerColumns::eHidePlayedInLatest:
                return tr( "Hide Played InLatest" );
            case EServerColumns::eRememberAudioSelections:
                return tr( "Remember Audio Selections" );
            case EServerColumns::eRememberSubtitleSelections:
                return tr( "Remember Subtitle Selections" );
            case EServerColumns::eEnableNextEpisodeAutoPlay:
                return tr( "Enable Next Episode Auto Play" );
            case EServerColumns::eResumeRewindSeconds:
                return tr( "Resume Rewind Seconds" );
            case EServerColumns::eIntroSkipMode:
                return tr( "Intro Skip Mode" );

            case EServerColumns::eUserName:
                return tr( "%2" ).arg( ( *pos ).second.second->displayName() );
            case EServerColumns::eIconStatus:
                return tr( "Icon Ratio" );
            case EServerColumns::eDateCreated:
                return tr( "Date Created" );
            case EServerColumns::eLastActivityDate:
                return tr( "Last Activity" );
            case EServerColumns::eLastLoginDate:
                return tr( "Last Login" );
        }
    }
    else if ( role == Qt::DecorationRole )
    {
        switch ( columnNum )
        {
            case EServerColumns::eUserName:
                return ( *pos ).second.second->icon();
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
        for ( auto && serverInfo : *fServerModel )
        {
            if ( !serverInfo->isEnabled() )
                continue;
            auto serverName = serverInfo->keyName();
            if ( user->hasAvatarInfo( serverName ) )
                syncSystem->requestGetUserAvatar( serverName, user->getUserID( serverName ) );
        }
    }
}

QVariant CUsersModel::getColor( const QModelIndex & index, bool background, bool missingOnly /*=false*/ ) const
{
    if ( !index.isValid() )
        return {};

    auto userData = fUsers[ index.row() ];
    if ( index.column() == eConnectedID )
    {
        if ( userData->connectedIDNeedsUpdate() )
            return fSettings->dataMissingColor( background );
    }

    if ( index.column() < eFirstServerColumn )
        return {};

    auto pos = fColumnToServerInfo.find( index.column() );
    if ( pos == fColumnToServerInfo.end() )
        return {};

    if ( !userData->onServer( ( *pos ).second.second->keyName() ) )
    {
        return fSettings->dataMissingColor( background );
    }

    if ( missingOnly )
        return {};

    // its on the server
    bool dataSame = false;
    switch ( perServerColumn( index.column() ) )
    {
        case eUserName:
            dataSame = userData->allUserDataTheSame();
            break;
        case eServerConnectedID:
            dataSame = userData->allConnectIDTheSame();
            break;
        case eServerConnectedIDType:
            dataSame = userData->allConnectIDTypeTheSame();
            break;
        case ePrefix:
            dataSame = userData->allPrefixTheSame();
            break;
        case eEnableAutoLogin:
            dataSame = userData->allEnableAutoLoginTheSame();
            break;
        case EServerColumns::eAudioLanguagePreference:
            dataSame = userData->allAudioLanguagePreferenceTheSame();
        case EServerColumns::ePlayDefaultAudioTrack:
            dataSame = userData->allPlayDefaultAudioTrackTheSame();
        case EServerColumns::eSubtitleLanguagePreference:
            dataSame = userData->allSubtitleLanguagePreferenceTheSame();
        case EServerColumns::eDisplayMissingEpisodes:
            dataSame = userData->allDisplayMissingEpisodesTheSame();
        case EServerColumns::eSubtitleMode:
            dataSame = userData->allSubtitleModeTheSame();
        case EServerColumns::eEnableLocalPassword:
            dataSame = userData->allEnableLocalPasswordTheSame();
        case EServerColumns::eOrderedViews:
            dataSame = userData->allOrderedViewsTheSame();
        case EServerColumns::eLatestItemsExcludes:
            dataSame = userData->allLatestItemsExcludesTheSame();
        case EServerColumns::eMyMediaExcludes:
            dataSame = userData->allMyMediaExcludesTheSame();
        case EServerColumns::eHidePlayedInLatest:
            dataSame = userData->allHidePlayedInLatestTheSame();
        case EServerColumns::eRememberAudioSelections:
            dataSame = userData->allRememberAudioSelectionsTheSame();
        case EServerColumns::eRememberSubtitleSelections:
            dataSame = userData->allRememberSubtitleSelectionsTheSame();
        case EServerColumns::eEnableNextEpisodeAutoPlay:
            dataSame = userData->allEnableNextEpisodeAutoPlayTheSame();
        case EServerColumns::eResumeRewindSeconds:
            dataSame = userData->allResumeRewindSecondsTheSame();
        case EServerColumns::eIntroSkipMode:
            dataSame = userData->allIntroSkipModeTheSame();

        case eIconStatus:
            dataSame = userData->allIconInfoTheSame();
            break;
        case eDateCreated:
            dataSame = userData->allDateCreatedSame();
            break;
        case eLastActivityDate:
            dataSame = userData->allLastActivityDateSame();
            break;
        case eLastLoginDate:
            dataSame = userData->allLastLoginDateSame();
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

void CUsersModel::setUserAvatar( const QString & serverName, const QString & userID, const QByteArray & data )
{
    auto image = QImage::fromData( data );
    if ( !image.isNull() )
    {
        auto user = userDataOnServer( serverName, userID );
        if ( !user )
            return;

        user->setAvatar( serverName, fServerModel->serverCnt(), image );
        emit dataChanged( indexForUser( user, 0 ), indexForUser( user, columnCount() - 1 ) );
    }

}

void CUsersModel::updateUserConnectID( const QString & serverName, const QString & userID, const QString & idType, const QString & connectID )
{
    auto user = userDataOnServer( serverName, userID );
    if ( !user )
        return;

    user->setConnectedID( serverName, idType, connectID );
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

QString CUsersModel::serverForColumn( int column ) const
{
    if ( column == CUsersModel::eConnectedID )
        return "<ALL>";
#ifndef NDEBUG
    if ( column == CUsersModel::eAllNames )
        return "<ALL>";
#endif

    auto serverInfo = this->serverInfo( column );
    auto serverName = serverInfo ? serverInfo->keyName() : QString();

    //qDebug() << "Server for column: " << index.column() << serverName;
    return serverName;
}

std::list< int > CUsersModel::columnsForBaseColumn( int baseColumn ) const
{
    std::list< int > retVal;

    auto curr = baseColumn;
    while ( curr <= columnCount() )
    {
        retVal.push_back( curr );
        curr += columnsPerServer();
    }

    return retVal;
}

std::shared_ptr< CUserData > CUsersModel::userData( const QModelIndex & idx ) const
{
    if ( !idx.isValid() )
        return {};
    return userData( idx.row() );
}

std::shared_ptr< CUserData > CUsersModel::userData( int userNum ) const
{
    if ( ( userNum < 0 ) || ( userNum >= fUsers.size() ) )
        return {};
    return fUsers[ userNum ];
}

std::shared_ptr< CUserData > CUsersModel::userDataOnServer( const QString & serverName, const QString & userID ) const
{
    for ( auto && ii : fUsers )
    {
        if ( ii->isUser( serverName, userID ) )
            return ii;
    }
    return {};
}

std::shared_ptr< CUserData > CUsersModel::userDataExhaustive( const QString & name ) const
{
    for ( auto && ii : fUsers )
    {
        if ( ii->isUser( name ) )
            return ii;
    }
    return {};
}

std::shared_ptr< CUserData > CUsersModel::userData( const QString & name, bool exhaustiveSearch ) const
{
    if ( name.isEmpty() )
        return {};

    auto pos = fUserMap.find( name );
    if ( pos != fUserMap.end() )
        return ( *pos ).second;
    if ( exhaustiveSearch )
    {
        return userDataExhaustive( name );
    }

    return {};
}

CUsersModel::TUserDataVector CUsersModel::getAllUsers( bool sorted ) const
{
    if ( sorted )
    {
        TUserDataVector retVal;
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

std::shared_ptr< CUserData > CUsersModel::loadUser( const QString & serverName, const QJsonObject & userObj )
{
    //qDebug().noquote().nospace() << QJsonDocument( userObj ).toJson();
    QString name;
    QString userID;
    QString connectedID;
    std::tie( name, userID, connectedID ) = SUserServerData::getUserNames( userObj );
    if ( name.isEmpty() || userID.isEmpty() )
        return {};

    auto userData = this->userData( connectedID, false );
    if ( !userData )
        userData = this->userData( name, true );

    if ( !userData )
    {
        userData = std::make_shared< CUserData >( serverName, userObj );

        beginInsertRows( QModelIndex(), static_cast<int>( fUsers.size() ), static_cast<int>( fUsers.size() ) );
        fUsers.push_back( userData );
        Q_ASSERT( !userData->sortName( fServerModel ).isEmpty() );
        fUserMap[ userData->sortName( fServerModel ) ] = userData;
        endInsertRows();
    }
    else
    {
        userData->loadFromJSON( serverName, userObj );
        emit dataChanged( indexForUser( userData, 0 ), indexForUser( userData, columnCount() - 1 ) );
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
    else if ( fForUserSelection && ( role != CUsersModel::eIsMissingOnServerFGColor ) && ( role != CUsersModel::eIsMissingOnServerBGColor ) )
    {
        if ( role == Qt::ForegroundRole )
        {
            auto isMissing = index.data( CUsersModel::eIsMissingOnServerFGColor ).value< QColor >();
            if ( isMissing.isValid() )
                return isMissing;
            else
                return {};
        }

        if ( role == Qt::BackgroundRole )
        {
            auto isMissing = index.data( CUsersModel::eIsMissingOnServerBGColor ).value< QColor >();
            if ( isMissing.isValid() )
                return isMissing;
            else
                return {};
        }
    }
    return retVal;
}

