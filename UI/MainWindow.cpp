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
#include "SettingsDlg.h"
#include "MediaWindow.h"
#include "MediaTree.h"

#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/MediaData.h"
#include "Core/MediaModel.h"
#include "Core/UsersModel.h"
#include "Core/Settings.h"
#include "Core/ServerInfo.h"
#include "Core/ProgressSystem.h"
#include "SABUtils/QtUtils.h"
#include "SABUtils/GitHubGetVersions.h"
#include "SABUtils/DownloadFile.h"

#include "../Version.h"

#include "ui_MainWindow.h"
#include "SABUtils/WidgetChanged.h"

#include <QTimer>
#include <QScrollBar>
#include <QFileInfo>
#include <QProgressDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QRegularExpression>
#include <QDateTime>
#include <QMetaMethod>
#include <QAbstractItemModelTester>
#include <QInputDialog>
#include <QProcess>
CMainWindow::CMainWindow( QWidget * parent )
    : QMainWindow( parent ),
    fImpl( new Ui::CMainWindow )
{
    fImpl->setupUi( this );
    fSettings = std::make_shared< CSettings >();

    fMediaModel = std::make_shared< CMediaModel >( fSettings );
    fMediaFilterModel = new CMediaFilterModel( fMediaModel.get() );
    fMediaFilterModel->setSourceModel( fMediaModel.get() );
    connect( fMediaModel.get(), &CMediaModel::sigPendingMediaUpdate, this, &CMainWindow::slotPendingMediaUpdate );

    fUsersModel = std::make_shared< CUsersModel >( fSettings );
    fUsersFilterModel = new CUsersFilterModel( fUsersModel.get() );
    fUsersFilterModel->setSourceModel( fUsersModel.get() );
    connect( this, &CMainWindow::sigSettingsLoaded, fUsersModel.get(), &CUsersModel::slotSettingsChanged );

    NSABUtils::setupModelChanged( fUsersModel.get(), this, QMetaMethod::fromSignal( &CMainWindow::sigDataChanged ) );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CMainWindow::sigDataChanged ) );

    connect( this, &CMainWindow::sigDataChanged, this, &CMainWindow::slotDataChanged );

    connect( fImpl->actionViewMediaInformation, &QAction::triggered, this, &CMainWindow::slotViewMediaInfo );
    connect( fImpl->actionRepairUserConnectedIDs, &QAction::triggered, this, &CMainWindow::slotRepairUserConnectedIDs );
    fImpl->users->setModel( fUsersFilterModel );
#ifdef NDEBUG
    fImpl->users->setColumnHidden( CUsersModel::eAllNames, true );
