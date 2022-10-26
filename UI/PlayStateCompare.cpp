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

#include "PlayStateCompare.h"
#include "ui_PlayStateCompare.h"

#include "DataTree.h"
#include "MediaWindow.h"
#include "TabUIInfo.h"

#include "Core/MediaModel.h"
#include "Core/ProgressSystem.h"
#include "Core/ServerInfo.h"
#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/UsersModel.h"

#include "SABUtils/AutoWaitCursor.h"
#include "SABUtils/QtUtils.h"
#include "SABUtils/WidgetChanged.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QCloseEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QTimer>
#include <QToolBar>

CPlayStateCompare::CPlayStateCompare( QWidget * parent )
    : CTabPageBase( parent ),
    fImpl( new Ui::CPlayStateCompare )
{
    fImpl->setupUi( this );
    setupActions();

    connect( this, &CPlayStateCompare::sigModelDataChanged, this, &CPlayStateCompare::slotModelDataChanged );

    connect( this, &CTabPageBase::sigSetCurrentDataItem, this, &CPlayStateCompare::slotSetCurrentMediaItem );
    connect( this, &CTabPageBase::sigViewData, this, &CPlayStateCompare::slotViewMedia );
}

void CPlayStateCompare::setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CProgressSystem > progressSystem )
{
    CTabPageBase::setupPage( settings, syncSystem, mediaModel, userModel, progressSystem );

    fUsersFilterModel = new CUsersFilterModel( true, fUsersModel.get() );
    fUsersFilterModel->setSourceModel( fUsersModel.get() );

    fMediaFilterModel = new CMediaFilterModel( fMediaModel.get() );
    fMediaFilterModel->setSourceModel( fMediaModel.get() );
    connect( fMediaModel.get(), &CMediaModel::sigPendingMediaUpdate, this, &CPlayStateCompare::slotPendingMediaUpdate );

    fMediaFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );
    fUsersFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CPlayStateCompare::sigModelDataChanged ) );

    fImpl->users->setModel( fUsersFilterModel );
    fImpl->users->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->users, &QTreeView::clicked, this, &CPlayStateCompare::slotCurrentUserChanged );

    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CPlayStateCompare::slotUserMediaLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CPlayStateCompare::slotUserMediaCompletelyLoaded );

    slotSetCurrentMediaItem( QModelIndex() );
}

