// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software iRHS
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

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../Version.h"
#include "SettingsDlg.h"

#include "Core/ProgressSystem.h"
#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UsersModel.h"

#include "SABUtils/DownloadFile.h"
#include "SABUtils/GitHubGetVersions.h"

#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QTimer>

CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    fSettings = std::make_shared< CSettings >();

    fUsersModel = std::make_shared< CUsersModel >( fSettings );
    connect( this, &CMainWindow::sigSettingsLoaded, fUsersModel.get(), &CUsersModel::slotSettingsChanged );

    fImpl->playStateCompare->setup( fSettings, fUsersModel );

    setupProgressSystem();

    connect( fImpl->playStateCompare, &CPlayStateCompare::sigAddToLog, this, &CMainWindow::slotAddToLog );
    connect( fImpl->playStateCompare, &CPlayStateCompare::sigAddInfoToLog, this, &CMainWindow::slotAddInfoToLog );

    connect( this, &CMainWindow::sigCanceled, fImpl->playStateCompare, &CPlayStateCompare::slotCanceled );
    connect( this, &CMainWindow::sigDataChanged, fImpl->playStateCompare, &CPlayStateCompare::slotDataChanged );
    connect( this, &CMainWindow::sigSettingsChanged, fImpl->playStateCompare, &CPlayStateCompare::slotSettingsChanged );
    connect( this, &CMainWindow::sigSettingsLoaded, fImpl->playStateCompare, &CPlayStateCompare::sigSettingsLoaded );

    connect( this, &CMainWindow::sigDataChanged, this, &CMainWindow::slotUpdateActions );

    connect( fImpl->tabWidget, &QTabWidget::currentChanged, this, &CMainWindow::slotCurentTabChanged );


    connect( fImpl->actionLoadSettings, &QAction::triggered, this, &CMainWindow::slotLoadSettings );
    connect( fImpl->menuLoadRecent, &QMenu::aboutToShow, this, &CMainWindow::slotRecentMenuAboutToShow );

    connect( fImpl->actionSave, &QAction::triggered, this, &CMainWindow::slotSave );
    connect( fImpl->actionSaveAs, &QAction::triggered, this, &CMainWindow::slotSaveAs );
    connect( fImpl->actionSettings, &QAction::triggered, this, &CMainWindow::slotSettings );

    connect( fImpl->actionCheckForLatestVersion, &QAction::triggered, this, &CMainWindow::slotActionCheckForLatest );

    fGitHubVersion.first = new NSABUtils::CGitHubGetVersions( {}, this );
    fGitHubVersion.first->setCurrentVersion( NVersion::MAJOR_VERSION, NVersion::MINOR_VERSION, NVersion::buildDateTime() );
    connect( fGitHubVersion.first, &NSABUtils::CGitHubGetVersions::sigVersionsDownloaded, this, &CMainWindow::slotVersionsDownloaded );
    connect( fGitHubVersion.first, &NSABUtils::CGitHubGetVersions::sigLogMessage, this, &CMainWindow::slotAddInfoToLog );

    slotCurentTabChanged( 0 );
    fImpl->tabWidget->setCurrentIndex( 0 );

    if ( CSettings::loadLastProject() )
        QTimer::singleShot( 0, this, &CMainWindow::slotLoadLastProject );

    if ( CSettings::checkForLatest() )
        QTimer::singleShot( 0, this, &CMainWindow::slotCheckForLatest );
}

void CMainWindow::setupProgressSystem()
{
    auto progressSystem = std::make_shared< CProgressSystem >();
    progressSystem->setSetTitleFunc( [this]( const QString & title )
                                     {
                                         return progressSetup( title );
                                     } );
    progressSystem->setTitleFunc( [this]()
                                  {
                                      if ( fProgressDlg )
                                          return fProgressDlg->labelText();
                                      return QString();
                                  } );
    progressSystem->setMaximumFunc( [this]()
                                    {
                                        if ( fProgressDlg )
                                            return fProgressDlg->maximum();
                                        return 0;
                                    } );
    progressSystem->setSetMaximumFunc( [this]( int count )
                                       {
                                           progressSetMaximum( count );
                                       } );
    progressSystem->setValueFunc( [this]()
                                  {
                                      return progressValue();
                                  } );
    progressSystem->setSetValueFunc( [this]( int value )
                                     {
                                         return progressSetValue( value );
                                     } );
    progressSystem->setIncFunc( [this]()
                                {
                                    return progressIncValue();
                                } );
    progressSystem->setResetFunc( [this]()
                                  {
                                      return progressReset();
                                  } );
    progressSystem->setWasCanceledFunc( [this]()
                                        {
                                            if ( fProgressDlg )
                                                return fProgressDlg->wasCanceled();
                                            return false;
                                        } );

    fImpl->playStateCompare->setProgressSystem( progressSystem );
}

