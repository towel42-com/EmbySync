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

#include "MissingMovies.h"
#include "ui_MissingMovies.h"

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
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>

CMissingMovies::CMissingMovies(QWidget* parent)
    : CTabPageBase(parent),
    fImpl(new Ui::CMissingMovies)
{
    fImpl->setupUi(this);
    setupActions();

    connect(this, &CMissingMovies::sigModelDataChanged, this, &CMissingMovies::slotModelDataChanged);
    connect(this, &CMissingMovies::sigDataContextMenuRequested, this, &CMissingMovies::slotMediaContextMenu);

    connect(fImpl->listFileBtn, &QToolButton::clicked,
        [this]()
        {
            auto fileName = QFileDialog::getOpenFileName(this, QObject::tr("Select File"), QString(), QObject::tr("Movie List File (*.json);;All Files (* *.*)"));
            if (fileName.isEmpty())
                return;
            fImpl->listFile->setText(QFileInfo(fileName).absoluteFilePath());
        });
    connect(fImpl->listFile, &NSABUtils::CPathBasedDelayLineEdit::sigTextEditedAfterDelay, this, &CMissingMovies::slotLoadFile);
    connect(fImpl->listFile, &NSABUtils::CPathBasedDelayLineEdit::sigFinishedEditingAfterDelay, this, &CMissingMovies::slotLoadFile);
    connect(fImpl->listFile, &NSABUtils::CPathBasedDelayLineEdit::sigTextChangedAfterDelay, this, &CMissingMovies::slotLoadFile);

    QSettings settings;
    settings.beginGroup("MissingMovies");
    fImpl->listFile->setText(settings.value("MovieListFile", QString()).toString());

}

CMissingMovies::~CMissingMovies()
{
    QSettings settings;
    settings.beginGroup("MissingMovies");
    settings.setValue("MovieListFile", fImpl->listFile->text());
}

void CMissingMovies::setupPage(std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CCollectionsModel > collectionsModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem)
{
    CTabPageBase::setupPage(settings, syncSystem, mediaModel, collectionsModel, userModel, serverModel, progressSystem);

    fServerFilterModel = new CServerFilterModel(fServerModel.get());
    fServerFilterModel->setSourceModel(fServerModel.get());
    fServerFilterModel->sort(0, Qt::SortOrder::AscendingOrder);
    NSABUtils::setupModelChanged(fMediaModel.get(), this, QMetaMethod::fromSignal(&CMissingMovies::sigModelDataChanged));

    fImpl->servers->setModel(fServerFilterModel);
    fImpl->servers->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(fImpl->servers, &QTreeView::clicked, this, &CMissingMovies::slotCurrentServerChanged);

    slotMediaChanged();

    connect(fMediaModel.get(), &CMediaModel::sigMediaChanged, this, &CMissingMovies::slotMediaChanged);

    fMoviesModel = new CMovieSearchFilterModel(settings, fMediaModel.get());
    fMoviesModel->setSourceModel(fMediaModel.get());
    //connect( fMediaModel.get(), &CMediaModel::sigPendingMediaUpdate, this, &CPlayStateCompare::slotPendingMediaUpdate );
    connect(fSyncSystem.get(), &CSyncSystem::sigAllMoviesLoaded, this, &CMissingMovies::slotAllMoviesLoaded);

    slotSetCurrentServer(QModelIndex());
    showPrimaryServer();
}

void CMissingMovies::slotMediaChanged()
{
}

void CMissingMovies::setupActions()
{
    fActionSearchForAll = new QAction(this);
    fActionSearchForAll->setObjectName(QString::fromUtf8("fActionSearchForAll"));
    QIcon icon3;
    icon3.addFile(QString::fromUtf8(":/SABUtilsResources/search.png"), QSize(), QIcon::Normal, QIcon::Off);
    Q_ASSERT(!icon3.isNull());
    fActionSearchForAll->setIcon(icon3);
    fActionSearchForAll->setText(QCoreApplication::translate("CMissingMovies", "Search for All Missing", nullptr));
    fActionSearchForAll->setToolTip(QCoreApplication::translate("CMissingMovies", "Search for All Missing", nullptr));

    fToolBar = new QToolBar(this);
    fToolBar->setObjectName(QString::fromUtf8("fToolBar"));

    fToolBar->addAction(fActionSearchForAll);

    connect(fActionSearchForAll, &QAction::triggered, this, &CMissingMovies::slotSearchForAllMissing);
}

bool CMissingMovies::prepForClose()
{
    return true;
}

void CMissingMovies::loadSettings()
{
}

bool CMissingMovies::okToClose()
{
    bool okToClose = true;
    return okToClose;
}

void CMissingMovies::reset()
{
    resetPage();
}

void CMissingMovies::resetPage()
{
}

void CMissingMovies::slotCanceled()
{
    fSyncSystem->slotCanceled();
}

void CMissingMovies::slotSettingsChanged()
{
    loadServers();
    showPrimaryServer();
}

void CMissingMovies::showPrimaryServer()
{
    NSABUtils::CAutoWaitCursor awc;
    fServerFilterModel->setOnlyShowEnabledServers(true);
    fServerFilterModel->setOnlyShowPrimaryServer(true);
}

void CMissingMovies::slotModelDataChanged()
{
    fImpl->summaryLabel->setText(fMoviesModel->summary());
}

void CMissingMovies::loadingUsersFinished()
{
}

QSplitter* CMissingMovies::getDataSplitter() const
{
    return fImpl->dataSplitter;
}

