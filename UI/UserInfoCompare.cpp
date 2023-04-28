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

#include "UserInfoCompare.h"
#include "ui_UserInfoCompare.h"

#include "DataTree.h"
#include "TabUIInfo.h"
#include "UserWindow.h"

#include "Core/MediaModel.h"
#include "Core/ProgressSystem.h"
#include "Core/ServerInfo.h"
#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/UsersModel.h"
#include "Core/ServerModel.h"

#include "SABUtils/AutoWaitCursor.h"
#include "SABUtils/QtUtils.h"
#include "SABUtils/WidgetChanged.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QMetaMethod>
#include <QTimer>
#include <QToolBar>
#include <QCloseEvent>
#include <QTreeView>

CUserInfoCompare::CUserInfoCompare( QWidget *parent ) :
    CTabPageBase( parent ),
    fImpl( new Ui::CUserInfoCompare )
{
    fImpl->setupUi( this );
    setupActions();

    connect( this, &CUserInfoCompare::sigModelDataChanged, this, &CUserInfoCompare::slotModelDataChanged );
    connect( this, &CUserInfoCompare::sigDataContextMenuRequested, this, &CUserInfoCompare::slotUsersContextMenu );

    connect( this, &CTabPageBase::sigSetCurrentDataItem, this, &CUserInfoCompare::slotSetCurrentUser );
    connect( this, &CTabPageBase::sigViewData, this, &CUserInfoCompare::slotViewUser );
}

void CUserInfoCompare::setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CCollectionsModel > collectionsModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem )
{
    CTabPageBase::setupPage( settings, syncSystem, mediaModel, collectionsModel, userModel, serverModel, progressSystem );

    fUsersFilterModel = new CUsersFilterModel( false, fUsersModel.get() );
    fUsersFilterModel->setSourceModel( fUsersModel.get() );

    fUsersFilterModel->sort( -1, Qt::SortOrder::AscendingOrder );
}

