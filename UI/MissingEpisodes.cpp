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
#include <QStyledItemDelegate>


class CEditDelegate : public QStyledItemDelegate
{
public:
    CEditDelegate( QObject *parent = 0 ) :
        QStyledItemDelegate( parent )
    {
    }
    virtual QWidget *createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const 
    { 
        if ( index.column() == 0 )
            return nullptr;
        return QStyledItemDelegate::createEditor( parent, option, index );
    }
};

CMissingEpisodes::CMissingEpisodes( QWidget *parent ) :
    CTabPageBase( parent ),
    fImpl( new Ui::CMissingEpisodes )
{
    fImpl->setupUi( this );
    setupActions();

    connect( this, &CMissingEpisodes::sigModelDataChanged, this, &CMissingEpisodes::slotModelDataChanged );
    connect( this, &CMissingEpisodes::sigDataContextMenuRequested, this, &CMissingEpisodes::slotMediaContextMenu );

    connect( fImpl->selectAll, &QPushButton::clicked, this, &CMissingEpisodes::slotSelectAll );
    connect( fImpl->unselectAll, &QPushButton::clicked, this, &CMissingEpisodes::slotUnselectAll );
    connect( fImpl->showsFilter, &QTreeWidget::itemChanged, this, &CMissingEpisodes::slotSearchByShowNameChanged );
    fImpl->showsFilter->setItemDelegate( new CEditDelegate( this ) );


    fOrigFilter = loadShowFilter();
    slotSearchByShowNameChanged();
}

void CMissingEpisodes::slotSearchByShowNameChanged()
{
    if ( !fMissingMediaModel )
        return;

    auto selected = getSelectedShows();
    fMissingMediaModel->setShowFilter( selected );

    saveShowFilter();
}

std::list< std::shared_ptr< SShowFilter > > CMissingEpisodes::loadShowFilter() const
{
    std::list< std::shared_ptr< SShowFilter > > retVal;

    QSettings settings;
    settings.beginGroup( "MissingEpisodes" );
    int cnt = settings.beginReadArray( "Show" );
    for ( int ii = 0; ii < cnt; ++ii )
    {
        settings.setArrayIndex( ii );
        auto name = settings.value( "Name" ).toString();
        QVariant minSeason;
        if ( settings.contains( "MinSeason" ) )
            minSeason = settings.value( "MinSeason" );

        QVariant maxSeason;
        if ( settings.contains( "MinSeason" ) )
            maxSeason = settings.value( "MaxSeason" );

        auto enabled = settings.value( "Enabled", true ).toBool();

        retVal.push_back( std::make_shared< SShowFilter >( name, minSeason, maxSeason, enabled ) );
    }
    settings.endArray();
    settings.endGroup();

    return retVal;
}

void CMissingEpisodes::saveShowFilter()
{
    auto selected = getSelectedShows();
    if ( selected == fOrigFilter )
        return;

    QSettings settings;
    settings.beginGroup( "MissingEpisodes" );
    settings.beginWriteArray( "Show", static_cast< int >( selected.size() ) );
    int showNum = 0;
    for ( auto &&ii : selected )
    {
        settings.setArrayIndex( showNum++ );
        settings.setValue( "Name", ii->fSeriesName );
        settings.setValue( "MinSeason", ii->fMinSeason.has_value() ? ii->fMinSeason.value() : QVariant() );
        settings.setValue( "MaxSeason", ii->fMaxSeason.has_value() ? ii->fMaxSeason.value() : QVariant() );
        settings.setValue( "Enabled", ii->fEnabled );
    }
    settings.endArray();
    settings.endGroup();
    fOrigFilter = selected;
}

void CMissingEpisodes::slotSelectAll()
{
    fImpl->showsFilter->blockSignals( true );

    for ( auto ii = 0; ii < fImpl->showsFilter->topLevelItemCount(); ++ii )
    {
        auto curr = fImpl->showsFilter->topLevelItem( ii );
        curr->setCheckState( 0, Qt::CheckState::Checked );
    }

    fImpl->showsFilter->blockSignals( false );
    slotSearchByShowNameChanged();
}

void CMissingEpisodes::slotUnselectAll()
{
    fImpl->showsFilter->blockSignals( true );

    for ( auto ii = 0; ii < fImpl->showsFilter->topLevelItemCount(); ++ii )
    {
        auto curr = fImpl->showsFilter->topLevelItem( ii );
        curr->setCheckState( 0, Qt::CheckState::Unchecked );
    }

    fImpl->showsFilter->blockSignals( false );
    slotSearchByShowNameChanged();
}

CMissingEpisodes::~CMissingEpisodes()
{
    saveShowFilter();
}

void CMissingEpisodes::setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CCollectionsModel > collectionsModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem )
{
    CTabPageBase::setupPage( settings, syncSystem, mediaModel, collectionsModel, userModel, serverModel, progressSystem );

    fServerFilterModel = new CServerFilterModel( fServerModel.get() );
    fServerFilterModel->setSourceModel( fServerModel.get() );
    fServerFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CMissingEpisodes::sigModelDataChanged ) );

    fImpl->servers->setModel( fServerFilterModel );
    fImpl->servers->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->servers, &QTreeView::clicked, this, &CMissingEpisodes::slotCurrentServerChanged );

    slotMediaChanged();

    connect( fMediaModel.get(), &CMediaModel::sigMediaChanged, this, &CMissingEpisodes::slotMediaChanged );

    fMissingMediaModel = new CMediaMissingFilterModel( settings, fMediaModel.get() );
    fMissingMediaModel->setSourceModel( fMediaModel.get() );
    connect( fSyncSystem.get(), &CSyncSystem::sigMissingEpisodesLoaded, this, &CMissingEpisodes::slotMissingEpisodesLoaded );

    slotSetCurrentServer( QModelIndex() );
    showPrimaryServer();
}

