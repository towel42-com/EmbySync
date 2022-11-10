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

CMissingEpisodes::CMissingEpisodes( QWidget * parent )
    : CTabPageBase( parent ),
    fImpl( new Ui::CMissingEpisodes )
{
    fImpl->setupUi( this );
    setupActions();

    connect( this, &CMissingEpisodes::sigModelDataChanged, this, &CMissingEpisodes::slotModelDataChanged );
    connect( this, &CMissingEpisodes::sigDataContextMenuRequested, this, &CMissingEpisodes::slotUsersContextMenu );

    connect( fImpl->searchByDate, &QCheckBox::toggled, this, &CMissingEpisodes::slotSearchByDateChanged );
    connect( fImpl->searchByShowName, &QCheckBox::toggled, this, &CMissingEpisodes::slotSearchByShowNameChanged );

    connect( fImpl->shows, qOverload< int >( &QComboBox::currentIndexChanged ),
             [this]()
             {
                 if ( !fMissingMediaModel )
                     return;
                 auto currText = fImpl->shows->currentText();
                 fMissingMediaModel->setShowFilter( currText );
             } );

    QSettings settings;
    fImpl->minPremiereDate->setDate( QDate::currentDate().addDays( -7 ) );
    fImpl->maxPremiereDate->setDate( QDate::currentDate().addDays( 7 ) );

    settings.beginGroup( "MissingEpisodes" );
    fImpl->searchByDate->setChecked( settings.value( "SearchByDate", true ).toBool() );
    fImpl->searchByShowName->setChecked( settings.value( "SearchByShowName", false ).toBool() );

    NSABUtils::autoSize( fImpl->shows );
    slotSearchByDateChanged();
    slotSearchByShowNameChanged();

}

void CMissingEpisodes::slotSearchByShowNameChanged()
{
    bool hasShows = fImpl->shows->count() != 0;
    bool enabled = hasShows && fImpl->searchByShowName->isChecked();
    fImpl->shows->setEnabled( enabled );
    fImpl->searchByShowName->setEnabled( hasShows );
}

void CMissingEpisodes::slotSearchByDateChanged()
{
    fImpl->minPremiereDate->setEnabled( fImpl->searchByDate->isChecked() );
    fImpl->maxPremiereDate->setEnabled( fImpl->searchByDate->isChecked() );
}

CMissingEpisodes::~CMissingEpisodes()
{
    QSettings settings;
    settings.beginGroup( "MissingEpisodes" );
    settings.setValue( "SearchByDate", fImpl->searchByDate->isChecked() );
    settings.setValue( "SearchByShowName", fImpl->searchByShowName->isChecked() );
}

void CMissingEpisodes::setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem )
{
    CTabPageBase::setupPage( settings, syncSystem, mediaModel, userModel, serverModel, progressSystem );

    fServerFilterModel = new CServerFilterModel( fServerModel.get() );
    fServerFilterModel->setSourceModel( fServerModel.get() );
    fServerFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CMissingEpisodes::sigModelDataChanged ) );

    fImpl->servers->setModel( fServerFilterModel );
    fImpl->servers->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->servers, &QTreeView::clicked, this, &CMissingEpisodes::slotCurrentUserChanged );

    slotMediaChanged();

    connect( fMediaModel.get(), &CMediaModel::sigMediaChanged, this, &CMissingEpisodes::slotMediaChanged );

    fMissingMediaModel = new CMediaMissingFilterModel( settings, fMediaModel.get() );
    fMissingMediaModel->setSourceModel( fMediaModel.get() );
    //connect( fMediaModel.get(), &CMediaModel::sigPendingMediaUpdate, this, &CPlayStateCompare::slotPendingMediaUpdate );
    connect( fSyncSystem.get(), &CSyncSystem::sigMissingEpisodesLoaded, this, &CMissingEpisodes::slotMissingEpisodesLoaded );

    slotSetCurrentServer( QModelIndex() );
    slotToggleShowEnabledServers();
}

void CMissingEpisodes::slotMediaChanged()
{
    auto tmp = fMediaModel->getKnownShows();
    QStringList showNames;
    for ( auto && ii : tmp )
    {
        showNames.push_back( ii );
    }
    showNames.sort();
    showNames.removeAll( QString() );
    if ( !showNames.isEmpty() )
        showNames.push_front( QString() );
    auto curr = fImpl->shows->currentText();
    fImpl->shows->clear();
    fImpl->shows->addItems( showNames );
    if ( !showNames.isEmpty() )
    {
        auto pos = fImpl->shows->findText( curr );
        if ( pos != -1 )
            fImpl->shows->setCurrentIndex( pos );
    }
    NSABUtils::autoSize( fImpl->shows );
    slotSearchByShowNameChanged();
}