#endif

    fImpl->users->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->users, &QTreeView::customContextMenuRequested, this, &CMainWindow::slotUsersContextMenu );


    fMediaFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );
    fUsersFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );

    fSyncSystem = std::make_shared< CSyncSystem >( fSettings, fUsersModel, fMediaModel, this );
    connect( fSyncSystem.get(), &CSyncSystem::sigAddToLog, this, &CMainWindow::slotAddToLog );
    connect( fSyncSystem.get(), &CSyncSystem::sigLoadingUsersFinished, this, &CMainWindow::slotLoadingUsersFinished );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainWindow::slotUserMediaLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMainWindow::slotUserMediaCompletelyLoaded );

    fSyncSystem->setUserMsgFunc(
        [ this ]( const QString & title, const QString & msg, bool isCritical )
        {
            if ( isCritical )
                QMessageBox::critical( this, title, msg );
            else
                QMessageBox::information( this, title, msg );
        } );
    auto progressSystem = std::make_shared< CProgressSystem >();
    progressSystem->setSetTitleFunc( [ this ]( const QString & title )
        {
            return progressSetup( title );
        } );
    progressSystem->setTitleFunc( [ this ]()
        {
            if ( fProgressDlg )
                return fProgressDlg->labelText();
            return QString();
        } );
    progressSystem->setMaximumFunc( [ this ]()
        {
            if ( fProgressDlg )
                return fProgressDlg->maximum();
            return 0;
        } );
    progressSystem->setSetMaximumFunc( [ this ]( int count )
        {
            progressSetMaximum( count );
        } );
    progressSystem->setValueFunc( [ this ]()
        {
            return progressValue();
        } );
    progressSystem->setSetValueFunc( [ this ]( int value )
        {
            return progressSetValue( value );
        } );
    progressSystem->setIncFunc( [ this ]()
        {
            return progressIncValue();
        } );
    progressSystem->setResetFunc( [ this ]()
        {
            return progressReset();
        } );
    progressSystem->setWasCanceledFunc( [ this ]()
        {
            if ( fProgressDlg )
                return fProgressDlg->wasCanceled();
            return false;
        } );

    fSyncSystem->setProgressSystem( progressSystem );

    connect( fImpl->actionLoadSettings, &QAction::triggered, this, &CMainWindow::slotLoadSettings );
    connect( fImpl->menuLoadRecent, &QMenu::aboutToShow, this, &CMainWindow::slotRecentMenuAboutToShow );
    connect( fImpl->actionReloadServers, &QAction::triggered, this, &CMainWindow::slotReloadServers );
    connect( fImpl->actionReloadCurrentUser, &QAction::triggered, this, &CMainWindow::slotReloadCurrentUser );

    connect( fImpl->actionOnlyShowSyncableUsers, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowSyncableUsers );
    connect( fImpl->actionOnlyShowMediaWithDifferences, &QAction::triggered, this, &CMainWindow::slotToggleOnlyShowMediaWithDifferences );
    connect( fImpl->actionShowMediaWithIssues, &QAction::triggered, this, &CMainWindow::slotToggleShowMediaWithIssues );

    connect( fImpl->actionSave, &QAction::triggered, this, &CMainWindow::slotSave );
    connect( fImpl->actionSaveAs, &QAction::triggered, this, &CMainWindow::slotSaveAs );
    connect( fImpl->actionSettings, &QAction::triggered, this, &CMainWindow::slotSettings );

    connect( fImpl->users, &QTreeView::clicked, this, &CMainWindow::slotCurrentUserChanged );

    connect( fImpl->actionProcess, &QAction::triggered, fSyncSystem.get(), &CSyncSystem::slotProcess );
    connect( fImpl->actionSelectiveProcess, &QAction::triggered, this, &CMainWindow::slotSelectiveProcess );

    connect( fImpl->actionCheckForLatestVersion, &QAction::triggered, this, &CMainWindow::slotActionCheckForLatest );
    slotSetCurrentMediaItem( QModelIndex() );

    fGitHubVersion.first = new NSABUtils::CGitHubGetVersions( {}, this );
    fGitHubVersion.first->setCurrentVersion( NVersion::MAJOR_VERSION, NVersion::MINOR_VERSION, NVersion::buildDateTime() );
    connect( fGitHubVersion.first, &NSABUtils::CGitHubGetVersions::sigVersionsDownloaded, this, &CMainWindow::slotVersionsDownloaded );
    connect( fGitHubVersion.first, &NSABUtils::CGitHubGetVersions::sigLogMessage, this, &CMainWindow::slotAddInfoToLog );

    if ( CSettings::loadLastProject() )
        QTimer::singleShot( 0, this, &CMainWindow::slotLoadLastProject );

    if ( CSettings::checkForLatest() )
        QTimer::singleShot( 0, this, &CMainWindow::slotCheckForLatest );
}

CMainWindow::~CMainWindow()
{
    if ( fMediaWindow )
        delete fMediaWindow.data();
}

void CMainWindow::showEvent( QShowEvent * /*event*/ )
{
}

void CMainWindow::closeEvent( QCloseEvent * event )
{
    bool okToClose = fSettings->maybeSave( this );
    if ( okToClose && fMediaWindow )
        okToClose = fMediaWindow->okToClose();
    if ( !okToClose )
        event->ignore();
    else
    {
        event->accept();
        if ( fMediaWindow )
            fMediaWindow->close();
        fMediaWindow.data();
    }
}