void CUserInfoCompare::setupActions()
{
    fProcessMenu = new QMenu( this );
    fProcessMenu->setObjectName( QString::fromUtf8( "fProcessMenu" ) );
    fProcessMenu->setTitle( QCoreApplication::translate( "CUserInfoCompare", "Process", nullptr ) );

    fActionProcess = new QAction( this );
    fActionProcess->setObjectName( QString::fromUtf8( "fActionProcess" ) );
    setIcon( QString::fromUtf8( ":/resources/process.png" ), fActionProcess );
    fActionProcess->setText( QCoreApplication::translate( "CUserInfoCompare", "Process Media", nullptr ) );
    fActionProcess->setToolTip( QCoreApplication::translate( "CUserInfoCompare", "Process Media", nullptr ) );

    fActionSelectiveProcess = new QAction( this );
    fActionSelectiveProcess->setObjectName( QString::fromUtf8( "fActionSelectiveProcess" ) );
    setIcon( QString::fromUtf8( ":/resources/processRight.png" ), fActionSelectiveProcess );
    fActionSelectiveProcess->setText( QCoreApplication::translate( "CUserInfoCompare", "Select a Server and Update other servers to it..", nullptr ) );
    fActionSelectiveProcess->setToolTip( QCoreApplication::translate( "CUserInfoCompare", "Select a Server and Update other servers to it", nullptr ) );

    fActionRepairUserConnectedIDs = new QAction( this );
    fActionRepairUserConnectedIDs->setObjectName( QString::fromUtf8( "fActionRepairUserConnectedIDs" ) );
    fActionRepairUserConnectedIDs->setText( QCoreApplication::translate( "CUserInfoCompare", "Repair User Connected IDs", nullptr ) );
    fActionRepairUserConnectedIDs->setToolTip( QCoreApplication::translate( "CUserInfoCompare", "Repair User Connected IDs", nullptr ) );

    fProcessMenu->addAction( fActionProcess );
    fProcessMenu->addAction( fActionSelectiveProcess );
    fProcessMenu->addSeparator();
    fProcessMenu->addAction( fActionRepairUserConnectedIDs );

    fViewMenu = new QMenu( this );
    fViewMenu->setObjectName( QString::fromUtf8( "fViewMenu" ) );
    fViewMenu->setTitle( QCoreApplication::translate( "CUserInfoCompare", "View", nullptr ) );

    fActionViewUserInformation = new QAction( this );
    fActionViewUserInformation->setObjectName( QString::fromUtf8( "fActionViewUserInformation" ) );
    fActionViewUserInformation->setText( QCoreApplication::translate( "CUserInfoCompare", "User Information...", nullptr ) );

    fViewMenu->addAction( fActionViewUserInformation );
    connect( fActionViewUserInformation, &QAction::triggered, this, &CUserInfoCompare::slotViewUserInfo );

    fActionOnlyShowSyncableUsers = new QAction( this );
    fActionOnlyShowSyncableUsers->setObjectName( QString::fromUtf8( "fActionOnlyShowSyncableUsers" ) );
    fActionOnlyShowSyncableUsers->setCheckable( true );
    fActionOnlyShowSyncableUsers->setText( QCoreApplication::translate( "CUserInfoCompare", "Only Show Syncable Users?", nullptr ) );
    setIcon( QString::fromUtf8( ":/resources/syncusers.png" ), fActionOnlyShowSyncableUsers );

    fActionOnlyShowUsersWithDifferences = new QAction( this );
    fActionOnlyShowUsersWithDifferences->setObjectName( QString::fromUtf8( "fActionOnlyShowUsersWithDifferences" ) );
    fActionOnlyShowUsersWithDifferences->setCheckable( true );
    fActionOnlyShowUsersWithDifferences->setText( QCoreApplication::translate( "CUserInfoCompare", "Only Show Users with Differences?", nullptr ) );
    fActionOnlyShowUsersWithDifferences->setToolTip( QCoreApplication::translate( "CUserInfoCompare", "Only Show Users with Differences?", nullptr ) );
    setIcon( QString::fromUtf8( ":/resources/syncmedia.png" ), fActionOnlyShowUsersWithDifferences );

    fActionShowUsersWithIssues = new QAction( this );
    fActionShowUsersWithIssues->setObjectName( QString::fromUtf8( "fActionShowUsersWithIssues" ) );
    fActionShowUsersWithIssues->setCheckable( true );
    setIcon( QString::fromUtf8( ":/resources/issues.png" ), fActionShowUsersWithIssues );
    fActionShowUsersWithIssues->setText( QCoreApplication::translate( "CUserInfoCompare", "Show Users with Issues?", nullptr ) );
    fActionShowUsersWithIssues->setToolTip( QCoreApplication::translate( "CUserInfoCompare", "Show Users with Issues?", nullptr ) );

    fToolBar = new QToolBar( this );
    fToolBar->setObjectName( QString::fromUtf8( "fToolBar" ) );

    fToolBar->addAction( fActionOnlyShowSyncableUsers );
    fToolBar->addAction( fActionOnlyShowUsersWithDifferences );
    fToolBar->addAction( fActionShowUsersWithIssues );
    fToolBar->addSeparator();
    fToolBar->addAction( fActionProcess );
    fToolBar->addAction( fActionSelectiveProcess );

    connect( fActionProcess, &QAction::triggered, this, &CUserInfoCompare::slotProcess );
    connect( fActionSelectiveProcess, &QAction::triggered, this, &CUserInfoCompare::slotSelectiveProcess );
    connect( fActionRepairUserConnectedIDs, &QAction::triggered, this, &CUserInfoCompare::slotRepairUserConnectedIDs );

    connect( fActionOnlyShowSyncableUsers, &QAction::triggered, this, &CUserInfoCompare::slotToggleOnlyShowSyncableUsers );
    connect( fActionOnlyShowUsersWithDifferences, &QAction::triggered, this, &CUserInfoCompare::slotToggleOnlyShowUsersWithDifferences );
    connect( fActionShowUsersWithIssues, &QAction::triggered, this, &CUserInfoCompare::slotToggleShowUsersWithIssues );
}

CUserInfoCompare::~CUserInfoCompare()
{
    if ( fUserWindow )
        delete fUserWindow.data();
}

bool CUserInfoCompare::prepForClose()
{
    if ( fUserWindow )
    {
        if ( !fUserWindow->okToClose() )
            return false;
    }
    return true;
}

