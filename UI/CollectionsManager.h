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

#ifndef _COLLECTIONSMANAGER_H
#define _COLLECTIONSMANAGER_H

#include "TabPageBase.h"
#include <memory>
#include <QPointer>
#include <list>
#include <QUrl>
#include <QModelIndex>
class QMenu;
class QAction;
class QToolBar;
class CCollectionsFilterModel;
namespace Ui
{
    class CCollectionsManager;
}

class CMediaData;
class CUserData;
class CSettings;
class CSyncSystem;
class CUsersModel;
class CCollectionsManager;
class CDataTree;
class CProgressSystem;
class CTabUIInfo;
class CServerFilterModel;

class CCollectionsManager : public CTabPageBase
{
    Q_OBJECT
public:
    CCollectionsManager( QWidget *parent = nullptr );

    virtual ~CCollectionsManager() override;

    virtual void setupPage(
        std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CMediaModel > mediaModel, std::shared_ptr< CCollectionsModel > collectionsModel, std::shared_ptr< CUsersModel > userModel,
        std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CProgressSystem > progressSystem ) override;

    virtual void setupActions();

    virtual bool okToClose() override;

    virtual void loadSettings() override;
    virtual void resetPage() override;

    virtual std::shared_ptr< CTabUIInfo > getUIInfo() const override;

    virtual void loadingUsersFinished() override;

    virtual QSplitter *getDataSplitter() const override;

    virtual bool prepForClose() override;
    std::shared_ptr< CMediaData > getMediaData( QModelIndex idx ) const;

    virtual int defaultSortColumn() const override { return 0; }
    virtual Qt::SortOrder defaultSortOrder() const override { return Qt::SortOrder::AscendingOrder; }

Q_SIGNALS:
    void sigModelDataChanged();

public Q_SLOTS:
    virtual void slotCanceled() override;
    virtual void slotModelDataChanged() override;
    virtual void slotSettingsChanged() override;
    void slotMediaContextMenu( CDataTree *dataTree, const QPoint &pos );
    virtual void slotSetCurrentServer( const QModelIndex &index );
    void slotCreateMissingCollections();
private Q_SLOTS:
    void slotCurrentServerChanged( const QModelIndex &index );
    void slotAllMoviesLoaded();
    void slotAllCollectionsLoaded();
    void slotMediaChanged();
    void slotLoadFile( const QString &fileName );

private:
    void setCollectionsFile( const QString &fileName, bool force );

    void showPrimaryServer();
    std::shared_ptr< CServerInfo > getCurrentServerInfo( const QModelIndex &index = {} ) const;
    std::shared_ptr< CServerInfo > getServerInfo( QModelIndex idx ) const;

    void reset();

    void loadServers();
    virtual void createServerTrees( QAbstractItemModel *model ) override;

    std::unique_ptr< Ui::CCollectionsManager > fImpl;

    QPointer< QAction > fLoadCollections;
    QPointer< QAction > fCreateCollections;
    QPointer< QToolBar > fToolBar{ nullptr };

    CCollectionsFilterModel *fFilterModel{ nullptr };
    CServerFilterModel *fServerFilterModel{ nullptr };
};
#endif