void CMainWindow::slotSettings()
{
    CSettingsDlg settings( fSettings, fSyncSystem, fUsersModel->getAllUsers( true ), this );
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
    resetServers();

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
        resetServers();
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

    loadServers();
    fImpl->actionOnlyShowSyncableUsers->setChecked( fSettings->onlyShowSyncableUsers() );
    fImpl->actionOnlyShowMediaWithDifferences->setChecked( fSettings->onlyShowMediaWithDifferences() );
    fImpl->actionShowMediaWithIssues->setChecked( fSettings->showMediaWithIssues() );

    fUsersModel->clear();
    fSyncSystem->loadUsers();
    emit sigSettingsLoaded();
}

void CMainWindow::slotDataChanged()
{
    bool hasCurrentUser = fImpl->users->selectionModel()->currentIndex().isValid();
    bool canSync = fSettings->canAnyServerSync();
    bool hasDataToProcess = canSync && fMediaModel->hasMediaToProcess();
    bool hasUsersNeedingFixing = fUsersModel->hasUsersWithConnectedIDNeedingUpdate();

    fImpl->actionReloadServers->setEnabled( canSync );
    fImpl->actionReloadCurrentUser->setEnabled( canSync && hasCurrentUser );

    fImpl->actionProcess->setEnabled( hasDataToProcess );
    fImpl->actionSelectiveProcess->setEnabled( hasDataToProcess );
    fImpl->actionRepairUserConnectedIDs->setEnabled( hasUsersNeedingFixing );
}

void CMainWindow::slotLoadingUsersFinished()
{
    onlyShowSyncableUsers();
    fUsersFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    NSABUtils::autoSize( fImpl->users, -1 );
}

void CMainWindow::slotReloadServers()
{
    resetServers();
    fSyncSystem->loadServers();
    fSyncSystem->loadUsers();
}

void CMainWindow::slotReloadCurrentUser()
{
    fSyncSystem->clearCurrUser();
    auto currIdx = fImpl->users->selectionModel()->currentIndex();
    slotCurrentUserChanged( currIdx );
}

void CMainWindow::slotCurrentUserChanged( const QModelIndex & index )
{
    fImpl->actionReloadCurrentUser->setEnabled( index.isValid() );

    if ( fSyncSystem->isRunning() )
        return;

    auto prevUserData = fSyncSystem->currUser();

    auto idx = index;
    if ( !idx.isValid() )
        idx = fImpl->users->selectionModel()->currentIndex();

    if ( !index.isValid() )
        return;

    auto userData = getUserData( idx );
    if ( !userData )
        return;

    if ( fSyncSystem->currUser() == userData )
        return;

    fMediaModel->clear();

    fSyncSystem->loadUsersMedia( userData );
}


std::shared_ptr< CUserData > CMainWindow::getCurrUserData() const
{
    auto idx = fImpl->users->selectionModel()->currentIndex();
    if ( !idx.isValid() )
        return {};

    return getUserData( idx );
}

std::shared_ptr< CUserData > CMainWindow::getUserData( QModelIndex idx ) const
{
    if ( idx.model() != fUsersModel.get() )
        idx = fUsersFilterModel->mapToSource( idx );

    auto retVal = fUsersModel->userData( idx );
    return retVal;
}

void CMainWindow::slotToggleOnlyShowSyncableUsers()
{
    fSettings->setOnlyShowSyncableUsers( fImpl->actionOnlyShowSyncableUsers->isChecked() );
    onlyShowSyncableUsers();
}

void CMainWindow::onlyShowSyncableUsers()
{
    auto usersSummary = fUsersModel->settingsChanged();
    fImpl->usersLabel->setText( tr( "Users: %1 sync-able out of %2 total users" ).arg( usersSummary.fSyncable ).arg( usersSummary.fTotal ) );
}

void CMainWindow::slotToggleOnlyShowMediaWithDifferences()
{
    fSettings->setOnlyShowMediaWithDifferences( fImpl->actionOnlyShowMediaWithDifferences->isChecked() );
    onlyShowMediaWithDifferences();
}

void CMainWindow::slotToggleShowMediaWithIssues()
{
    fSettings->setShowMediaWithIssues( fImpl->actionShowMediaWithIssues->isChecked() );
    showMediaWithIssues();
}

