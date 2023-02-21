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

#include "CreateCollections.h"
#include "ui_CreateCollections.h"

#include "DataTree.h"
#include "MediaWindow.h"
#include "TabUIInfo.h"

#include <QFileDialog>

#include "Core/MediaModel.h"
#include "Core/MediaData.h"
#include "Core/ProgressSystem.h"
#include "Core/ServerInfo.h"
#include "Core/Settings.h"
#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/ServerModel.h"
#include <QAbstractItemModelTester>

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
#include <QInputDialog>
#include <QTimer>
#include <QToolBar>
#include <QSettings>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

CCreateCollections::CCreateCollections( QWidget * parent )
    : CTabPageBase( parent ),
    fImpl( new Ui::CCreateCollections )
{
    fImpl->setupUi( this );
    setupActions();

    connect( this, &CCreateCollections::sigModelDataChanged, this, &CCreateCollections::slotModelDataChanged );
    connect( this, &CCreateCollections::sigDataContextMenuRequested, this, &CCreateCollections::slotMediaContextMenu );

    connect( fImpl->listFileBtn, &QToolButton::clicked,
             [this]()
             {
                 auto fileName = QFileDialog::getOpenFileName( this, QObject::tr( "Select File" ), QString(), QObject::tr( "Movie List File (*.json);;All Files (* *.*)" ) );
                 if ( fileName.isEmpty() )
                     return;
                 fImpl->listFile->setText( QFileInfo( fileName ).absoluteFilePath() );
             } );
    connect( fImpl->listFile, &NSABUtils::CPathBasedDelayLineEdit::sigTextEditedAfterDelay, this, &CCreateCollections::slotLoadFile );
    connect( fImpl->listFile, &NSABUtils::CPathBasedDelayLineEdit::sigFinishedEditingAfterDelay, this, &CCreateCollections::slotLoadFile );
    connect( fImpl->listFile, &NSABUtils::CPathBasedDelayLineEdit::sigTextChangedAfterDelay, this, &CCreateCollections::slotLoadFile );

    QSettings settings;
    settings.beginGroup( "CreateCollections" );
    fImpl->listFile->setText( settings.value( "CollectionsListFile", QString() ).toString() );

}

CCreateCollections::~CCreateCollections()
{
    QSettings settings;
    settings.beginGroup( "CreateCollections" );
    settings.setValue( "CollectionsListFile", fImpl->listFile->text() );
}

void CCreateCollections::setupPage( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem )
{
    CTabPageBase::setupPage( settings, syncSystem, mediaModel, userModel, serverModel, progressSystem );

    fServerFilterModel = new CServerFilterModel( fServerModel.get() );
    fServerFilterModel->setSourceModel( fServerModel.get() );
    fServerFilterModel->sort( 0, Qt::SortOrder::AscendingOrder );
    NSABUtils::setupModelChanged( fMediaModel.get(), this, QMetaMethod::fromSignal( &CCreateCollections::sigModelDataChanged ) );

    fImpl->servers->setModel( fServerFilterModel );
    fImpl->servers->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->servers, &QTreeView::clicked, this, &CCreateCollections::slotCurrentServerChanged );

    slotMediaChanged();

    connect( fMediaModel.get(), &CMediaModel::sigMediaChanged, this, &CCreateCollections::slotMediaChanged );

    fCollections = new CCollectionsModel( fMediaModel );
    connect( this, &CCreateCollections::sigModelDataChanged, fCollections, &CCollectionsModel::slotMediaModelDataChanged );

    //new QAbstractItemModelTester( fCollections, QAbstractItemModelTester::FailureReportingMode::Fatal, this );
    //fMoviesModel->setSourceModel( fMediaModel.get() );
    //connect( fMediaModel.get(), &CMediaModel::sigPendingMediaUpdate, this, &CCreateCollections::slotPendingMediaUpdate );
    connect( fSyncSystem.get(), &CSyncSystem::sigAllMoviesLoaded, this, &CCreateCollections::slotAllMoviesLoaded );
    connect( fSyncSystem.get(), &CSyncSystem::sigAllCollectionsLoaded, this, &CCreateCollections::slotAllCollectionsLoaded );
    

    slotSetCurrentServer( QModelIndex() );
    showPrimaryServer();
}

void CCreateCollections::slotMediaChanged()
{
}

