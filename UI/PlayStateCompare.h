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

#include "TabPageBase.h"
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
class CMediaFilterModel;
class CUsersModel;
class CUsersFilterModel;
class CMediaWindow;
class CDataTree;
class CProgressSystem;
class CTabUIInfo;

class CPlayStateCompare : public CTabPageBase
{
    Q_OBJECT
public:
    CPlayStateCompare( QWidget * parent = nullptr );
    virtual ~CPlayStateCompare() override;

    virtual void setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CProgressSystem > progressSystem ) override;
    virtual void setupActions();

    virtual bool okToClose() override;

    virtual void loadSettings() override;
    virtual void resetPage() override;

    virtual std::shared_ptr< CTabUIInfo > getUIInfo() const override;

    virtual void loadingUsersFinished() override;

    virtual QSplitter * getDataSplitter() const override;

    void onlyShowSyncableUsers();
    void onlyShowMediaWithDifferences();
    void showMediaWithIssues();

    virtual bool prepForClose() override;
Q_SIGNALS:
    void sigModelDataChanged();

public Q_SLOTS:
    virtual void slotCanceled() override;
    virtual void slotModelDataChanged() override;
    virtual void slotSettingsChanged() override;

private Q_SLOTS:
    void slotReloadCurrentUser();

    void slotProcess();

    void slotSelectiveProcess();

    void slotUserMediaCompletelyLoaded();
    void slotPendingMediaUpdate();

    void slotCurrentUserChanged( const QModelIndex & index );

    void slotToggleOnlyShowSyncableUsers();
    void slotToggleOnlyShowMediaWithDifferences();
    void slotToggleShowMediaWithIssues();


    void slotUserMediaLoaded();

    void slotSetCurrentMediaItem( const QModelIndex & current );
    void slotViewMedia( const QModelIndex & current );

    void slotViewMediaInfo();
private:
    std::shared_ptr< CMediaData > getMediaData( QModelIndex idx ) const;

    std::shared_ptr< CUserData > getCurrUserData() const;
    std::shared_ptr< CUserData > getUserData( QModelIndex idx ) const;

    void reset();

    void loadServers();

    std::unique_ptr< Ui::CPlayStateCompare > fImpl;

    CUsersFilterModel * fUsersFilterModel{ nullptr };
    CMediaFilterModel * fMediaFilterModel{ nullptr };

    QTimer * fPendingMediaUpdateTimer{ nullptr };
    QTimer * fMediaLoadedTimer{ nullptr };

    QPointer< CMediaWindow > fMediaWindow;

    QPointer< QAction > fActionReloadCurrentUser{ nullptr };

    QPointer< QMenu > fProcessMenu{ nullptr };
    QPointer< QAction > fActionProcess{ nullptr };
    QPointer< QAction > fActionSelectiveProcess{ nullptr };

    QPointer< QMenu > fViewMenu{ nullptr };
    QPointer< QAction > fActionViewMediaInformation{ nullptr };

    QPointer< QAction > fActionOnlyShowSyncableUsers{ nullptr };
    QPointer< QAction > fActionOnlyShowMediaWithDifferences{ nullptr };
    QPointer< QAction > fActionShowMediaWithIssues{ nullptr };

    std::list< QPointer< QAction > > fEditActions;
    QPointer< QToolBar > fToolBar{ nullptr };
};
#endif
