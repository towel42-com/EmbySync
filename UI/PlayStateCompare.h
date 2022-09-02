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

#ifndef _PLAYSTATECOMPARE_H
#define _PLAYSTATECOMPARE_H

#include <QWidget>
#include <memory>
#include <QPointer>
class QMenu;
class QAction;
class QToolBar;
namespace Ui
{
    class CPlayStateCompare;
}

class CMediaData;
class CUserData;
class CSettings;
class CSyncSystem;
class CMediaModel;
class CMediaFilterModel;
class CUsersModel;
class CUsersFilterModel;
class CMediaWindow;
class CMediaTree;
class CProgressSystem;

class CPlayStateCompare : public QWidget
{
    Q_OBJECT
public:
    CPlayStateCompare( QWidget * parent = nullptr );
    virtual ~CPlayStateCompare() override;

    void setup( std::shared_ptr< CSettings > settings, std::shared_ptr< CUsersModel > userModel );
    void setupActions();

    void setProgressSystem( std::shared_ptr< CProgressSystem > funcs );
    bool okToClose();

    std::shared_ptr< CSyncSystem > syncSystem() const { return fSyncSystem; }
    std::vector< std::shared_ptr< CUserData > > getAllUsers( bool sorted ) const;

    void loadSettings();
    void resetServers();

    void onlyShowSyncableUsers();
    void onlyShowMediaWithDifferences();
    void showMediaWithIssues();

    std::list< QMenu * > getMenus() const;
    std::list< QAction * > getEditActions() const;
    std::list< QToolBar * > getToolBars() const;

Q_SIGNALS:
    void sigSettingsLoaded();
    void sigDataChanged();
    void sigAddToLog( int msgType, const QString & msg );
    void sigAddInfoToLog( const QString & msg );

public Q_SLOTS:
    void slotCanceled();
    void slotDataChanged();
    void slotSettingsChanged();

private Q_SLOTS:
    void slotReloadServers();
    void slotReloadCurrentUser();

    void slotProcess();

    void slotSelectiveProcess();
    void slotRepairUserConnectedIDs();

    void slotUserMediaCompletelyLoaded();
    void slotPendingMediaUpdate();

    void slotCurrentUserChanged( const QModelIndex & index );
    void slotUsersContextMenu( const QPoint & pos );

    void slotToggleOnlyShowSyncableUsers();
    void slotToggleOnlyShowMediaWithDifferences();
    void slotToggleShowMediaWithIssues();


    void slotLoadingUsersFinished();
    void slotUserMediaLoaded();

    void slotSetCurrentMediaItem( const QModelIndex & current );
    void slotViewMedia( const QModelIndex & current );

    void slotViewMediaInfo();
    void slotSetConnectID();
    void slotAutoSetConnectID();
private:
    std::shared_ptr< CMediaData > getMediaData( QModelIndex idx ) const;

    std::shared_ptr< CUserData > getCurrUserData() const;
    std::shared_ptr< CUserData > getUserData( QModelIndex idx ) const;

    void reset();

    void loadServers();

    std::unique_ptr< Ui::CPlayStateCompare > fImpl;
    std::shared_ptr< CSyncSystem > fSyncSystem;
    std::shared_ptr< CUsersModel > fUsersModel;
    CUsersFilterModel * fUsersFilterModel{ nullptr };

    std::shared_ptr< CMediaModel >fMediaModel;
    CMediaFilterModel * fMediaFilterModel{ nullptr };

    QTimer * fPendingMediaUpdateTimer{ nullptr };
    QTimer * fMediaLoadedTimer{ nullptr };

    QPointer< CMediaWindow > fMediaWindow;
    std::vector< CMediaTree * > fMediaTrees;

    QMenu * fReloadMenu{ nullptr };
    QAction * fActionReloadServers{ nullptr };
    QAction * fActionReloadCurrentUser{ nullptr };

    QMenu * fProcessMenu{ nullptr };
    QAction * fActionRepairUserConnectedIDs{ nullptr };
    QAction * fActionProcess{ nullptr };
    QAction * fActionSelectiveProcess{ nullptr };

    QMenu * fViewMenu{ nullptr };
    QAction * fActionViewMediaInformation{ nullptr };

    QAction * fActionOnlyShowSyncableUsers{ nullptr };
    QAction * fActionOnlyShowMediaWithDifferences{ nullptr };
    QAction * fActionShowMediaWithIssues{ nullptr };

    std::list< QAction * > fEditActions;
    QToolBar * fToolBar{ nullptr };

    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CProgressSystem > fProgressSystem;
};
#endif
