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
#include "TabUIInfo.h"

#include "Core/ProgressSystem.h"
#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UsersModel.h"
#include "Core/MediaModel.h"
#include "Core/CollectionsModel.h"
#include "Core/ServerModel.h"

#include "SABUtils/DownloadFile.h"
#include "SABUtils/GitHubGetVersions.h"
#include "SABUtils/WidgetChanged.h"

#include <QDebug>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QSettings>
#include <QTimer>
#include <QMetaMethod>

CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );

    fServerModel = std::make_shared< CServerModel >();
    connect( fServerModel.get(), &CServerModel::sigServersLoaded, this, &CMainWindow::slotServersLoaded );
    fSettings = std::make_shared< CSettings >( fServerModel );

    fUsersModel = std::make_shared< CUsersModel >( fSettings, fServerModel );
    NSABUtils::setupModelChanged( fUsersModel.get(), this, QMetaMethod::fromSignal( &CMainWindow::sigModelDataChanged ) );
    connect( this, &CMainWindow::sigSettingsLoaded, fUsersModel.get(), &CUsersModel::slotSettingsChanged );
    connect( this, &CMainWindow::sigSettingsLoaded, this, &CMainWindow::slotSettingsChanged );
    connect( this, &CMainWindow::sigSettingsChanged, this, &CMainWindow::slotSettingsChanged );

    fMediaModel = std::make_shared< CMediaModel >( fSettings, fServerModel );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CMainWindow::sigModelDataChanged ) );

    fCollectionsModel = std::make_shared< CCollectionsModel >( fMediaModel );


    fSyncSystem = std::make_shared< CSyncSystem >( fSettings, fUsersModel, fMediaModel, fCollectionsModel, fServerModel );
    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMainWindow::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigAddInfoToLog, this, &CMainWindow::slotAddInfoToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMainWindow::slotLoadingUsersFinished );

    setupProgressSystem();

    connect( fImpl->actionReloadServers, &QAction::triggered, this, &CMainWindow::slotReloadServers );

    for( int ii = 0; ii < fImpl->tabWidget->count(); ++ii )
        setupPage( ii );

    connect( this, &CMainWindow::sigModelDataChanged, this, &CMainWindow::slotUpdateActions );
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

    auto idx = fImpl->tabWidget->currentIndex();

    QSettings settings;
    auto lastPage = settings.value( "LastPage", 0 ).toInt();
    if ( idx != lastPage )
        fImpl->tabWidget->setCurrentIndex( lastPage );
    else
        slotCurentTabChanged( lastPage );

    if ( CSettings::loadLastProject() )
        QTimer::singleShot( 0, this, &CMainWindow::slotLoadLastProject );

    if ( CSettings::checkForLatest() )
        QTimer::singleShot( 0, this, &CMainWindow::slotCheckForLatest );
}

void CMainWindow::setupProgressSystem()
{
    fProgressSystem = std::make_shared< CProgressSystem >();
    fProgressSystem->setSetTitleFunc( [this]( const QString & title )
                                     {
                                         return progressSetup( title );
                                     } );
    fProgressSystem->setTitleFunc( [this]()
                                  {
                                      if ( fProgressDlg )
                                          return fProgressDlg->labelText();
                                      return QString();
                                  } );
    fProgressSystem->setMaximumFunc( [this]()
                                    {
                                        if ( fProgressDlg )
                                            return fProgressDlg->maximum();
                                        return 0;
                                    } );
    fProgressSystem->setSetMaximumFunc( [this]( int count )
                                       {
                                           progressSetMaximum( count );
                                       } );
    fProgressSystem->setValueFunc( [this]()
                                  {
                                      return progressValue();
                                  } );
    fProgressSystem->setSetValueFunc( [this]( int value )
                                     {
                                         return progressSetValue( value );
                                     } );
    fProgressSystem->setIncFunc( [this]()
                                {
                                    return progressIncValue();
                                } );
    fProgressSystem->setResetFunc( [this]()
                                  {
                                      return progressReset();
                                  } );
    fProgressSystem->setWasCanceledFunc( [this]()
                                        {
                                            if ( fProgressDlg )
                                                return fProgressDlg->wasCanceled();
                                            return false;
                                        } );

    fSyncSystem->setProgressSystem( fProgressSystem );

    fSyncSystem->setUserMsgFunc(
        [this]( EMsgType msgType, const QString & title, const QString & msg )
        {
            if ( msgType == EMsgType::eError )
                QMessageBox::critical( this, title, msg );
            else if ( msgType == EMsgType::eWarning )
                QMessageBox::warning( this, title, msg );
            else
                QMessageBox::information( this, title, msg );
        } );
}

CMainWindow::~CMainWindow()
{
    QSettings settings;
    settings.setValue( "LastPage", fImpl->tabWidget->currentIndex() );
    fCurrentTabUIInfo = nullptr;
    disconnect( fImpl->tabWidget, &QTabWidget::currentChanged, this, &CMainWindow::slotCurentTabChanged );
}

void CMainWindow::showEvent( QShowEvent * /*event*/ )
{
}

