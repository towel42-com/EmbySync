#ifndef __USERSMODEL_H
#define __USERSMODEL_H

#include "IServerForColumn.h"
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <memory>
#include "SABUtils/HashUtils.h"

#include <unordered_set>
#include <vector>

class CSettings;
class CUserData;
class CServerInfo;
class CServerModel;
class CSyncSystem;
class CUsersModel : public QAbstractTableModel, public IServerForColumn
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
        eServerConnectedIDType,
        ePrefix,
        eIconStatus,
        eDateCreated,
        eLastActivityDate,
        eLastLoginDate,
        eEnableAutoLogin,

        eAudioLanguagePreference,
        ePlayDefaultAudioTrack,
        eSubtitleLanguagePreference,
        eDisplayMissingEpisodes,
        eSubtitleMode,
        eEnableLocalPassword,
        eOrderedViews,
        eLatestItemsExcludes,
        eMyMediaExcludes,
        eHidePlayedInLatest,
        eRememberAudioSelections,
        eRememberSubtitleSelections,
        eEnableNextEpisodeAutoPlay,
        eResumeRewindSeconds,
        eIntroSkipMode,

        eServerColCount
    };

    enum ECustomRoles
    {
        eShowItemRole=Qt::UserRole+1,
        eConnectedIDRole,
        eConnectedIDValidRole,
        eIsUserNameColumnRole,
        eSyncDirectionIconRole,
        eIsMissingOnServerFGColor,
        eIsMissingOnServerBGColor
    };

    CUsersModel( std::shared_ptr< CSettings > settings, std::shared_ptr< CServerModel > serverModel, QObject * parent=nullptr );

    int userCnt() const;
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const override;
    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const override;

    std::shared_ptr< const CServerInfo > serverInfo( const QModelIndex & index ) const;

    std::shared_ptr<const CServerInfo> serverInfo( int column ) const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    struct SUsersSummary
    {
        int fTotal{ 0 };
        int fMissing{ 0 };
        int fSyncable{ 0 };
    };

    SUsersSummary settingsChanged();
    SUsersSummary getMediaSummary() const;

    QModelIndex indexForUser( std::shared_ptr< CUserData > user, int column = 0 ) const;

    void setUserAvatar( const QString & serverName, const QString & userID, const QByteArray & data );

    void updateUserConnectID( const QString & serverName, const QString & userID, const QString & idType, const QString & connectID );

    void clear();

    virtual QString serverForColumn( int column ) const override;
    virtual std::list< int > columnsForBaseColumn( int baseColumn ) const override;

    using TUserDataVector = std::vector< std::shared_ptr< CUserData > >;

    std::shared_ptr< CUserData > loadUser( const QString & serverName, const QJsonObject & user );
    TUserDataVector getAllUsers( bool sorted ) const;

    bool hasUsersWithConnectedIDNeedingUpdate() const;
    std::list< std::shared_ptr< CUserData > > usersWithConnectedIDNeedingUpdate() const;

    void loadAvatars( std::shared_ptr< CSyncSystem > syncSystem ) const;

    std::shared_ptr< CUserData > userData( int userNum ) const;
    std::shared_ptr< CUserData > userData( const QModelIndex & idx ) const;
    std::shared_ptr< CUserData > userData( const QString & name, bool exhaustiveSearch ) const;
    std::shared_ptr< CUserData > userDataOnServer( const QString & serverName, const QString & userID ) const;

    using iterator = typename TUserDataVector::iterator;
    using const_iterator = typename TUserDataVector::const_iterator;

    iterator begin() { return fUsers.begin(); }
    iterator end() { return fUsers.end(); }
    const_iterator begin() const { return fUsers.cbegin(); }
    const_iterator end() const { return fUsers.cend(); }
public Q_SLOTS:
    void slotSettingsChanged();
    void slotServerInfoChanged();

private:
    std::shared_ptr< CUserData > userDataExhaustive( const QString & name ) const;

    int columnsPerServer() const;
    int perServerColumn( int column ) const;

    void setupColumns();
    int serverNum( int columnNum ) const;

    QVariant getColor( const QModelIndex & index, bool background, bool missingOnly=false ) const;
        
    std::map< QString, std::shared_ptr< CUserData > > fUserMap;
    TUserDataVector fUsers;
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CServerModel > fServerModel;

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
