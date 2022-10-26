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

#include "MissingEpisodes.h"
#include "ui_MissingEpisodes.h"

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
#include <QSettings>

CMissingEpisodes::CMissingEpisodes( QWidget * parent )
    : CTabPageBase( parent ),
    fImpl( new Ui::CMissingEpisodes )
{
    fImpl->setupUi( this );
    setupActions();

    connect( this, &CMissingEpisodes::sigModelDataChanged, this, &CMissingEpisodes::slotModelDataChanged );

    connect( this, &CTabPageBase::sigSetCurrentDataItem, this, &CMissingEpisodes::slotSetCurrentMediaItem );
    connect( this, &CTabPageBase::sigViewData, this, &CMissingEpisodes::slotViewMedia );

    QSettings settings;
    fImpl->minPremiereDate->setDate( settings.value( "minPremiereDate", QDate::currentDate().addYears( -1 ) ).toDate() );
    fImpl->maxPremiereDate->setDate( settings.value( "maxPremiereDate", QDate::currentDate().addDays( 7 ) ).toDate() );
}

CMissingEpisodes::~CMissingEpisodes()
{
    QSettings settings;
    settings.setValue( "maxPremiereDate", fImpl->minPremiereDate->date() );
    settings.setValue( "minPremiereDate", fImpl->maxPremiereDate->date() );

    if ( fMediaWindow )
        delete fMediaWindow.data();
}

void CMissingEpisodes::setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CProgressSystem > progressSystem )
{
    CTabPageBase::setupPage( settings, syncSystem, mediaModel, userModel, progressSystem );

    fUsersFilterModel = new CUsersFilterModel( true, fUsersModel.get() );
    fUsersFilterModel->setSourceModel( fUsersModel.get() );

    fMediaFilterModel = new CMediaFilterModel( fMediaModel.get() );
    fMediaFilterModel->setSourceModel( fMediaModel.get() );
    connect( fMediaModel.get(), &CMediaModel::sigPendingMediaUpdate, this, &CMissingEpisodes::slotPendingMediaUpdate );

    fMediaFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );
    fUsersFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CMissingEpisodes::sigModelDataChanged ) );

    fImpl->users->setModel( fUsersFilterModel );
    fImpl->users->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->users, &QTreeView::clicked, this, &CMissingEpisodes::slotCurrentUserChanged );

    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMissingEpisodes::slotUserMediaLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigUserMediaLoaded, this, &CMissingEpisodes::slotUserMediaCompletelyLoaded );

    slotSetCurrentMediaItem( QModelIndex() );
}