CMainWindow::~CMainWindow()
{
}

void CMainWindow::showEvent( QShowEvent * /*event*/ )
{
}

void CMainWindow::closeEvent( QCloseEvent * event )
{
    bool okToClose = fSettings->maybeSave( this );
        okToClose = okToClose && fImpl->playStateCompare->okToClose();
    if ( !okToClose )
        event->ignore();
    else
        event->accept();
}


void CMainWindow::slotSettings()
{
    CSettingsDlg settings( fSettings, fImpl->playStateCompare->syncSystem(), fImpl->playStateCompare->getAllUsers( true ), this );
    settings.exec();
    if ( fSettings->changed() )
    {
        slotSave();
        loadSettings();
    }
}

void CMainWindow::slotLoadLastProject()
{
    auto recentProjects = fSettings->recentProjectList();
    if ( !recentProjects.isEmpty() )
    {
        for ( int ii = 0; ii < recentProjects.size(); ++ii )
        {
            if ( QFile( recentProjects[ ii ] ).exists() )
            {
                auto project = recentProjects[ ii ];
                QTimer::singleShot( 0, [ this, project ]()
                    {
                        loadFile( project );
                    } );
                break;
            }
        }
    }
}

void CMainWindow::slotCheckForLatest()
{
    checkForLatest( true );
}

void CMainWindow::slotActionCheckForLatest()
{
    checkForLatest( false );
}

void CMainWindow::checkForLatest( bool quiteIfUpToDate )
{
    fGitHubVersion.second = quiteIfUpToDate;
    fGitHubVersion.first->requestLatestVersion();
}