void CMainWindow::closeEvent( QCloseEvent * event )
{
    bool okToClose = fSettings->maybeSave( this );
    okToClose = okToClose && okToClosePages();
    if ( !okToClose )
        event->ignore();
    else
    {
        if ( !prepForClosePages() )
        {
            event->ignore();
            return;
        }
        event->accept();
    }
}

void CMainWindow::slotSettingsChanged()
{
    fUsersModel->clear();
    fSyncSystem->loadUsers();
}

void CMainWindow::slotReloadServers()
{
    resetPages();
    fSyncSystem->loadServerInfo();
    fSyncSystem->loadUsers();
}

void CMainWindow::slotSettings()
{
    CSettingsDlg settings( fSettings, fServerModel, fSyncSystem, this );
    settings.setKnownUsers( fUsersModel->getAllUsers( true ) );
    settings.setKnownShows( fMediaModel->getKnownShows() );
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
    resetPages();

    fUsersModel->clear();
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
        resetPages();
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

    auto windowTitle = NVersion::getWindowTitle();
    if ( !fSettings->fileName().isEmpty() )
        windowTitle += " - " + QFileInfo( fSettings->fileName() ).fileName();

    setWindowTitle( windowTitle );

    loadSettingsIntoPages();


    emit sigSettingsChanged();
    emit sigSettingsLoaded();
}

void CMainWindow::slotUpdateActions()
{
    bool canSync = fServerModel->canAnyServerSync();
    fImpl->actionReloadServers->setEnabled( canSync );
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
    {
        //fProgressDlg->close();
        fProgressDlg->reset();
    }
}

void CMainWindow::progressSetup( const QString & title )
{
    if ( !fProgressDlg )
    {
        fProgressDlg = new QProgressDialog( title, tr( "Cancel" ), 0, 0, this );
        connect( fProgressDlg, &QProgressDialog::canceled, this, &CMainWindow::sigCanceled );
    }
    fProgressDlg->setLabelText( title );
    if ( fProgressDlg )
        fProgressDlg->setAutoClose( true );
    if ( fProgressDlg )
        fProgressDlg->setMinimumDuration( 0 );
    if ( fProgressDlg )
        fProgressDlg->setValue( 0 );
    if ( fProgressDlg )
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
    if ( fProgressDlg )
        fProgressDlg->open();
}

void CMainWindow::progressIncValue()
{
    if ( !fProgressDlg )
        return;

    fProgressDlg->setValue( progressValue() + 1 );
    qApp->processEvents();
}

void CMainWindow::slotCurentTabChanged( int /*idx*/ )
{
    if ( fCurrentTabUIInfo )
        fCurrentTabUIInfo->cleanupUI( this );

    auto currPage = getCurrentPage();
    if ( currPage )
        fCurrentTabUIInfo = currPage->getUIInfo();
    else
        fCurrentTabUIInfo.reset();

    if ( !fCurrentTabUIInfo )
        return;

    fCurrentTabUIInfo->setupUI( this, fImpl->menuEdit );
}

void CMainWindow::slotServersLoaded()
{
    fSyncSystem->loadServerInfo();
}

CTabPageBase * CMainWindow::getCurrentPage() const
{
    auto currIndex = fImpl->tabWidget->currentIndex();
    auto pos = fPages.find( currIndex );
    if ( pos == fPages.end() )
        return nullptr;
    return ( *pos ).second;
}

void CMainWindow::setupPage( int index )
{
    auto page = dynamic_cast< CTabPageBase * >( fImpl->tabWidget->widget( index ) );
    if ( !page )
        return;


    fPages[ index ] = page;
    
    page->setupPage( fSettings, fSyncSystem, fMediaModel, fCollectionsModel, fUsersModel, fServerModel, fProgressSystem );
    connect( page, &CTabPageBase::sigAddToLog, this, &CMainWindow::slotAddToLog );
    connect( page, &CTabPageBase::sigAddInfoToLog, this, &CMainWindow::slotAddInfoToLog );

    connect( this, &CMainWindow::sigCanceled, page, &CTabPageBase::slotCanceled );
    connect( this, &CMainWindow::sigModelDataChanged, page, &CTabPageBase::slotModelDataChanged );
    connect( this, &CMainWindow::sigSettingsChanged, page, &CTabPageBase::slotSettingsChanged );
    connect( this, &CMainWindow::sigSettingsLoaded, page, &CTabPageBase::sigSettingsLoaded );
}

void CMainWindow::resetPages()
{
    fUsersModel->clear();
    fMediaModel->clear();
    fSyncSystem->reset();

    for ( auto && ii : fPages )
        ii.second->resetPage();
}

void CMainWindow::loadSettingsIntoPages()
{
    for ( auto && ii : fPages )
        ii.second->loadSettings();
}


bool CMainWindow::okToClosePages()
{
    bool aOK = true;
    for ( auto && ii : fPages )
        aOK = aOK && ii.second->okToClose();
    return aOK;
}

bool CMainWindow::prepForClosePages()
{
    bool aOK = true;
    for ( auto && ii : fPages )
        aOK = aOK && ii.second->prepForClose();
    return aOK;
}

void CMainWindow::slotLoadingUsersFinished()
{
    fUsersModel->loadAvatars( fSyncSystem );
    for ( auto && ii : fPages )
        ii.second->loadingUsersFinished();
}