void CMissingEpisodes::slotMediaChanged()
{
    if ( !fMissingMediaModel || !fMediaModel->hasMedia() )
        return;

    auto selectedShows = getSelectedShows();

    auto showNames = fMediaModel->getKnownShows();
    disconnect( fImpl->showsFilter, &QTreeWidget::itemChanged, this, &CMissingEpisodes::slotSearchByShowNameChanged );

    fImpl->showsFilter->clear();


    for ( auto &&name : showNames )
    {
        std::shared_ptr< SShowFilter > filterForShow;
        for ( auto &&jj : selectedShows )
        {
            if ( jj->fSeriesName == name )
            {
                filterForShow = jj;
                break;
            }
        }

        auto minSeason = ( filterForShow && filterForShow->fMinSeason.has_value() ) ? QString::number( filterForShow->fMinSeason.value() ) : QString();
        auto maxSeason = ( filterForShow && filterForShow->fMaxSeason.has_value() ) ? QString::number( filterForShow->fMaxSeason.value() ) : QString();
        bool enabled = filterForShow ? filterForShow->fEnabled : true;

        auto curr = new QTreeWidgetItem( fImpl->showsFilter, { name, minSeason, maxSeason } );
        curr->setFlags( curr->flags() | Qt::ItemIsEditable );
        curr->setCheckState( 0, enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
    }
    fImpl->showsFilter->resizeColumnToContents( 0 );
    connect( fImpl->showsFilter, &QTreeWidget::itemChanged, this, &CMissingEpisodes::slotSearchByShowNameChanged );
    slotSearchByShowNameChanged();
}

std::list< std::shared_ptr< SShowFilter > > CMissingEpisodes::getSelectedShows() const
{
    if ( !fImpl->showsFilter->topLevelItemCount() )
    {
        return fOrigFilter;
    }

    std::list< std::shared_ptr< SShowFilter > > retVal;
    for ( auto ii = 0; ii < fImpl->showsFilter->topLevelItemCount(); ++ii )
    {
        auto curr = fImpl->showsFilter->topLevelItem( ii );
        retVal.push_back( std::make_shared< SShowFilter >( curr->text( 0 ), curr->text( 1 ), curr->text( 2 ), curr->checkState( 0 ) == Qt::CheckState::Checked ) );
    }
    return retVal;
}

void CMissingEpisodes::setupActions()
{
    fActionSearchForAll = new QAction( this );
    fActionSearchForAll->setObjectName( QString::fromUtf8( "fActionSearchForAll" ) );
    setIcon( QString::fromUtf8( ":/SABUtilsResources/search.png" ), fActionSearchForAll );
    fActionSearchForAll->setText( QCoreApplication::translate( "CMissingEpisodes", "Search for All Missing", nullptr ) );
    fActionSearchForAll->setToolTip( QCoreApplication::translate( "CMissingEpisodes", "Search for All Missing", nullptr ) );

    fToolBar = new QToolBar( this );
    fToolBar->setObjectName( QString::fromUtf8( "fToolBar" ) );

    fToolBar->addAction( fActionSearchForAll );

    connect( fActionSearchForAll, &QAction::triggered, this, &CMissingEpisodes::slotSearchForAllMissing );
}

bool CMissingEpisodes::prepForClose()
{
    return true;
}

void CMissingEpisodes::loadSettings()
{
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
    showPrimaryServer();
}

void CMissingEpisodes::showPrimaryServer()
{
    NSABUtils::CAutoWaitCursor awc;
    fServerFilterModel->setOnlyShowEnabledServers( true );
    fServerFilterModel->setOnlyShowPrimaryServer( true );
}

void CMissingEpisodes::slotModelDataChanged()
{
}

void CMissingEpisodes::loadingUsersFinished()
{
}

QSplitter *CMissingEpisodes::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CMissingEpisodes::slotCurrentServerChanged( const QModelIndex &index )
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

    if ( !fSyncSystem->loadMissingEpisodes( serverInfo ) )
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

void CMissingEpisodes::createServerTrees( QAbstractItemModel *model )
{
    addDataTreeForServer( nullptr, model );
}

void CMissingEpisodes::slotSetCurrentServer( const QModelIndex &current )
{
    auto serverInfo = getServerInfo( current );
    slotModelDataChanged();
}

void CMissingEpisodes::slotMissingEpisodesLoaded()
{
    auto currServer = getCurrentServerInfo();
    if ( !currServer )
        return;

    hideDataTreeColumns();
    sortDataTrees();
}

std::shared_ptr< CMediaData > CMissingEpisodes::getMediaData( QModelIndex idx ) const
{
    if ( idx.model() != fMediaModel.get() )
        idx = fMissingMediaModel->mapToSource( idx );

    auto retVal = fMediaModel->getMediaData( idx );
    return retVal;
}

void CMissingEpisodes::slotMediaContextMenu( CDataTree *dataTree, const QPoint &pos )
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
    mediaData->addSearchMenu( fSettings, &menu );
    menu.exec( dataTree->dataTree()->mapToGlobal( pos ) );
}

void CMissingEpisodes::slotSearchForAllMissing()
{
    bulkSearch(
        [ this ]( const QModelIndex &index )
        {
            auto premiereDate = index.data( CMediaModel::ECustomRoles::ePremiereDateRole ).toDate();
            if ( premiereDate > QDate::currentDate() )
            {
                return std::make_pair( false, QUrl() );
            }

            auto mediaData = getMediaData( index );
            if ( !mediaData )
            {
                return std::make_pair( false, QUrl() );
            }
            return std::make_pair( true, mediaData->getDefaultSearchURL( fSettings ) );
        } );
}