void CPlayStateCompare::setupActions()
{
    fActionReloadCurrentUser = new QAction( this );
    fActionReloadCurrentUser->setObjectName( QString::fromUtf8( "fActionReloadCurrentUser" ) );
    QIcon icon2;
    icon2.addFile( QString::fromUtf8( ":/resources/reloadUsers.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionReloadCurrentUser->setIcon( icon2 );
    fActionReloadCurrentUser->setText( QCoreApplication::translate( "CPlayStateCompare", "Reload Current User's Media", nullptr ) );
    fActionReloadCurrentUser->setToolTip( QCoreApplication::translate( "CPlayStateCompare", "Reload Current User", nullptr ) );

    fProcessMenu = new QMenu( this );
    fProcessMenu->setObjectName( QString::fromUtf8( "fProcessMenu" ) );
    fProcessMenu->setTitle( QCoreApplication::translate( "CPlayStateCompare", "Process", nullptr ) );

    fActionProcess = new QAction( this );
    fActionProcess->setObjectName( QString::fromUtf8( "fActionProcess" ) );
    QIcon icon3;
    icon3.addFile( QString::fromUtf8( ":/resources/process.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionProcess->setIcon( icon3 );
    fActionProcess->setText( QCoreApplication::translate( "CPlayStateCompare", "Process Media", nullptr ) );
    fActionProcess->setToolTip( QCoreApplication::translate( "CPlayStateCompare", "Process Media", nullptr ) );

    fActionSelectiveProcess = new QAction( this );
    fActionSelectiveProcess->setObjectName( QString::fromUtf8( "fActionSelectiveProcess" ) );
    QIcon icon4;
    icon4.addFile( QString::fromUtf8( ":/resources/processRight.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionSelectiveProcess->setIcon( icon4 );
    fActionSelectiveProcess->setText( QCoreApplication::translate( "CPlayStateCompare", "Select a Server and Update other servers to it..", nullptr ) );
    fActionSelectiveProcess->setToolTip( QCoreApplication::translate( "CPlayStateCompare", "Select a Server and Update other servers to it", nullptr ) );

    fProcessMenu->addAction( fActionProcess );
    fProcessMenu->addAction( fActionSelectiveProcess );

    fViewMenu = new QMenu( this );
    fViewMenu->setObjectName( QString::fromUtf8( "fViewMenu" ) );
    fViewMenu->setTitle( QCoreApplication::translate( "CPlayStateCompare", "View", nullptr ) );

    fActionViewMediaInformation = new QAction( this );
    fActionViewMediaInformation->setObjectName( QString::fromUtf8( "fActionViewMediaInformation" ) );
    fActionViewMediaInformation->setText( QCoreApplication::translate( "CPlayStateCompare", "Media Information...", nullptr ) );

    fViewMenu->addAction( fActionViewMediaInformation );

    fActionOnlyShowSyncableUsers = new QAction( this );
    fActionOnlyShowSyncableUsers->setObjectName( QString::fromUtf8( "fActionOnlyShowSyncableUsers" ) );
    fActionOnlyShowSyncableUsers->setCheckable( true );
    fActionOnlyShowSyncableUsers->setText( QCoreApplication::translate( "CPlayStateCompare", "Only Show Syncable Users?", nullptr ) );
    QIcon icon6;
    icon6.addFile( QString::fromUtf8( ":/resources/syncusers.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionOnlyShowSyncableUsers->setIcon( icon6 );

    fActionOnlyShowMediaWithDifferences = new QAction( this );
    fActionOnlyShowMediaWithDifferences->setObjectName( QString::fromUtf8( "fActionOnlyShowMediaWithDifferences" ) );
    fActionOnlyShowMediaWithDifferences->setCheckable( true );
    fActionOnlyShowMediaWithDifferences->setText( QCoreApplication::translate( "CPlayStateCompare", "Only Show Media with Differences?", nullptr ) );
    QIcon icon7;
    icon7.addFile( QString::fromUtf8( ":/resources/syncmedia.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionOnlyShowMediaWithDifferences->setIcon( icon7 );

    fActionShowMediaWithIssues = new QAction( this );
    fActionShowMediaWithIssues->setObjectName( QString::fromUtf8( "fActionShowMediaWithIssues" ) );
    fActionShowMediaWithIssues->setCheckable( true );
    QIcon icon5;
    icon5.addFile( QString::fromUtf8( ":/resources/issues.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionShowMediaWithIssues->setIcon( icon5 );
    fActionShowMediaWithIssues->setText( QCoreApplication::translate( "CPlayStateCompare", "Show Media with Issues?", nullptr ) );
    fActionShowMediaWithIssues->setToolTip( QCoreApplication::translate( "CPlayStateCompare", "Show Media with Issues?", nullptr ) );

    fEditActions = { fActionOnlyShowSyncableUsers, fActionOnlyShowMediaWithDifferences, fActionShowMediaWithIssues };
    fToolBar = new QToolBar( this );
    fToolBar->setObjectName( QString::fromUtf8( "fToolBar" ) );

    fToolBar->addAction( fActionOnlyShowSyncableUsers );
    fToolBar->addAction( fActionOnlyShowMediaWithDifferences );
    fToolBar->addAction( fActionShowMediaWithIssues );
    fToolBar->addSeparator();
    fToolBar->addAction( fActionProcess );
    fToolBar->addAction( fActionSelectiveProcess );

    connect( fActionReloadCurrentUser, &QAction::triggered, this, &CPlayStateCompare::slotReloadCurrentUser );

    connect( fActionProcess, &QAction::triggered, this, &CPlayStateCompare::slotProcess );
    connect( fActionSelectiveProcess, &QAction::triggered, this, &CPlayStateCompare::slotSelectiveProcess );

    connect( fActionViewMediaInformation, &QAction::triggered, this, &CPlayStateCompare::slotViewMediaInfo );

    connect( fActionOnlyShowSyncableUsers, &QAction::triggered, this, &CPlayStateCompare::slotToggleOnlyShowSyncableUsers );
    connect( fActionOnlyShowMediaWithDifferences, &QAction::triggered, this, &CPlayStateCompare::slotToggleOnlyShowMediaWithDifferences );
    connect( fActionShowMediaWithIssues, &QAction::triggered, this, &CPlayStateCompare::slotToggleShowMediaWithIssues );

}

CPlayStateCompare::~CPlayStateCompare()
{
    if ( fMediaWindow )
        delete fMediaWindow.data();
}

bool CPlayStateCompare::prepForClose()
{
    if ( fMediaWindow )
    {
        if ( !fMediaWindow->okToClose() )
            return false;
    }
    return true;
}

void CPlayStateCompare::loadSettings()
{
    fActionOnlyShowSyncableUsers->setChecked( fSettings->onlyShowSyncableUsers() );
    fActionOnlyShowMediaWithDifferences->setChecked( fSettings->onlyShowMediaWithDifferences() );
    fActionShowMediaWithIssues->setChecked( fSettings->showMediaWithIssues() );
}

bool CPlayStateCompare::okToClose()
{
    bool okToClose = fMediaWindow ? fMediaWindow->okToClose() : true;
    return okToClose;
}

void CPlayStateCompare::reset()
{
    resetPage();
}

void CPlayStateCompare::resetPage()
{
}

void CPlayStateCompare::slotCanceled()
{
    fSyncSystem->slotCanceled();
}

void CPlayStateCompare::slotSettingsChanged()
{
    loadServers();
}

void CPlayStateCompare::slotModelDataChanged()
{
    bool hasCurrentUser = fImpl->users->selectionModel()->currentIndex().isValid();
    bool canSync = fSettings->canAnyServerSync();
    bool hasDataToProcess = canSync && fMediaModel->hasMediaToProcess();
    bool mediaLoaded = canSync && fMediaModel->rowCount();
    bool hasUsersNeedingFixing = fUsersModel->hasUsersWithConnectedIDNeedingUpdate();

    fActionReloadCurrentUser->setEnabled( canSync && hasCurrentUser );

    fActionProcess->setEnabled( hasDataToProcess );
    fActionSelectiveProcess->setEnabled( hasDataToProcess );

    fActionViewMediaInformation->setEnabled( mediaLoaded );

    if ( fMediaWindow )
        fMediaWindow->reloadMedia();
}

void CPlayStateCompare::loadingUsersFinished()
{
    onlyShowSyncableUsers();
    fUsersFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    NSABUtils::autoSize( fImpl->users, -1 );
}

QSplitter * CPlayStateCompare::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CPlayStateCompare::slotReloadCurrentUser()
{
    fSyncSystem->clearCurrUser();
    auto currIdx = fImpl->users->selectionModel()->currentIndex();
    slotCurrentUserChanged( currIdx );
}

void CPlayStateCompare::slotCurrentUserChanged( const QModelIndex & index )
{
    fActionReloadCurrentUser->setEnabled( index.isValid() );

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


std::shared_ptr< CUserData > CPlayStateCompare::getCurrUserData() const
{
    auto idx = fImpl->users->selectionModel()->currentIndex();
    if ( !idx.isValid() )
        return {};

    return getUserData( idx );
}

std::shared_ptr< CUserData > CPlayStateCompare::getUserData( QModelIndex idx ) const
{
    if ( idx.model() != fUsersModel.get() )
        idx = fUsersFilterModel->mapToSource( idx );

    auto retVal = fUsersModel->userData( idx );
    return retVal;
}

void CPlayStateCompare::slotToggleOnlyShowSyncableUsers()
{
    fSettings->setOnlyShowSyncableUsers( fActionOnlyShowSyncableUsers->isChecked() );
    onlyShowSyncableUsers();
}

void CPlayStateCompare::onlyShowSyncableUsers()
{
    NSABUtils::CAutoWaitCursor awc;
    auto usersSummary = fUsersModel->settingsChanged();
    fImpl->usersLabel->setText( tr( "Users: %1 sync-able out of %2 total users" ).arg( usersSummary.fSyncable ).arg( usersSummary.fTotal ) );
}

void CPlayStateCompare::slotToggleOnlyShowMediaWithDifferences()
{
    fSettings->setOnlyShowMediaWithDifferences( fActionOnlyShowMediaWithDifferences->isChecked() );
    onlyShowMediaWithDifferences();
}

void CPlayStateCompare::onlyShowMediaWithDifferences()
{
    NSABUtils::CAutoWaitCursor awc;

    fMediaModel->settingsChanged();

    fProgressSystem->resetProgress();

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

    autoSizeDataTrees();
}

void CPlayStateCompare::slotToggleShowMediaWithIssues()
{
    fSettings->setShowMediaWithIssues( fActionShowMediaWithIssues->isChecked() );
    showMediaWithIssues();
}

void CPlayStateCompare::showMediaWithIssues()
{
    NSABUtils::CAutoWaitCursor awc;

    fMediaModel->settingsChanged();
    fProgressSystem->resetProgress();

    auto mediaSummary = SMediaSummary( fMediaModel );
    fImpl->mediaSummaryLabel->setText( mediaSummary.getSummaryText() );
}

std::shared_ptr< CTabUIInfo > CPlayStateCompare::getUIInfo() const
{
    auto retVal = std::make_shared< CTabUIInfo >();

    retVal->fMenus = { fProcessMenu, fViewMenu };
    retVal->fToolBars = { fToolBar };

    retVal->fActions[ "Edit" ] = std::make_pair( true, QList< QPointer< QAction > >( { fActionOnlyShowSyncableUsers, fActionOnlyShowMediaWithDifferences, fActionShowMediaWithIssues } ) );
    retVal->fActions[ "Reload" ] = std::make_pair( false, QList< QPointer< QAction > >( { fActionReloadCurrentUser } ) );
    return retVal;
}

std::shared_ptr< CMediaData > CPlayStateCompare::getMediaData( QModelIndex idx ) const
{
    if ( idx.model() != fMediaModel.get() )
    {
        idx = fMediaFilterModel->mapToSource( idx );
    }

    auto mediaData = fMediaModel->getMediaData( idx );
    return mediaData;
}

void CPlayStateCompare::slotPendingMediaUpdate()
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

void CPlayStateCompare::loadServers()
{
    CTabPageBase::loadServers( fMediaFilterModel );
}

void CPlayStateCompare::slotViewMedia( const QModelIndex & current )
{
    slotViewMediaInfo();
    slotSetCurrentMediaItem( current );
}

void CPlayStateCompare::slotSetCurrentMediaItem( const QModelIndex & current )
{
    auto mediaInfo = getMediaData( current );

    if ( fMediaWindow )
        fMediaWindow->setMedia( mediaInfo );

    slotModelDataChanged();
}

void CPlayStateCompare::slotViewMediaInfo()
{
    if ( !fMediaWindow )
        fMediaWindow = new CMediaWindow( fSettings, fSyncSystem, nullptr );

    auto idx = currentDataIndex();
    if ( idx.isValid() )
    {
        auto mediaInfo = getMediaData( idx );
        if ( mediaInfo )
        {
            fMediaWindow->setMedia( mediaInfo );
        }
    }

    fMediaWindow->show();
    fMediaWindow->activateWindow();
    fMediaWindow->raise();
}

void CPlayStateCompare::slotUserMediaLoaded()
{
    auto currUser = getCurrUserData();
    if ( !currUser )
        return;

    hideDataTreeColumns();
}


void CPlayStateCompare::slotUserMediaCompletelyLoaded()
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

void CPlayStateCompare::slotProcess()
{
    fSyncSystem->slotProcessMedia();
}

void CPlayStateCompare::slotSelectiveProcess()
{
    auto serverName = selectServer();
    if ( serverName.isEmpty() )
        return;

    fSyncSystem->selectiveProcessMedia( serverName );
}