void CMissingEpisodes::setupActions()
{
    fActionReloadCurrentUser = new QAction( this );
    fActionReloadCurrentUser->setObjectName( QString::fromUtf8( "fActionReloadCurrentUser" ) );
    QIcon icon2;
    icon2.addFile( QString::fromUtf8( ":/resources/reloadUsers.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionReloadCurrentUser->setIcon( icon2 );
    fActionReloadCurrentUser->setText( QCoreApplication::translate( "CMissingEpisodes", "Reload Current User's Media", nullptr ) );
    fActionReloadCurrentUser->setToolTip( QCoreApplication::translate( "CMissingEpisodes", "Reload Current User", nullptr ) );

    fProcessMenu = new QMenu( this );
    fProcessMenu->setObjectName( QString::fromUtf8( "fProcessMenu" ) );
    fProcessMenu->setTitle( QCoreApplication::translate( "CMissingEpisodes", "Process", nullptr ) );

    fActionProcess = new QAction( this );
    fActionProcess->setObjectName( QString::fromUtf8( "fActionProcess" ) );
    QIcon icon3;
    icon3.addFile( QString::fromUtf8( ":/resources/process.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionProcess->setIcon( icon3 );
    fActionProcess->setText( QCoreApplication::translate( "CMissingEpisodes", "Process Media", nullptr ) );
    fActionProcess->setToolTip( QCoreApplication::translate( "CMissingEpisodes", "Process Media", nullptr ) );

    fActionSelectiveProcess = new QAction( this );
    fActionSelectiveProcess->setObjectName( QString::fromUtf8( "fActionSelectiveProcess" ) );
    QIcon icon4;
    icon4.addFile( QString::fromUtf8( ":/resources/processRight.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionSelectiveProcess->setIcon( icon4 );
    fActionSelectiveProcess->setText( QCoreApplication::translate( "CMissingEpisodes", "Select a Server and Update other servers to it..", nullptr ) );
    fActionSelectiveProcess->setToolTip( QCoreApplication::translate( "CMissingEpisodes", "Select a Server and Update other servers to it", nullptr ) );

    fProcessMenu->addAction( fActionProcess );
    fProcessMenu->addAction( fActionSelectiveProcess );

    fViewMenu = new QMenu( this );
    fViewMenu->setObjectName( QString::fromUtf8( "fViewMenu" ) );
    fViewMenu->setTitle( QCoreApplication::translate( "CMissingEpisodes", "View", nullptr ) );

    fActionViewMediaInformation = new QAction( this );
    fActionViewMediaInformation->setObjectName( QString::fromUtf8( "fActionViewMediaInformation" ) );
    fActionViewMediaInformation->setText( QCoreApplication::translate( "CMissingEpisodes", "Media Information...", nullptr ) );

    fViewMenu->addAction( fActionViewMediaInformation );

    fActionOnlyShowSyncableUsers = new QAction( this );
    fActionOnlyShowSyncableUsers->setObjectName( QString::fromUtf8( "fActionOnlyShowSyncableUsers" ) );
    fActionOnlyShowSyncableUsers->setCheckable( true );
    fActionOnlyShowSyncableUsers->setText( QCoreApplication::translate( "CMissingEpisodes", "Only Show Syncable Users?", nullptr ) );
    QIcon icon6;
    icon6.addFile( QString::fromUtf8( ":/resources/syncusers.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionOnlyShowSyncableUsers->setIcon( icon6 );

    fActionOnlyShowMediaWithDifferences = new QAction( this );
    fActionOnlyShowMediaWithDifferences->setObjectName( QString::fromUtf8( "fActionOnlyShowMediaWithDifferences" ) );
    fActionOnlyShowMediaWithDifferences->setCheckable( true );
    fActionOnlyShowMediaWithDifferences->setText( QCoreApplication::translate( "CMissingEpisodes", "Only Show Media with Differences?", nullptr ) );
    QIcon icon7;
    icon7.addFile( QString::fromUtf8( ":/resources/syncmedia.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionOnlyShowMediaWithDifferences->setIcon( icon7 );

    fActionShowMediaWithIssues = new QAction( this );
    fActionShowMediaWithIssues->setObjectName( QString::fromUtf8( "fActionShowMediaWithIssues" ) );
    fActionShowMediaWithIssues->setCheckable( true );
    QIcon icon5;
    icon5.addFile( QString::fromUtf8( ":/resources/issues.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionShowMediaWithIssues->setIcon( icon5 );
    fActionShowMediaWithIssues->setText( QCoreApplication::translate( "CMissingEpisodes", "Show Media with Issues?", nullptr ) );
    fActionShowMediaWithIssues->setToolTip( QCoreApplication::translate( "CMissingEpisodes", "Show Media with Issues?", nullptr ) );

    fEditActions = { fActionOnlyShowSyncableUsers, fActionOnlyShowMediaWithDifferences, fActionShowMediaWithIssues };
    fToolBar = new QToolBar( this );
    fToolBar->setObjectName( QString::fromUtf8( "fToolBar" ) );

    fToolBar->addAction( fActionOnlyShowSyncableUsers );
    fToolBar->addAction( fActionOnlyShowMediaWithDifferences );
    fToolBar->addAction( fActionShowMediaWithIssues );
    fToolBar->addSeparator();
    fToolBar->addAction( fActionProcess );
    fToolBar->addAction( fActionSelectiveProcess );

    connect( fActionReloadCurrentUser, &QAction::triggered, this, &CMissingEpisodes::slotReloadCurrentUser );

    connect( fActionProcess, &QAction::triggered, this, &CMissingEpisodes::slotProcess );
    connect( fActionSelectiveProcess, &QAction::triggered, this, &CMissingEpisodes::slotSelectiveProcess );

    connect( fActionViewMediaInformation, &QAction::triggered, this, &CMissingEpisodes::slotViewMediaInfo );

    connect( fActionOnlyShowSyncableUsers, &QAction::triggered, this, &CMissingEpisodes::slotToggleOnlyShowSyncableUsers );
    connect( fActionOnlyShowMediaWithDifferences, &QAction::triggered, this, &CMissingEpisodes::slotToggleOnlyShowMediaWithDifferences );
    connect( fActionShowMediaWithIssues, &QAction::triggered, this, &CMissingEpisodes::slotToggleShowMediaWithIssues );

}

bool CMissingEpisodes::prepForClose()
{
    if ( fMediaWindow )
    {
        if ( !fMediaWindow->okToClose() )
            return false;
    }
    return true;
}

void CMissingEpisodes::loadSettings()
{
    fActionOnlyShowSyncableUsers->setChecked( fSettings->onlyShowSyncableUsers() );
    fActionOnlyShowMediaWithDifferences->setChecked( fSettings->onlyShowMediaWithDifferences() );
    fActionShowMediaWithIssues->setChecked( fSettings->showMediaWithIssues() );
}

bool CMissingEpisodes::okToClose()
{
    bool okToClose = fMediaWindow ? fMediaWindow->okToClose() : true;
    return okToClose;
}

void CMissingEpisodes::reset()
{
    resetPage();
}

void CMissingEpisodes::resetPage()
{
}

void CMissingEpisodes::slotCanceled()
{
    fSyncSystem->slotCanceled();
}

void CMissingEpisodes::slotSettingsChanged()
{
    loadServers();
}

void CMissingEpisodes::slotModelDataChanged()
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

void CMissingEpisodes::loadingUsersFinished()
{
    onlyShowSyncableUsers();
    fUsersFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    NSABUtils::autoSize( fImpl->users, -1 );
}

QSplitter * CMissingEpisodes::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CMissingEpisodes::slotReloadCurrentUser()
{
    fSyncSystem->clearCurrUser();
    auto currIdx = fImpl->users->selectionModel()->currentIndex();
    slotCurrentUserChanged( currIdx );
}

void CMissingEpisodes::slotCurrentUserChanged( const QModelIndex & index )
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

    fMediaModel->clear();

    fSyncSystem->loadMissingEpisodes( userData, fImpl->minPremiereDate->date(), fImpl->maxPremiereDate->date() );
}


std::shared_ptr< CUserData > CMissingEpisodes::getCurrUserData() const
{
    auto idx = fImpl->users->selectionModel()->currentIndex();
    if ( !idx.isValid() )
        return {};

    return getUserData( idx );
}

std::shared_ptr< CUserData > CMissingEpisodes::getUserData( QModelIndex idx ) const
{
    if ( idx.model() != fUsersModel.get() )
        idx = fUsersFilterModel->mapToSource( idx );

    auto retVal = fUsersModel->userData( idx );
    return retVal;
}

void CMissingEpisodes::slotToggleOnlyShowSyncableUsers()
{
    fSettings->setOnlyShowSyncableUsers( fActionOnlyShowSyncableUsers->isChecked() );
    onlyShowSyncableUsers();
}

void CMissingEpisodes::onlyShowSyncableUsers()
{
    NSABUtils::CAutoWaitCursor awc;
    auto usersSummary = fUsersModel->settingsChanged();
    fImpl->usersLabel->setText( tr( "Users: %1 sync-able out of %2 total users" ).arg( usersSummary.fSyncable ).arg( usersSummary.fTotal ) );
}

void CMissingEpisodes::slotToggleOnlyShowMediaWithDifferences()
{
    fSettings->setOnlyShowMediaWithDifferences( fActionOnlyShowMediaWithDifferences->isChecked() );
    onlyShowMediaWithDifferences();
}

void CMissingEpisodes::onlyShowMediaWithDifferences()
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

void CMissingEpisodes::slotToggleShowMediaWithIssues()
{
    fSettings->setShowMediaWithIssues( fActionShowMediaWithIssues->isChecked() );
    showMediaWithIssues();
}

void CMissingEpisodes::showMediaWithIssues()
{
    NSABUtils::CAutoWaitCursor awc;

    fMediaModel->settingsChanged();
    fProgressSystem->resetProgress();

    auto mediaSummary = SMediaSummary( fMediaModel );
    fImpl->mediaSummaryLabel->setText( mediaSummary.getSummaryText() );
}

std::shared_ptr< CTabUIInfo > CMissingEpisodes::getUIInfo() const
{
    auto retVal = std::make_shared< CTabUIInfo >();

    retVal->fMenus = { fProcessMenu, fViewMenu };
    retVal->fToolBars = { fToolBar };

    retVal->fActions[ "Edit" ] = std::make_pair( true, QList< QPointer< QAction > >( { fActionOnlyShowSyncableUsers, fActionOnlyShowMediaWithDifferences, fActionShowMediaWithIssues } ) );
    retVal->fActions[ "Reload" ] = std::make_pair( false, QList< QPointer< QAction > >( { fActionReloadCurrentUser } ) );
    return retVal;
}

std::shared_ptr< CMediaData > CMissingEpisodes::getMediaData( QModelIndex idx ) const
{
    if ( idx.model() != fMediaModel.get() )
    {
        idx = fMediaFilterModel->mapToSource( idx );
    }

    auto mediaData = fMediaModel->getMediaData( idx );
    return mediaData;
}

void CMissingEpisodes::slotPendingMediaUpdate()
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

void CMissingEpisodes::loadServers()
{
    CTabPageBase::loadServers( fMediaFilterModel );
}

void CMissingEpisodes::slotViewMedia( const QModelIndex & current )
{
    slotViewMediaInfo();
    slotSetCurrentMediaItem( current );
}

void CMissingEpisodes::slotSetCurrentMediaItem( const QModelIndex & current )
{
    auto mediaInfo = getMediaData( current );

    if ( fMediaWindow )
        fMediaWindow->setMedia( mediaInfo );

    slotModelDataChanged();
}

void CMissingEpisodes::slotViewMediaInfo()
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

void CMissingEpisodes::slotUserMediaLoaded()
{
    auto currUser = getCurrUserData();
    if ( !currUser )
        return;

    hideDataTreeColumns();
}


void CMissingEpisodes::slotUserMediaCompletelyLoaded()
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

void CMissingEpisodes::slotProcess()
{
    fSyncSystem->slotProcessMedia();
}

void CMissingEpisodes::slotSelectiveProcess()
{
    auto serverName = selectServer();
    if ( serverName.isEmpty() )
        return;

    fSyncSystem->selectiveProcessMedia( serverName );
}
