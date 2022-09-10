// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef _USERINFOCOMPARE_H
#define _USERINFOCOMPARE_H

#include "TabPageBase.h"
#include <memory>
#include <QPointer>
class QMenu;
class QAction;
class QToolBar;
namespace Ui
{
    class CUserInfoCompare;
}

class CServerInfo;
class CUserData;
class CSettings;
class CSyncSystem;
class CUsersModel;
class CUsersFilterModel;
class CDataTree;
class CProgressSystem;
class CTabUIInfo;
class CMediModel;

class CUserInfoCompare : public CTabPageBase
{
    Q_OBJECT
public:
    CUserInfoCompare( QWidget * parent = nullptr );
    virtual ~CUserInfoCompare() override;

    virtual void setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CProgressSystem > progressSystem ) override;
    virtual void setupActions();

    virtual bool okToClose() override;
    virtual void resetPage() override;
    virtual void loadSettings() override;

    virtual std::shared_ptr< CTabUIInfo > getUIInfo() const override;
    virtual void loadingUsersFinished();

    virtual QSplitter * getDataSplitter() const override;

    void onlyShowSyncableUsers();
    void onlyShowUsersWithDifferences();
    void showUsersWithIssues();

Q_SIGNALS:
    void sigSettingsLoaded();
    void sigModelDataChanged();
    void sigAddToLog( int msgType, const QString & msg );
    void sigAddInfoToLog( const QString & msg );

public Q_SLOTS:
    void slotCanceled();
    void slotModelDataChanged();
    void slotSettingsChanged();

private Q_SLOTS:
    void slotReloadCurrentUser();

    void slotRepairUserConnectedIDs();

    void slotCurrentUserChanged( const QModelIndex & index );
    void slotUsersContextMenu( CDataTree * dataTree, const QPoint & pos );

    void slotToggleOnlyShowSyncableUsers();
    void slotToggleOnlyShowUsersWithDifferences();
    void slotToggleShowUsersWithIssues();


    void slotSetConnectID();
    void slotAutoSetConnectID();
private:
    std::shared_ptr< CUserData > getCurrUserData() const;
    std::shared_ptr< CUserData > getUserData( QModelIndex idx ) const;
    std::shared_ptr< const CServerInfo > getServerInfo( QModelIndex idx ) const;

    void reset();

    void loadServers();

    std::unique_ptr< Ui::CUserInfoCompare > fImpl;
    CUsersFilterModel * fUsersFilterModel{ nullptr };

    QAction * fActionReloadCurrentUser{ nullptr };

    QMenu * fProcessMenu{ nullptr };
    QAction * fActionRepairUserConnectedIDs{ nullptr };

    QAction * fActionOnlyShowSyncableUsers{ nullptr };
    QAction * fActionOnlyShowUsersWithDifferences{ nullptr };
    QAction * fActionShowUsersWithIssues{ nullptr };

    std::list< QAction * > fEditActions;
    QToolBar * fToolBar{ nullptr };
    CDataTree * fContextTree{ nullptr };
};
#endif