void CMissingMovies::slotCurrentServerChanged(const QModelIndex& index)
{
    if (fSyncSystem->isRunning())
        return;

    auto idx = index;
    if (!idx.isValid())
        idx = fImpl->servers->selectionModel()->currentIndex();

    if (!index.isValid())
        return;

    auto serverInfo = getServerInfo(idx);
    if (!serverInfo)
        return;

    if (!fDataTrees.empty())
    {
        fDataTrees[0]->setServer(serverInfo, true);
    }

    fMediaModel->clear();
    if (!fSyncSystem->loadAllMovies(serverInfo))
    {
        QMessageBox::critical(this, tr("No Admin User Found"), tr("No user found with Administrator Privileges on server '%1'").arg(serverInfo->displayName()));
    }
    if (!fImpl->listFile->text().isEmpty())
        setMovieSearchFile(fImpl->listFile->text(), true);
    else if (!fFileName.isEmpty())
        setMovieSearchFile(fFileName, true);
}


std::shared_ptr< CServerInfo > CMissingMovies::getCurrentServerInfo() const
{
    auto idx = fImpl->servers->selectionModel()->currentIndex();
    if (!idx.isValid())
        return {};

    return getServerInfo(idx);
}

std::shared_ptr< CTabUIInfo > CMissingMovies::getUIInfo() const
{
    auto retVal = std::make_shared< CTabUIInfo >();

    retVal->fMenus = {};
    retVal->fToolBars = { fToolBar };

    return retVal;
}

std::shared_ptr< CServerInfo > CMissingMovies::getServerInfo(QModelIndex idx) const
{
    if (idx.model() != fServerModel.get())
        idx = fServerFilterModel->mapToSource(idx);

    auto retVal = fServerModel->getServerInfo(idx);
    return retVal;
}

void CMissingMovies::loadServers()
{
    CTabPageBase::loadServers(fMoviesModel);
}

void CMissingMovies::createServerTrees(QAbstractItemModel* model)
{
    addDataTreeForServer(nullptr, model);
}

void CMissingMovies::slotSetCurrentServer(const QModelIndex& current)
{
    auto serverInfo = getServerInfo(current);
    slotModelDataChanged();
}

void CMissingMovies::slotAllMoviesLoaded()
{
    auto currServer = getCurrentServerInfo();
    if (!currServer)
        return;

    hideDataTreeColumns();
    sortDataTrees();
    fMoviesModel->addMissingMoviesToSourceModel();
}

std::shared_ptr< CMediaData > CMissingMovies::getMediaData(QModelIndex idx) const
{
    if (idx.model() != fMediaModel.get())
        idx = fMoviesModel->mapToSource(idx);

    auto retVal = fMediaModel->getMediaData(idx);
    return retVal;
}

void CMissingMovies::slotMediaContextMenu(CDataTree* dataTree, const QPoint& pos)
{
    if (!dataTree)
        return;

    auto idx = dataTree->indexAt(pos);
    if (!idx.isValid())
        return;

    auto mediaData = getMediaData(idx);
    if (!mediaData)
        return;

    QMenu menu(tr("Context Menu"));

    auto action = new QAction("Search for Torrent on RARBG", &menu);
    menu.addAction(action);
    connect(action, &QAction::triggered,
        [mediaData]()
        {
            auto url = mediaData->getSearchURL(CMediaData::ETorrentSite::eRARBG);
            QDesktopServices::openUrl(url);
        });

    action = new QAction("Search for Torrent on piratebay.org", &menu);
    menu.addAction(action);
    connect(action, &QAction::triggered,
        [mediaData]()
        {
            auto url = mediaData->getSearchURL(CMediaData::ETorrentSite::ePirateBay);
            QDesktopServices::openUrl(url);
        });

    menu.exec(dataTree->dataTree()->mapToGlobal(pos));
}

void CMissingMovies::slotLoadFile(const QString& fileName)
{
    setMovieSearchFile(fileName, true);
}

void CMissingMovies::setMovieSearchFile(const QString& fileName, bool force)
{
    if (!fMediaModel)
        return;

    if (fileName.isEmpty())
        return;

    if (!force && (QFileInfo(fileName) == QFileInfo(fFileName)))
        return;

    fFileName.clear();
    QFile fi(fileName);
    if (!fi.open(QFile::ReadOnly))
    {
        return;
    }
    fFileName = fileName;

    auto data = fi.readAll();
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
    {
        QMessageBox::critical(this, tr("Error Reading File"), tr("Error: %1 @ %2").arg(error.errorString()).arg(error.offset));
        return;
    }
    if (!doc.isObject())
    {
        QMessageBox::critical(this, tr("Error Reading File"), tr("Error: Top level item should be object"));
        return;
    }
    auto root = doc.object();
    auto moviesObj = root["movies"];
    if (!moviesObj.isArray())
    {
        QMessageBox::critical(this, tr("Error Reading File"), tr("Error: Top level item should contain an array called movies"));
        return;
    }
    auto movies = moviesObj.toArray();
    for (auto&& movie : movies)
    {
        auto name = movie.toObject()["name"].toString();
        auto year = movie.toObject()["year"].toInt();
        fMoviesModel->addSearchMovie(name, year, false);
    }
    fMoviesModel->finishedAddingSearchMovies();
}

void CMissingMovies::slotSearchForAllMissing()
{
    bulkSearch(
        [this](const QModelIndex& index)
        {
            auto onServer = index.data(CMediaModel::ECustomRoles::eOnServerRole).toBool();
            if (onServer)
            {
                return std::make_pair(false, QUrl());
            }

            auto mediaData = getMediaData(index);
            if (!mediaData)
            {
                return std::make_pair(false, QUrl());
            }
            return std::make_pair(true, mediaData->getSearchURL(CMediaData::ETorrentSite::eRARBG));
        });
}