void CMainWindow::onlyShowMediaWithDifferences()
{
    fMediaModel->settingsChanged();

    progressReset();

    auto mediaSummary = SMediaSummary( fMediaModel );
    fImpl->mediaSummaryLabel->setText( mediaSummary.getSummaryText() );

    auto column = fMediaFilterModel->sortColumn();
    auto order = fMediaFilterModel->sortOrder();
    if ( column == -1 )
    {
        column = 0;
        order = Qt::AscendingOrder;
    }
    fMediaFilterModel->sort( column, order );

    for ( auto && ii : fMediaTrees )
        ii->autoSize();
}

void CMainWindow::showMediaWithIssues()
{
    fMediaModel->settingsChanged();
    progressReset();

    auto mediaSummary = SMediaSummary( fMediaModel );
    fImpl->mediaSummaryLabel->setText( mediaSummary.getSummaryText() );
}

void CMainWindow::slotAddToLog( int msgType, const QString & msg )
{
    auto fullMsg = createMessage( static_cast<EMsgType>( msgType ), msg );
    qDebug() << fullMsg;
    fImpl->log->appendPlainText( fullMsg );
    fImpl->statusbar->showMessage( fullMsg, 500 );
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
        connect( fProgressDlg, &QProgressDialog::canceled, fSyncSystem.get(), &CSyncSystem::slotCanceled );
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

std::shared_ptr< CMediaData > CMainWindow::getMediaData( QModelIndex idx ) const
{
    if ( idx.model() != fMediaModel.get() )
    {
        idx = fMediaFilterModel->mapToSource( idx );
    }

    auto mediaData = fMediaModel->getMediaData( idx );
    return mediaData;
}

void CMainWindow::slotPendingMediaUpdate()
{
    if ( !fPendingMediaUpdateTimer )
    {
        fPendingMediaUpdateTimer = new QTimer( this );
        fPendingMediaUpdateTimer->setSingleShot( true );
        fPendingMediaUpdateTimer->setInterval( 2500 );
        connect( fPendingMediaUpdateTimer, &QTimer::timeout,
            [ this ]()
            {
                onlyShowMediaWithDifferences();
                delete fPendingMediaUpdateTimer;
                fPendingMediaUpdateTimer = nullptr;
            } );
    }
    fPendingMediaUpdateTimer->stop();
    fPendingMediaUpdateTimer->start();
}


void CMainWindow::resetServers()
{
    fUsersModel->clear();
    fMediaModel->clear();
    fSyncSystem->reset();
}

void CMainWindow::loadServers()
{
    for ( auto && ii : fMediaTrees )
        delete ii;
    fMediaTrees.clear();

    fSyncSystem->loadServers();

    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = fSettings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;

        auto mediaTree = new CMediaTree( fSettings->serverInfo( ii ), fImpl->upperSplitter );
        fImpl->upperSplitter->addWidget( mediaTree );
        mediaTree->setModel( fMediaFilterModel );
        connect( mediaTree, &CMediaTree::sigCurrChanged, this, &CMainWindow::slotSetCurrentMediaItem );
        connect( mediaTree, &CMediaTree::sigViewMedia, this, &CMainWindow::slotViewMedia );
        fMediaTrees.push_back( mediaTree );
    }

    for ( size_t ii = 0; ii < fMediaTrees.size(); ++ii )
    {
        for ( size_t jj = 0; jj < fMediaTrees.size(); ++jj )
        {
            if ( ii == jj )
                continue;
            fMediaTrees[ ii ]->addPeerMediaTree( fMediaTrees[ jj ] );
        }
    }
}

void CMainWindow::slotViewMedia( const QModelIndex & current )
{
    slotViewMediaInfo();
    slotSetCurrentMediaItem( current );
}

void CMainWindow::slotSetCurrentMediaItem( const QModelIndex & current )
{
    auto mediaInfo = getMediaData( current );

    if ( fMediaWindow )
        fMediaWindow->setMedia( mediaInfo );

    fImpl->actionViewMediaInformation->setEnabled( mediaInfo.get() != nullptr );

    slotDataChanged();
}