void CMissingEpisodes::setupActions()
{
    fActionOnlyShowEnabledServers = new QAction( this );
    fActionOnlyShowEnabledServers->setObjectName( QString::fromUtf8( "fActionOnlyShowEnabledServers" ) );
    fActionOnlyShowEnabledServers->setCheckable( true );
    fActionOnlyShowEnabledServers->setText( QCoreApplication::translate( "CPlayStateCompare", "Only Show Syncable Users?", nullptr ) );
    QIcon icon6;
    icon6.addFile( QString::fromUtf8( ":/resources/showEnabled.png" ), QSize(), QIcon::Normal, QIcon::Off );
    fActionOnlyShowEnabledServers->setIcon( icon6 );

    fToolBar = new QToolBar( this );
    fToolBar->setObjectName( QString::fromUtf8( "fToolBar" ) );

    fToolBar->addAction( fActionOnlyShowEnabledServers );

    connect( fActionOnlyShowEnabledServers, &QAction::triggered, this, &CMissingEpisodes::slotToggleShowEnabledServers );
}

bool CMissingEpisodes::prepForClose()
{
    return true;
}

void CMissingEpisodes::loadSettings()
{
    fActionOnlyShowEnabledServers->setChecked( fSettings->onlyShowEnabledServers() );
}

bool CMissingEpisodes::okToClose()
{
    bool okToClose = true;
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
    slotToggleShowEnabledServers();
}

void CMissingEpisodes::slotModelDataChanged()
{
}

void CMissingEpisodes::loadingUsersFinished()
{
}

QSplitter * CMissingEpisodes::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CMissingEpisodes::slotCurrentUserChanged( const QModelIndex & index )
{
    if ( fSyncSystem->isRunning() )
        return;

    auto prevUserData = fSyncSystem->currUser();

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

    auto minDate = fImpl->searchByDate->isChecked() ? fImpl->minPremiereDate->date() : QDate();
    auto maxDate = fImpl->searchByDate->isChecked() ? fImpl->maxPremiereDate->date() : QDate();
    if ( !fSyncSystem->loadMissingEpisodes( serverInfo, minDate, maxDate ) )
    {
        QMessageBox::critical( this, tr( "No Admin User Found" ), tr( "No user found with Administrator Privileges on server '%1'" ).arg( serverInfo->displayName() ) );
    }
}


std::shared_ptr< CServerInfo > CMissingEpisodes::getCurrentServerInfo() const
{
    auto idx = fImpl->servers->selectionModel()->currentIndex();
    if ( !idx.isValid() )
        return {};

    return getServerInfo( idx );
}

std::shared_ptr< CTabUIInfo > CMissingEpisodes::getUIInfo() const
{
    auto retVal = std::make_shared< CTabUIInfo >();

    retVal->fMenus = {};
    retVal->fToolBars = { fToolBar };

    retVal->fActions[ "Filter" ] = std::make_pair( false, QList< QPointer< QAction > >( { fActionOnlyShowEnabledServers }  ) );
    return retVal;
}

std::shared_ptr< CServerInfo > CMissingEpisodes::getServerInfo( QModelIndex idx ) const
{
    if ( idx.model() != fServerModel.get() )
        idx = fServerFilterModel->mapToSource( idx );

    auto retVal = fServerModel->getServerInfo( idx );
    return retVal;
}

void CMissingEpisodes::loadServers()
{
    CTabPageBase::loadServers( fMissingMediaModel );
}

void CMissingEpisodes::createServerTrees( QAbstractItemModel * model )
{
    addDataTreeForServer( nullptr, model );
}

void CMissingEpisodes::slotSetCurrentServer( const QModelIndex & current )
{
    auto serverInfo = getServerInfo( current );
    slotModelDataChanged();
}

void CMissingEpisodes::slotMissingEpisodesLoaded()
{
    auto currUser = getCurrentServerInfo();
    if ( !currUser )
        return;

    hideDataTreeColumns();
    sortDataTrees();
}

void CMissingEpisodes::slotToggleShowEnabledServers()
{   
    fSettings->setOnlyShowEnabledServers( fActionOnlyShowEnabledServers->isChecked() );
    showEnabledServers();
}

void CMissingEpisodes::showEnabledServers()
{
    NSABUtils::CAutoWaitCursor awc;
    fServerFilterModel->setOnlyShowEnabledServers( fSettings->onlyShowEnabledServers() );
}

std::shared_ptr< CMediaData > CMissingEpisodes::getMediaData( QModelIndex idx ) const
{
    if ( idx.model() != fMediaModel.get() )
        idx = fMissingMediaModel->mapToSource( idx );

    auto retVal = fMediaModel->getMediaData( idx );
    return retVal;
}

void CMissingEpisodes::slotUsersContextMenu( CDataTree * dataTree, const QPoint & pos )
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

    QAction action( "Search for Torrent" );
    menu.addAction( &action );
    connect( &action, &QAction::triggered, 
             [mediaData]()
             {
                 auto url = mediaData->getSearchURL();
                 QDesktopServices::openUrl( url );
             } );

    menu.exec( dataTree->dataTree()->mapToGlobal( pos ) );
}
