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

#include "TabPageBase.h"
#include "DataTree.h"
#include "Core/SyncSystem.h"
#include "Core/Settings.h"
#include "Core/ServerInfo.h"
#include "Core/ServerModel.h"

#include <QSplitter>
#include <QModelIndex>
#include <QInputDialog>
#include <QDesktopServices>
#include <QTimer>
#include <QAction>

CTabPageBase::CTabPageBase( QWidget *parent ) :
    QWidget( parent )
{
}

CTabPageBase::~CTabPageBase()
{
}

QModelIndex CTabPageBase::currentDataIndex() const
{
    if ( fDataTrees.empty() )
        return {};
    return fDataTrees.front()->currentIndex();
}

std::pair< QModelIndex, QWidget * > CTabPageBase::dataIndexAt( const QPoint &pos ) const
{
    for ( auto &&ii : fDataTrees )
    {
        auto idx = ii->indexAt( pos );
        if ( idx.isValid() )
        {
            return { idx, ii };
        }
    }
    return {};
}

void CTabPageBase::autoSizeDataTrees()
{
    for ( auto &&ii : fDataTrees )
        ii->autoSize();
}

void CTabPageBase::hideDataTreeColumns()
{
    for ( auto &&ii : fDataTrees )
        ii->hideColumns();
}

void CTabPageBase::sortDataTrees()
{
    for ( auto &&ii : fDataTrees )
        ii->sort( defaultSortColumn(), defaultSortOrder() );
}

QString CTabPageBase::selectServer() const
{
    QStringList serverNames;
    std::map< QString, QString > servers;
    for ( auto &&serverInfo : *fServerModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;

        auto name = serverInfo->displayName();
        auto key = serverInfo->keyName();

        servers[ name ] = key;
        serverNames << name;
    }

    if ( serverNames.isEmpty() )
        return QString();

    bool aOK = false;
    auto whichServer = QInputDialog::getItem( const_cast< CTabPageBase * >( this ), tr( "Select Source Server" ), tr( "Source Server:" ), serverNames, 0, false, &aOK );
    if ( !aOK || whichServer.isEmpty() )
        return QString();
    auto pos = servers.find( whichServer );
    if ( pos == servers.end() )
        return QString();
    return ( *pos ).second;
}

void CTabPageBase::setIcon( const QString &path, QAction *action )
{
    QIcon icon;
    icon.addFile( path, QSize(), QIcon::Normal, QIcon::Off );
    Q_ASSERT( !icon.isNull() );
    action->setIcon( icon );
}

void CTabPageBase::bulkSearch( std::function< std::pair< bool, QUrl >( const QModelIndex &idx ) > addItemFunc )
{
    fBulkSearchURLs.clear();
    for ( auto &&currTree : fDataTrees )
    {
        auto treeModel = currTree->model();

        auto rowCount = treeModel->rowCount();
        for ( int ii = 0; ii < rowCount; ++ii )
        {
            auto index = treeModel->index( ii, 0 );
            auto text = index.data().toString();
            auto curr = addItemFunc( index );
            if ( curr.first )
            {
                fBulkSearchURLs.emplace_back( curr.second );
            }
        }
    }
    //while ( fBulkSearchURLs.size() > 10 )
    //    fBulkSearchURLs.pop_back();

    slotNextSearchURL();
}

void CTabPageBase::slotNextSearchURL()
{
    if ( fBulkSearchURLs.empty() )
        return;

    auto url = fBulkSearchURLs.front();
    fBulkSearchURLs.pop_front();
    QDesktopServices::openUrl( url );

    QTimer::singleShot( 500, this, &CTabPageBase::slotNextSearchURL );
}

void CTabPageBase::loadServers( QAbstractItemModel *model )
{
    clearServers();
    createServerTrees( model );
    setupDataTreePeers();
}

void CTabPageBase::setupDataTreePeers()
{
    for ( size_t ii = 0; ii < fDataTrees.size(); ++ii )
    {
        for ( size_t jj = 0; jj < fDataTrees.size(); ++jj )
        {
            if ( ii == jj )
                continue;
            fDataTrees[ ii ]->addPeerDataTree( fDataTrees[ jj ] );
        }
    }
}

void CTabPageBase::createServerTrees( QAbstractItemModel *model )
{
    for ( auto &&serverInfo : *fServerModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;

        addDataTreeForServer( serverInfo, model );
    }
}

CDataTree *CTabPageBase::addDataTreeForServer( std::shared_ptr< const CServerInfo > server, QAbstractItemModel *model )
{
    auto dataTree = new CDataTree( server, getDataSplitter() );

    getDataSplitter()->addWidget( dataTree );
    dataTree->setModel( model );
    dataTree->sort( defaultSortColumn(), defaultSortOrder() );
    connect( dataTree, &CDataTree::sigCurrChanged, this, &CTabPageBase::sigSetCurrentDataItem );
    connect( dataTree, &CDataTree::sigViewData, this, &CTabPageBase::sigViewData );
    connect( dataTree, &CDataTree::sigDataContextMenuRequested, this, &CTabPageBase::sigDataContextMenuRequested );
    connect( dataTree, &CDataTree::sigItemDoubleClicked, this, &CTabPageBase::sigItemDoubleClicked );

    fDataTrees.push_back( dataTree );
    return dataTree;
}

void CTabPageBase::clearServers()
{
    for ( auto &&ii : fDataTrees )
        delete ii;
    fDataTrees.clear();
}