void CMainWindow::slotViewMediaInfo()
{
    if ( !fMediaWindow )
        fMediaWindow = new CMediaWindow( fSettings, fSyncSystem, nullptr );

    for ( auto && ii : fMediaTrees )
    {
        auto currIdx = ii->currentIndex();
        if ( currIdx.isValid() )
        {
            auto mediaInfo = getMediaData( currIdx );
            if ( mediaInfo )
            {
                fMediaWindow->setMedia( mediaInfo );
                break;
            }
        }
    }
    fMediaWindow->show();
    fMediaWindow->activateWindow();
    fMediaWindow->raise();
}

void CMainWindow::slotUserMediaLoaded()
{
    auto currUser = getCurrUserData();
    if ( !currUser )
        return;

    for ( auto && ii : fMediaTrees )
        ii->hideColumns();
}


void CMainWindow::slotUserMediaCompletelyLoaded()
{
    if ( !fMediaLoadedTimer )
    {
        fMediaLoadedTimer = new QTimer( this );
        fMediaLoadedTimer->setSingleShot( true );
        fMediaLoadedTimer->setInterval( 500 );
        connect( fMediaLoadedTimer, &QTimer::timeout,
                 [this]()
                 {
                     onlyShowMediaWithDifferences();

                     delete fMediaLoadedTimer;
                     fMediaLoadedTimer = nullptr;
                 } );
    }
    fMediaLoadedTimer->stop();
    fMediaLoadedTimer->start();
}

void CMainWindow::slotSelectiveProcess()
{
    QStringList serverNames;
    std::map< QString, QString > servers;
    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = fSettings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;

        auto name = serverInfo->displayName();
        auto key = serverInfo->keyName();

        servers[ name ] = key;
        serverNames << name;
    }

    if ( serverNames.isEmpty() )
        return;

    auto whichServer = QInputDialog::getItem( this, tr( "Select Source Server" ), tr( "Source Server:" ), serverNames, 0, false );
    if ( whichServer.isEmpty() )
        return;
    auto pos = servers.find( whichServer );
    if ( pos == servers.end() )
        return;
    fSyncSystem->selectiveProcess( (*pos).second );
}

void CMainWindow::slotRepairUserConnectedIDs()
{
    auto users = fUsersModel->usersWithConnectedIDNeedingUpdate();
    fSyncSystem->repairConnectIDs( users );
}

void CMainWindow::slotSetConnectID()
{
    auto currIdx = fImpl->users->currentIndex();
    if ( !currIdx.isValid() )
        return;

    auto userData = getUserData( currIdx );
    if ( !userData )
        return;
    
    auto newConnectedID = QInputDialog::getText( this, tr( "Enter Connect ID" ), tr( "Connect ID:" ), QLineEdit::Normal, userData->connectedID() );
    if ( newConnectedID.isEmpty() || ( newConnectedID == userData->connectedID() ) )
        return;

    fSyncSystem->setConnectedID( newConnectedID, userData );
 }

void CMainWindow::slotAutoSetConnectID()
{
    auto currIdx = fImpl->users->currentIndex();
    if ( !currIdx.isValid() )
        return;

    auto userData = getUserData( currIdx );
    if ( !userData )
        return;

    fSyncSystem->repairConnectIDs( { userData } );
}

void CMainWindow::slotUsersContextMenu( const QPoint & pos )
{
    auto idx = fImpl->users->indexAt( pos );
    if ( !idx.isValid() )
        return;

    auto userData = getUserData( idx );
    if ( !userData )
        return;

    QMenu menu( tr( "Context Menu" ) );

    QAction action( "Set Connect ID" );
    menu.addAction( &action );
    connect( &action, &QAction::triggered, this, &CMainWindow::slotSetConnectID );

    QAction action2( "AutoSet Connect ID" );
    menu.addAction( &action2 );
    action2.setEnabled( userData->connectedIDNeedsUpdate() );
    connect( &action2, &QAction::triggered, this, &CMainWindow::slotAutoSetConnectID );

    menu.exec( fImpl->users->mapToGlobal( pos ) );
}
