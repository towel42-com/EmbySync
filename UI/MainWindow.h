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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <tuple>
#include <optional>
#include <QPointer>
namespace Ui
{
    class CMainWindow;
}
class CMediaData;
class CUserData;
class CSettings;
class CSyncSystem;
class QProgressDialog;
class CMediaModel;
class CMediaFilterModel;
class CUsersModel;
class CUsersFilterModel;
class QTreeView;


class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum class EWhichTree
    {
        eLHS,
        eRHS
    };
    CMainWindow( QWidget * parent = nullptr );

    virtual ~CMainWindow() override;

    virtual void closeEvent( QCloseEvent * closeEvent ) override;
    virtual void showEvent( QShowEvent * event ) override;
    virtual bool eventFilter( QObject * obj, QEvent * event ) override;
Q_SIGNALS:
    void sigSettingsLoaded();
    void sigDataChanged();
public Q_SLOTS:
    void slotSettings();

    void loadSettings();

    void slotDataChanged();

    void slotLoadSettings();
    void slotSave();
    void slotRecentMenuAboutToShow();
    void slotUserMediaCompletelyLoaded();
    void slotPendingMediaUpdate();
private Q_SLOTS:
    void slotCurrentUserChanged( const QModelIndex & index );

    void slotReloadServers();
    void slotReloadCurrentUser();
    
    void slotToggleOnlyShowSyncableUsers();
    void slotToggleOnlyShowMediaWithDifferences();
    void slotToggleShowMediaWithIssues();

    void slotAddToLog( int msgType, const QString & msg );

    void slotLoadingUsersFinished();
    void slotUserMediaLoaded();

    void slotSetCurrentMediaItem( const QModelIndex & current, const QModelIndex & previous );

    void slotApplyToLeft();
    void slotApplyToRight();
    void slotUploadUserMediaData();
private:
    void hideColumns( QTreeView * treeView, EWhichTree whichTree );

    std::shared_ptr< CMediaData > getMediaData( QModelIndex idx ) const;
        
    void progressSetup( const QString & title );

    void progressSetMaximum( int count );
    int progressValue() const;
    void progressSetValue( int value );
    void progressIncValue();
    void progressReset();

    void onlyShowSyncableUsers();
    void onlyShowMediaWithDifferences();
    void showMediaWithIssues();
    std::shared_ptr< CUserData > getCurrUserData() const;
    std::shared_ptr< CUserData > getUserData( const QModelIndex & idx ) const;

    void loadFile( const QString & fileName );

    void reset();

    void resetServers();

    void loadAllMedia();
    void loadAllMedia( bool isLHS );


    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CSyncSystem > fSyncSystem;

    QProgressDialog * fProgressDlg{ nullptr };

    CMediaModel * fMediaModel{ nullptr };
    CMediaFilterModel * fMediaFilterModel{ nullptr };

    CUsersModel * fUsersModel{ nullptr };
    CUsersFilterModel * fUsersFilterModel{ nullptr };

    QTimer * fPendingMediaUpdateTimer{ nullptr };
    QTimer * fMediaLoadedTimer{ nullptr };

};
#endif 