void CMainWindow::slotVersionsDownloaded()
{
    if ( fGitHubVersion.first->hasError() )
    {
        QMessageBox::critical( this, tr( "Error Checking for Update" ), fGitHubVersion.first->errorString() );
        return;
    }

    if ( fGitHubVersion.first->hasUpdate() && fGitHubVersion.first->updateRelease()->getAssetForOS() )
    {
        auto update = fGitHubVersion.first->updateRelease();

        if ( QMessageBox::information( this, tr( "Update Available" ), tr( "There is an update available: '%1'\nWould you like to download it?" ).arg( update->getTitle() ), QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes )
        {
            auto asset = update->getAssetForOS();
            if ( !asset )
                return;

            NSABUtils::CDownloadFile dlg( asset->fName, asset->fUrl.second, asset->fSize, this );
            if ( dlg.startDownload() && ( dlg.exec() == QDialog::Accepted ) && dlg.installAfterDownload() )
            {
                QProcess::startDetached( dlg.getDownloadFile(), {} );
                close();
            }
        }
    }
    else if ( !fGitHubVersion.second )
        QMessageBox::information( this, tr( "Latest Version" ), fGitHubVersion.first->updateVersion() );
}

void CMainWindow::reset()
{
    fImpl->playStateCompare->resetServers();
    fSettings->reset();
    fImpl->log->clear();
}

void CMainWindow::slotRecentMenuAboutToShow()
{
    fImpl->menuLoadRecent->clear();

    auto recentProjects = fSettings->recentProjectList();
    int num = 0;
    for ( auto && ii : recentProjects )
    {
        if ( !QFileInfo( ii ).exists() )
            continue;
        num++;
        auto action = new QAction( QString( "%1%2 - %3" ).arg( ( num < 10 ) ? "&" : "" ).arg( num ).arg( ii ) );
        action->setData( ii );
        action->setStatusTip( tr( "Open %1" ).arg( ii ) );
        connect( action, &QAction::triggered,
            [ = ]()
            {
                auto fileName = action->data().toString();
                loadFile( fileName );
            } );


        fImpl->menuLoadRecent->addAction( action );
    }

    if ( num == 0 )
    {
        fImpl->menuLoadRecent->addAction( fImpl->actionNoRecentFiles );
    }

}

void CMainWindow::loadFile( const QString & fileName )
{
    if ( fileName == fSettings->fileName() )
        return;

    reset();
    if ( fSettings->load( fileName, true, this ) )
    {
        fImpl->playStateCompare->resetServers();
        loadSettings();
    }
}

void CMainWindow::slotLoadSettings()
{
    reset();

    if ( fSettings->load( true, this ) )
        loadSettings();
}

void CMainWindow::slotSave()
{
    fSettings->save( this );
}

void CMainWindow::slotSaveAs()
{
    fSettings->saveAs( this );
}

void CMainWindow::loadSettings()
{
    slotAddToLog( EMsgType::eInfo, "Loading Settings" );

    auto windowTitle = QString::fromStdString( NVersion::getWindowTitle() );
    if ( !fSettings->fileName().isEmpty() )
        windowTitle += " - " + QFileInfo( fSettings->fileName() ).fileName();

    setWindowTitle( windowTitle );

    fImpl->playStateCompare->loadSettings();


    emit sigSettingsChanged();
    emit sigSettingsLoaded();
}

void CMainWindow::slotUpdateActions()
{
}

void CMainWindow::slotAddToLog( int msgType, const QString & msg )
{
    auto fullMsg = createMessage( static_cast<EMsgType>( msgType ), msg );
    qDebug() << fullMsg;
    fImpl->log->appendPlainText( fullMsg );
    statusBar()->showMessage( fullMsg, 500 );
}

void CMainWindow::slotAddInfoToLog( const QString & msg )
{
    slotAddToLog( EMsgType::eInfo, msg );
}

void CMainWindow::progressReset()
{
    if ( fProgressDlg )
        fProgressDlg->deleteLater();
    fProgressDlg = nullptr;
}

void CMainWindow::progressSetup( const QString & title )
{
    if ( !fProgressDlg )
    {
        fProgressDlg = new QProgressDialog( title, tr( "Cancel" ), 0, 0, this );
        connect( fProgressDlg, &QProgressDialog::canceled, this, &CMainWindow::sigCanceled );
    }
    fProgressDlg->setLabelText( title );
    fProgressDlg->setAutoClose( true );
    fProgressDlg->setMinimumDuration( 0 );
    fProgressDlg->setValue( 0 );
    fProgressDlg->open();
}

void CMainWindow::progressSetMaximum( int count )
{
    if ( !fProgressDlg )
        return;
    fProgressDlg->setMaximum( count );
}

int CMainWindow::progressValue() const
{
    if ( !fProgressDlg )
        return 0;
    return fProgressDlg->value();
}

void CMainWindow::progressSetValue( int value )
{
    if ( !fProgressDlg )
        return;
    fProgressDlg->setValue( value );
    fProgressDlg->open();
}

void CMainWindow::progressIncValue()
{
    if ( !fProgressDlg )
        return;

    fProgressDlg->setValue( progressValue() + 1 );
}

void CMainWindow::slotCurentTabChanged( int idx )
{
    for ( auto && ii : fCurrentTabInfo.fMenus )
        menuBar()->removeAction( ii->menuAction() );
    for ( auto && ii : fCurrentTabInfo.fEditActions )
        fImpl->menuEdit->removeAction( ii );
    for ( auto && ii : fCurrentTabInfo.fToolBars )
        removeToolBar( ii );

    if ( idx == 0 )
    {
        fCurrentTabInfo.fMenus = fImpl->playStateCompare->getMenus();
        fCurrentTabInfo.fEditActions = fImpl->playStateCompare->getEditActions();
        fCurrentTabInfo.fToolBars = fImpl->playStateCompare->getToolBars();
    }

    for ( auto && ii : fCurrentTabInfo.fMenus )
        menuBar()->insertAction( fImpl->menuEdit->menuAction(), ii->menuAction() );
    
    auto insertBefore = findEditMenuInsertBefore();
    for ( auto && ii : fCurrentTabInfo.fEditActions )
        fImpl->menuEdit->insertAction( insertBefore, ii );
    for ( auto && ii : fCurrentTabInfo.fToolBars )
        addToolBar( Qt::TopToolBarArea, ii );
}

QAction * CMainWindow::findEditMenuInsertBefore()
{
    if ( !fEditMenuInsertBefore )
    {
        auto editActions = fImpl->menuEdit->actions();
        fEditMenuInsertBefore = nullptr;
        for ( int ii = 1; ii < editActions.count(); ++ii )
        {
            auto prev = editActions.at( ii - 1 );
            auto curr = editActions.at( ii );

            if ( prev->isSeparator() && curr->isSeparator() )
            {
                fEditMenuInsertBefore = curr;
                break;
            }
        }
        Q_ASSERT( fEditMenuInsertBefore );
    }

    return fEditMenuInsertBefore;
}
