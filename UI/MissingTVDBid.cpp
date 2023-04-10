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

#include "MissingTVDBid.h"
#include "ui_MissingTVDBid.h"

#include "DataTree.h"
#include "MediaWindow.h"
#include "TabUIInfo.h"

#include "Core/MediaModel.h"
#include "Core/MediaData.h"
#include "Core/ProgressSystem.h"
#include "Core/ServerInfo.h"
#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/ServerModel.h"

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
#include <QDesktopServices>

CMissingTVDBid::CMissingTVDBid( QWidget *parent ) :
    CTabPageBase( parent ),
    fImpl( new Ui::CMissingTVDBid )
{
    fImpl->setupUi( this );
    setupActions();

    connect( this, &CMissingTVDBid::sigModelDataChanged, this, &CMissingTVDBid::slotModelDataChanged );
    connect( this, &CMissingTVDBid::sigDataContextMenuRequested, this, &CMissingTVDBid::slotMediaContextMenu );
}

CMissingTVDBid::~CMissingTVDBid()
{
}

void CMissingTVDBid::setupPage(
    std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CCollectionsModel > collectionsModel, std::shared_ptr< CUsersModel > userModel,
    std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem )
{
    CTabPageBase::setupPage( settings, syncSystem, mediaModel, collectionsModel, userModel, serverModel, progressSystem );

    fServerFilterModel = new CServerFilterModel( fServerModel.get() );
    fServerFilterModel->setSourceModel( fServerModel.get() );
    fServerFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CMissingTVDBid::sigModelDataChanged ) );

    fImpl->servers->setModel( fServerFilterModel );
    fImpl->servers->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->servers, &QTreeView::clicked, this, &CMissingTVDBid::slotCurrentServerChanged );

    slotMediaChanged();

    connect( fMediaModel.get(), &CMediaModel::sigMediaChanged, this, &CMissingTVDBid::slotMediaChanged );

    fMissingMediaModel = new CMediaMissingFilterModel( settings, fMediaModel.get() );
    fMissingMediaModel->setSourceModel( fMediaModel.get() );
    // connect( fMediaModel.get(), &CMediaModel::sigPendingMediaUpdate, this, &CPlayStateCompare::slotPendingMediaUpdate );
    connect( fSyncSystem.get(), &CSyncSystem::sigMissingTVDBidLoaded, this, &CMissingTVDBid::slotMissingTVDBidLoaded );

    slotSetCurrentServer( QModelIndex() );
    showPrimaryServer();
}

void CMissingTVDBid::slotMediaChanged()
{
}

void CMissingTVDBid::setupActions()
{
    fToolBar = new QToolBar( this );
    fToolBar->setObjectName( QString::fromUtf8( "fToolBar" ) );
}

bool CMissingTVDBid::prepForClose()
{
    return true;
}

void CMissingTVDBid::loadSettings()
{
}

bool CMissingTVDBid::okToClose()
{
    bool okToClose = true;
    return okToClose;
}

void CMissingTVDBid::reset()
{
    resetPage();
}

void CMissingTVDBid::resetPage()
{
}

void CMissingTVDBid::slotCanceled()
{
    fSyncSystem->slotCanceled();
}

void CMissingTVDBid::slotSettingsChanged()
{
    loadServers();
    showPrimaryServer();
}

void CMissingTVDBid::showPrimaryServer()
{
    NSABUtils::CAutoWaitCursor awc;
    fServerFilterModel->setOnlyShowEnabledServers( true );
    fServerFilterModel->setOnlyShowPrimaryServer( true );
}

void CMissingTVDBid::slotModelDataChanged()
{
}

void CMissingTVDBid::loadingUsersFinished()
{
}

QSplitter *CMissingTVDBid::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CMissingTVDBid::slotCurrentServerChanged( const QModelIndex &index )
{
    if ( fSyncSystem->isRunning() )
        return;

    auto idx = index;
    if ( !idx.isValid() )
        idx = fImpl->servers->selectionModel()->currentIndex();

    if ( !index.isValid() )
        return;

    auto serverInfo = getServerInfo( idx );
    if ( !serverInfo )
        return;

    if ( !fDataTrees.empty() )
    {
        fDataTrees[ 0 ]->setServer( serverInfo, true );
    }

    fMediaModel->clear();

    if ( !fSyncSystem->loadMissingTVDBid( serverInfo ) )
    {
        QMessageBox::critical( this, tr( "No Admin User Found" ), tr( "No user found with Administrator Privileges on server '%1'" ).arg( serverInfo->displayName() ) );
    }
}

std::shared_ptr< CServerInfo > CMissingTVDBid::getCurrentServerInfo() const
{
    auto idx = fImpl->servers->selectionModel()->currentIndex();
    if ( !idx.isValid() )
        return {};

    return getServerInfo( idx );
}

std::shared_ptr< CTabUIInfo > CMissingTVDBid::getUIInfo() const
{
    auto retVal = std::make_shared< CTabUIInfo >();

    retVal->fMenus = {};
    retVal->fToolBars = { fToolBar };

    return retVal;
}

std::shared_ptr< CServerInfo > CMissingTVDBid::getServerInfo( QModelIndex idx ) const
{
    if ( idx.model() != fServerModel.get() )
        idx = fServerFilterModel->mapToSource( idx );

    auto retVal = fServerModel->getServerInfo( idx );
    return retVal;
}

void CMissingTVDBid::loadServers()
{
    CTabPageBase::loadServers( fMissingMediaModel );
}

void CMissingTVDBid::createServerTrees( QAbstractItemModel *model )
{
    addDataTreeForServer( nullptr, model );
}

void CMissingTVDBid::slotSetCurrentServer( const QModelIndex &current )
{
    auto serverInfo = getServerInfo( current );
    slotModelDataChanged();
}

void CMissingTVDBid::slotMissingTVDBidLoaded()
{
    auto currServer = getCurrentServerInfo();
    if ( !currServer )
        return;

    hideDataTreeColumns();
    sortDataTrees();
}

std::shared_ptr< CMediaData > CMissingTVDBid::getMediaData( QModelIndex idx ) const
{
    if ( idx.model() != fMediaModel.get() )
        idx = fMissingMediaModel->mapToSource( idx );

    auto retVal = fMediaModel->getMediaData( idx );
    return retVal;
}

void CMissingTVDBid::slotMediaContextMenu( CDataTree *dataTree, const QPoint &pos )
{
    if ( !dataTree )
        return;

    auto idx = dataTree->indexAt( pos );
    if ( !idx.isValid() )
        return;

    auto mediaData = getMediaData( idx );
    if ( !mediaData )
        return;

    QMenu menu( tr( "Context Menu" ) );
    mediaData->addSearchMenu( &menu );
    menu.exec( dataTree->dataTree()->mapToGlobal( pos ) );
}