void CCreateCollections::setupActions()
{
    fCreateCollections = new QAction( this );
    fCreateCollections->setObjectName( QString::fromUtf8( "fActionProcess" ) );
    QIcon icon3;
    icon3.addFile( QString::fromUtf8( ":/SABUtilsResources/run.png" ), QSize(), QIcon::Normal, QIcon::Off );
    Q_ASSERT( !icon3.isNull() );
    fCreateCollections->setIcon( icon3 );
    fCreateCollections->setText( QCoreApplication::translate( "CCreateCollections", "Create Missing Collections", nullptr ) );
    fCreateCollections->setToolTip( QCoreApplication::translate( "CCreateCollections", "Create Missing Collections", nullptr ) );

    fToolBar = new QToolBar( this );
    fToolBar->setObjectName( QString::fromUtf8( "fToolBar" ) );

    fToolBar->addAction( fCreateCollections );

    connect( fCreateCollections, &QAction::triggered, this, &CCreateCollections::slotCreateMissingCollections );
}

bool CCreateCollections::prepForClose()
{
    return true;
}

void CCreateCollections::loadSettings()
{
}

bool CCreateCollections::okToClose()
{
    bool okToClose = true;
    return okToClose;
}

void CCreateCollections::reset()
{
    resetPage();
}

void CCreateCollections::resetPage()
{
}

void CCreateCollections::slotCanceled()
{
    fSyncSystem->slotCanceled();
}

void CCreateCollections::slotSettingsChanged()
{
    loadServers();
    showPrimaryServer();
}

void CCreateCollections::showPrimaryServer()
{
    NSABUtils::CAutoWaitCursor awc;
    fServerFilterModel->setOnlyShowEnabledServers( true );
    fServerFilterModel->setOnlyShowPrimaryServer( true );
}

void CCreateCollections::slotModelDataChanged()
{
    fImpl->summaryLabel->setText( fCollections->summary() );
}

void CCreateCollections::loadingUsersFinished()
{
}

QSplitter * CCreateCollections::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CCreateCollections::slotCurrentServerChanged( const QModelIndex & index )
{
    if ( fSyncSystem->isRunning() )
        return;

    auto serverInfo = getCurrentServerInfo( index );
    if ( !serverInfo )
        return;

    if ( !fDataTrees.empty() )
    {
        fDataTrees[ 0 ]->setServer( serverInfo, true );
    }

    fMediaModel->clear();
    if ( !fSyncSystem->loadAllMovies( serverInfo ) )
    {
        QMessageBox::critical( this, tr( "No Admin User Found" ), tr( "No user found with Administrator Privileges on server '%1'" ).arg( serverInfo->displayName() ) );
    }

    if ( !fImpl->listFile->text().isEmpty() )
        setCollectionsFile( fImpl->listFile->text(), true );
    else if ( !fFileName.isEmpty() )
        setCollectionsFile( fFileName, true );
}


std::shared_ptr< CServerInfo > CCreateCollections::getCurrentServerInfo( const QModelIndex & index ) const
{
    auto idx = index;
    if ( !idx.isValid() )
        idx = fImpl->servers->selectionModel()->currentIndex();
    if ( !idx.isValid() )
        return {};

    return getServerInfo( idx );
}

std::shared_ptr< CTabUIInfo > CCreateCollections::getUIInfo() const
{
    auto retVal = std::make_shared< CTabUIInfo >();

    retVal->fMenus = {};
    retVal->fToolBars = { fToolBar };

    return retVal;
}

std::shared_ptr< CServerInfo > CCreateCollections::getServerInfo( QModelIndex idx ) const
{
    if ( idx.model() != fServerModel.get() )
        idx = fServerFilterModel->mapToSource( idx );

    auto retVal = fServerModel->getServerInfo( idx );
    return retVal;
}

void CCreateCollections::loadServers()
{
    CTabPageBase::loadServers( fCollections );
}

void CCreateCollections::createServerTrees( QAbstractItemModel * model )
{
    auto dataTree = addDataTreeForServer( nullptr, model );
    dataTree->dataTree()->setRootIsDecorated( true );
    dataTree->dataTree()->setItemsExpandable( true );
    dataTree->dataTree()->setIndentation( 20 );
}

void CCreateCollections::slotSetCurrentServer( const QModelIndex & current )
{
    auto serverInfo = getServerInfo( current );
    slotModelDataChanged();
}

void CCreateCollections::slotAllMoviesLoaded()
{
    hideDataTreeColumns();
    sortDataTrees();

    auto serverInfo = getCurrentServerInfo();
    if ( !serverInfo )
        return;

    if ( !fSyncSystem->loadAllCollections( serverInfo ) )
    {
        QMessageBox::critical( this, tr( "No Admin User Found" ), tr( "No user found with Administrator Privileges on server '%1'" ).arg( serverInfo->displayName() ) );
    }
}