void CUserInfoCompare::loadSettings()
{
    fActionOnlyShowSyncableUsers->setChecked( fSettings->onlyShowSyncableUsers() );
    fActionOnlyShowUsersWithDifferences->setChecked( fSettings->onlyShowUsersWithDifferences() );
    fActionShowUsersWithIssues->setChecked( fSettings->showUsersWithIssues() );
}

bool CUserInfoCompare::okToClose()
{
    bool okToClose = fUserWindow ? fUserWindow->okToClose() : true;
    return true;
}

void CUserInfoCompare::reset()
{
    resetPage();
}

void CUserInfoCompare::resetPage()
{
}

void CUserInfoCompare::slotCanceled()
{
    fSyncSystem->slotCanceled();
}

void CUserInfoCompare::slotSettingsChanged()
{
    loadServers();
}

void CUserInfoCompare::slotModelDataChanged()
{
    bool hasCurrentUser = currentDataIndex().isValid();
    bool canSync = fServerModel->canAnyServerSync();
    bool hasDataToProcess = canSync && fMediaModel->hasMediaToProcess();
    bool mediaLoaded = canSync && fMediaModel->rowCount();
    bool hasUsersNeedingFixing = fUsersModel->hasUsersWithConnectedIDNeedingUpdate();

    fActionRepairUserConnectedIDs->setEnabled( hasUsersNeedingFixing );

    if ( fUserWindow )
        fUserWindow->reloadUser();
}

void CUserInfoCompare::loadingUsersFinished()
{
    onlyShowSyncableUsers();
    fUsersFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    hideDataTreeColumns();
    autoSizeDataTrees();
}

QSplitter *CUserInfoCompare::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CUserInfoCompare::slotCurrentUserChanged( const QModelIndex & /*index*/ )
{
}

std::shared_ptr< CUserData > CUserInfoCompare::getCurrUserData() const
{
    auto idx = currentDataIndex();
    if ( !idx.isValid() )
        return {};

    return getUserData( idx );
}

std::shared_ptr< CUserData > CUserInfoCompare::getUserData( QModelIndex idx ) const
{
    if ( idx.model() != fUsersModel.get() )
        idx = fUsersFilterModel->mapToSource( idx );

    auto retVal = fUsersModel->userData( idx );
    return retVal;
}

std::shared_ptr< const CServerInfo > CUserInfoCompare::getServerInfo( QModelIndex idx ) const
{
    if ( idx.model() != fUsersModel.get() )
        idx = fUsersFilterModel->mapToSource( idx );

    auto retVal = fUsersModel->serverInfo( idx );
    return retVal;
}

void CUserInfoCompare::slotToggleOnlyShowSyncableUsers()
{
    fSettings->setOnlyShowSyncableUsers( fActionOnlyShowSyncableUsers->isChecked() );
    onlyShowSyncableUsers();
}

void CUserInfoCompare::onlyShowSyncableUsers()
{
    NSABUtils::CAutoWaitCursor awc;
    auto usersSummary = fUsersModel->settingsChanged();
    fImpl->usersLabel->setText( tr( "Users: %1 sync-able out of %2 total users" ).arg( usersSummary.fSyncable ).arg( usersSummary.fTotal ) );
}

void CUserInfoCompare::slotToggleOnlyShowUsersWithDifferences()
{
    fSettings->setOnlyShowUsersWithDifferences( fActionOnlyShowUsersWithDifferences->isChecked() );
    onlyShowUsersWithDifferences();
}

void CUserInfoCompare::onlyShowUsersWithDifferences()
{
    NSABUtils::CAutoWaitCursor awc;

    fMediaModel->settingsChanged();

    fProgressSystem->resetProgress();

    auto column = fUsersFilterModel->sortColumn();
    auto order = fUsersFilterModel->sortOrder();
    if ( column == -1 )
    {
        column = 0;
        order = Qt::AscendingOrder;
    }
    fUsersFilterModel->sort( column, order );

    for ( auto &&ii : fDataTrees )
        ii->autoSize();
}

void CUserInfoCompare::slotToggleShowUsersWithIssues()
{
    fSettings->setShowUsersWithIssues( fActionShowUsersWithIssues->isChecked() );
    showUsersWithIssues();
}

