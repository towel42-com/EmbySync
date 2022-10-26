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
#include <unordered_map>
#include <memory>
namespace Ui
{
    class CMainWindow;
}
namespace NSABUtils{ class CGitHubGetVersions; }
class CSettings;
class QProgressDialog;
class CProgressSystem;
class CUsersModel;
class CSyncSystem;
class CUsersFilterModel;
class CTabPageBase;
class CMediaModel;
class CTabUIInfo;

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget * parent = nullptr );

    virtual ~CMainWindow() override;

    virtual void closeEvent( QCloseEvent * closeEvent ) override;

    virtual void showEvent( QShowEvent * event ) override;
Q_SIGNALS:
    void sigSettingsChanged();
    void sigSettingsLoaded();
    void sigModelDataChanged();
    void sigCanceled();
public Q_SLOTS:
    void slotSettings();

    void loadSettings();

    void loadSettingsIntoPages();

    void slotUpdateActions();

    void slotLoadSettings();
    void slotSave();
    void slotSaveAs();
    void slotRecentMenuAboutToShow();

    void slotLoadLastProject();
    void slotCheckForLatest();
    void slotActionCheckForLatest();
    void slotSettingsChanged();
    void slotLoadingUsersFinished();
    void slotReloadServers();

private Q_SLOTS:
    void slotAddToLog( int msgType, const QString & msg );
    void slotAddInfoToLog( const QString & msg );
    void slotVersionsDownloaded();
    void slotCurentTabChanged( int idx );

private:
    bool okToClosePages();
    bool prepForClosePages();


    CTabPageBase * getCurrentPage() const;
    void setupPage( int pageIndex );

    void setupProgressSystem();
    void checkForLatest( bool quiteIfUpToDate );


    void progressSetup( const QString & title );

    void progressSetMaximum( int count );
    int progressValue() const;
    void progressSetValue( int value );
    void progressIncValue();
    void progressReset();

    void loadFile( const QString & fileName );

    void reset();

    void resetPages();

    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::shared_ptr< CSettings > fSettings;
    std::shared_ptr< CSyncSystem > fSyncSystem;
    std::shared_ptr< CUsersModel > fUsersModel;
    std::shared_ptr< CMediaModel > fMediaModel;
    std::shared_ptr< CProgressSystem > fProgressSystem;


    QProgressDialog * fProgressDlg{ nullptr };

    std::unordered_map< int, CTabPageBase * > fPages;
    std::shared_ptr< CTabUIInfo > fCurrentTabUIInfo;

    std::pair< NSABUtils::CGitHubGetVersions *, bool > fGitHubVersion{ nullptr, false };
};
#endif