void CCreateCollections::slotAllCollectionsLoaded()
{
    auto serverInfo = getCurrentServerInfo();
    if ( !serverInfo )
        return;

    fCollections->updateCollections( serverInfo->keyName(), fMediaModel );
}

std::shared_ptr< CMediaData > CCreateCollections::getMediaData( QModelIndex idx ) const
{
    auto retVal = fMediaModel->getMediaData( idx );
    return retVal;
}

void CCreateCollections::slotMediaContextMenu( CDataTree * dataTree, const QPoint & pos )
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

    auto action = new QAction( "Search for Torrent on RARBG", &menu );
    menu.addAction( action );
    connect( action, &QAction::triggered,
             [mediaData]()
             {
                 auto url = mediaData->getSearchURL( CMediaData::ETorrentSite::eRARBG );
                 QDesktopServices::openUrl( url );
             } );

    action = new QAction( "Search for Torrent on piratebay.org", &menu );
    menu.addAction( action );
    connect( action, &QAction::triggered,
             [ mediaData ]()
             {
                 auto url = mediaData->getSearchURL( CMediaData::ETorrentSite::ePirateBay );
                 QDesktopServices::openUrl( url );
             } );

    menu.exec( dataTree->dataTree()->mapToGlobal( pos ) );
}

void CCreateCollections::slotLoadFile( const QString & fileName )
{
    setCollectionsFile( fileName, false );
}

void CCreateCollections::setCollectionsFile( const QString & fileName, bool force )
{
    if ( !fMediaModel )
        return;

    if ( fileName.isEmpty() )
        return;

    if ( !force && ( QFileInfo( fileName ) == QFileInfo( fFileName ) ) )
        return;

    auto serverInfo = getCurrentServerInfo();
    if (!serverInfo)
        return;

    fFileName.clear();
    QFile fi( fileName );
    if ( !fi.open( QFile::ReadOnly ) )
    {
        return;
    }
    fFileName = fileName;

    auto data = fi.readAll();
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson( data, &error );
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical( this, tr( "Error Reading File" ), tr( "Error: %1 @ %2" ).arg( error.errorString() ).arg( error.offset ) );
        return;
    }

    if (!doc.isObject())
    {
        QMessageBox::critical(this, tr("Error Reading File"), tr("Error: Top level item should be object"));
        return;
    }

    auto root = doc.object();
    auto collectionsObj = root["collections"];
    auto moviesObj = root["movies"];
    if (!collectionsObj.isArray() && !moviesObj.isArray())
    {
        QMessageBox::critical(this, tr("Error Reading File"), tr("Error: Top level item should contain an array called movies or collections"));
        return;
    }

    if (moviesObj.isArray())
    {
        QJsonObject collectionObj;
        collectionObj["collection"] = "<Unnamed Collection>";
        collectionObj["movies"] = moviesObj;
        auto tmp = QJsonArray();
        tmp.push_back( collectionObj );
        collectionsObj = tmp;
    }
    QJsonArray collections;
    if ( !collectionsObj.isArray() )
    {
        return;
    }

    fCollections->clear();
    collections = collectionsObj.toArray();
    for (auto&& curr : collections)
    {
        auto collectionName = curr.toObject()["collection"].toString();
        auto movies = curr.toObject()["movies"].toArray();
        QModelIndex idx;
        std::shared_ptr< CMediaCollection > collection;
        std::tie(idx, collection) = fCollections->addCollection(serverInfo->keyName(), collectionName);
        Q_ASSERT(collection.get() == fCollections->collection(idx));
        for (auto&& movie : movies)
        {
            auto rank = movie.toObject()["rank"].toInt();
            if (!movie.toObject().contains("rank"))
                rank = -1;
            auto movieName = movie.toObject()["name"].toString();
            auto year = movie.toObject()["year"].toInt();
            auto currCollection = fCollections->addMovie( movieName, year, idx, rank);
            if (currCollection->fCollection && currCollection->fCollection->isUnNamed() )
            {
                currCollection->fCollection->setFileName(fFileName);
            }
        }
    }
}

void CCreateCollections::slotCreateMissingCollections()
{
    auto serverInfo = getCurrentServerInfo();
    if ( !serverInfo )
        return;
    fCollections->createCollections( serverInfo, fSyncSystem, this );
}


