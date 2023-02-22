// The MIT License( MIT )
//
// Copyright( c ) 2022 Scott Aron Bloom
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
// copies of the Software, and to permit persons to whom the Software is
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

#ifndef _MISSINGMOVIES_H
#define _MISSINGMOVIES_H

#include "TabPageBase.h"
#include <memory>
#include <QPointer>
#include <list>
#include <QUrl>
class QMenu;
class QAction;
class QToolBar;
namespace Ui
{
    class CMissingMovies;
}

class CMediaData;
class CUserData;
class CSettings;
class CSyncSystem;
class CUsersModel;
class CDataTree;
class CProgressSystem;
class CTabUIInfo;
class CServerFilterModel;
class CMovieSearchFilterModel;

class CMissingMovies : public CTabPageBase
{
    Q_OBJECT
public:
    CMissingMovies(QWidget* parent = nullptr);

    virtual ~CMissingMovies() override;

    virtual void setupPage(std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CCollectionsModel > collectionsModel, std::shared_ptr< CUsersModel > userModel, std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem) override;

    virtual void setupActions();

    virtual bool okToClose() override;

    virtual void loadSettings() override;
    virtual void resetPage() override;

    virtual std::shared_ptr< CTabUIInfo > getUIInfo() const override;

    virtual void loadingUsersFinished() override;

    virtual QSplitter* getDataSplitter() const override;

    virtual bool prepForClose() override;
    std::shared_ptr< CMediaData > getMediaData(QModelIndex idx) const;

    virtual int defaultSortColumn() const override { return 1; }
    virtual Qt::SortOrder defaultSortOrder() const override { return Qt::SortOrder::AscendingOrder; }

Q_SIGNALS:
    void sigModelDataChanged();

public Q_SLOTS:
    virtual void slotCanceled() override;
    virtual void slotModelDataChanged() override;
    virtual void slotSettingsChanged() override;
    void slotMediaContextMenu(CDataTree* dataTree, const QPoint& pos);
    virtual void slotSetCurrentServer(const QModelIndex& index);
    void slotSearchForAllMissing();
private Q_SLOTS:
    void slotCurrentServerChanged(const QModelIndex& index);
    void slotAllMoviesLoaded();
    void slotMediaChanged();
    void slotLoadFile(const QString& fileName);
private:
    QString fFileName;
    void setMovieSearchFile(const QString& fileName, bool force);

    void showPrimaryServer();
    std::shared_ptr< CServerInfo > getCurrentServerInfo() const;
    std::shared_ptr< CServerInfo > getServerInfo(QModelIndex idx) const;

    void reset();

    void loadServers();
    virtual void createServerTrees(QAbstractItemModel* model) override;

    std::unique_ptr< Ui::CMissingMovies > fImpl;

    QPointer< QAction > fActionSearchForAll;
    QPointer< QToolBar > fToolBar{ nullptr };

    CServerFilterModel* fServerFilterModel{ nullptr };
    CMovieSearchFilterModel* fMoviesModel{ nullptr };
};
#endif