void CUserInfoCompare::showUsersWithIssues()
{
    NSABUtils::CAutoWaitCursor awc;

    fMediaModel->settingsChanged();
    fProgressSystem->resetProgress();
}

std::shared_ptr< CTabUIInfo > CUserInfoCompare::getUIInfo() const
{
    auto retVal = std::make_shared< CTabUIInfo >();

    retVal->fMenus = { fProcessMenu, fViewMenu };
    retVal->fToolBars = { fToolBar };

    retVal->fActions[ "Filter" ] = std::make_pair( false, QList< QPointer< QAction > >( { fActionOnlyShowSyncableUsers, fActionOnlyShowUsersWithDifferences, fActionShowUsersWithIssues } ) );
    return retVal;
}

void CUserInfoCompare::loadServers()
{
    CTabPageBase::loadServers( fUsersFilterModel );
}

void CUserInfoCompare::slotRepairUserConnectedIDs()
{
    auto users = fUsersModel->usersWithConnectedIDNeedingUpdate();
    fSyncSystem->repairConnectIDs( users );
}

void CUserInfoCompare::slotSetConnectID()
{
    auto currIdx = currentDataIndex();
    if ( !currIdx.isValid() )
        return;

    auto userData = getUserData( currIdx );
    if ( !userData )
        return;

    auto serverInfo = getServerInfo( currIdx );
    if ( !serverInfo && fContextTree )
        serverInfo = fContextTree->serverInfo();

    if ( !serverInfo )
        return;

    auto newConnectedID = QInputDialog::getText( this, tr( "Enter Connect ID" ), tr( "Connect ID:" ), QLineEdit::Normal, userData->connectedID( serverInfo->keyName() ) );
    if ( newConnectedID == userData->connectedID( serverInfo->keyName() ) )
        return;

    fSyncSystem->setConnectedID( serverInfo->keyName(), newConnectedID, userData );
}

void CUserInfoCompare::slotAutoSetConnectID()
{
    auto currIdx = currentDataIndex();
    if ( !currIdx.isValid() )
        return;

    auto userData = getUserData( currIdx );
    if ( !userData )
        return;

    fSyncSystem->repairConnectIDs( { userData } );
}

void CUserInfoCompare::slotUsersContextMenu( CDataTree *dataTree, const QPoint &pos )
{
    if ( !dataTree )
        return;

    fContextTree = dataTree;
    auto idx = dataTree->indexAt( pos );
    if ( !idx.isValid() )
        return;

    auto userData = getUserData( idx );
    if ( !userData )
        return;

    QMenu menu( tr( "Context Menu" ) );

    QAction action( "Set Connect ID" );
    menu.addAction( &action );
    connect( &action, &QAction::triggered, this, &CUserInfoCompare::slotSetConnectID );

    QAction action2( "AutoSet Connect ID" );
    menu.addAction( &action2 );
    action2.setEnabled( userData->connectedIDNeedsUpdate() );
    connect( &action2, &QAction::triggered, this, &CUserInfoCompare::slotAutoSetConnectID );

    menu.exec( dataTree->dataTree()->mapToGlobal( pos ) );
}

void CUserInfoCompare::slotViewUser( const QModelIndex &current )
{
    slotViewUserInfo();
    slotSetCurrentUser( current );
}

void CUserInfoCompare::slotSetCurrentUser( const QModelIndex &current )
{
    auto userData = getUserData( current );

    if ( fUserWindow )
        fUserWindow->setUser( userData );

    slotModelDataChanged();
}

void CUserInfoCompare::slotViewUserInfo()
{
    if ( !fUserWindow )
        fUserWindow = new CUserWindow( fServerModel, fSyncSystem, nullptr );

    auto idx = currentDataIndex();
    if ( idx.isValid() )
    {
        auto userData = getUserData( idx );
        if ( userData )
        {
            fUserWindow->setUser( userData );
        }
    }

    fUserWindow->show();
    fUserWindow->activateWindow();
    fUserWindow->raise();
}

void CUserInfoCompare::slotProcess()
{
    fSyncSystem->slotProcessUsers();
}

void CUserInfoCompare::slotSelectiveProcess()
{
    auto serverName = selectServer();
    if ( serverName.isEmpty() )
        return;

    fSyncSystem->selectiveProcessUsers( serverName );
}
