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

#include "DataTree.h"
#include "ui_DataTree.h"
#include "SABUtils/QtUtils.h"
#include "Core/Settings.h"
#include "Core/ServerInfo.h"
#include "Core/IServerForColumn.h"

#include <QSortFilterProxyModel>
#include <QScrollBar>
#include <QHeaderView>

CDataTree::CDataTree( const std::shared_ptr< const CServerInfo > &serverInfo, QWidget *parentWidget ) :
    QWidget( parentWidget ),
    fImpl( new Ui::CDataTree )
{
    fImpl->setupUi( this );
    fImpl->data->setExpandsOnDoubleClick( false );
    installEventFilter( this );
    fImpl->data->setContextMenuPolicy( Qt::ContextMenuPolicy::CustomContextMenu );
    connect( fImpl->data, &QTreeView::customContextMenuRequested, this, &CDataTree::slotContextMenuRequested );
    connect( fImpl->data->header(), &QHeaderView::sectionClicked, this, &CDataTree::slotHeaderClicked );
    setServer( serverInfo, false );
}

CDataTree::~CDataTree()
{
}

void CDataTree::setModel( QAbstractItemModel *model )
{
    fImpl->data->setModel( model );

    connect( fImpl->data->verticalScrollBar(), &QScrollBar::sliderMoved, this, &CDataTree::sigVSliderMoved );
    connect( fImpl->data->verticalScrollBar(), &QScrollBar::actionTriggered, this, &CDataTree::slotVActionTriggered );
    connect( fImpl->data->horizontalScrollBar(), &QScrollBar::sliderMoved, this, &CDataTree::slotUpdateHorizontalScroll );
    connect( fImpl->data->horizontalScrollBar(), &QScrollBar::actionTriggered, this, &CDataTree::slotUpdateHorizontalScroll );
    connect( fImpl->data->selectionModel(), &QItemSelectionModel::currentChanged, this, &CDataTree::sigCurrChanged );
    connect( fImpl->data, &QTreeView::doubleClicked, this, &CDataTree::sigViewData );
}

QAbstractItemModel *CDataTree::model() const
{
    return fImpl->data->model();
}

void CDataTree::setServer( const std::shared_ptr< const CServerInfo > &serverInfo, bool hideColumns )
{
    if ( fServerInfo )
        disconnect( fServerInfo.get(), &CServerInfo::sigServerInfoChanged, this, &CDataTree::slotServerInfoChanged );

    fServerInfo = serverInfo;
    if ( fServerInfo )
        connect( fServerInfo.get(), &CServerInfo::sigServerInfoChanged, this, &CDataTree::slotServerInfoChanged );
    slotServerInfoChanged();
    if ( hideColumns )
        this->hideColumns();
}

void CDataTree::addPeerDataTree( CDataTree *peer )
{
    fPeers.push_back( peer );
    connect( this, &CDataTree::sigCurrChanged, peer, &CDataTree::slotSetCurrentMediaItem );

    connect( this, &CDataTree::sigVSliderMoved, peer, &CDataTree::slotSetVSlider );
    connect( peer, &CDataTree::sigVSliderMoved, this, &CDataTree::slotSetVSlider );

    connect( this, &CDataTree::sigHScrollTo, peer, &CDataTree::slotHScrollTo );
    connect( this, &CDataTree::sigHSliderMoved, peer, &CDataTree::slotSetHSlider );
    connect( peer, &CDataTree::sigHSliderMoved, this, &CDataTree::slotSetHSlider );
}

QModelIndex CDataTree::currentIndex() const
{
    return fImpl->data->selectionModel()->currentIndex();
}

QModelIndex CDataTree::indexAt( const QPoint &pt ) const
{
    return fImpl->data->indexAt( pt );
}

void CDataTree::autoSize()
{
    NSABUtils::autoSize( fImpl->data );
}

void CDataTree::slotSetCurrentMediaItem( const QModelIndex &idx )
{
    fImpl->data->setCurrentIndex( idx );
}

void CDataTree::slotSetVSlider( int position )
{
    fImpl->data->verticalScrollBar()->setValue( position );
}

void CDataTree::slotSetHSlider( int position )
{
    fImpl->data->horizontalScrollBar()->setValue( position );
}

void CDataTree::slotHScrollTo( int value, int max )
{
    if ( max > 0 )
    {
        auto newPos = fImpl->data->horizontalScrollBar()->maximum() * value / max;
        fImpl->data->horizontalScrollBar()->setValue( newPos );
    }
}

void CDataTree::slotUpdateHorizontalScroll( int /*action*/ )
{
    auto value = fImpl->data->horizontalScrollBar()->value();
    auto max = fImpl->data->horizontalScrollBar()->maximum();
    emit sigHScrollTo( value, max );
}

void CDataTree::slotVActionTriggered( int action )
{
    if ( action == QAbstractSlider::SliderMove )
    {
        emit sigVSliderMoved( fImpl->data->verticalScrollBar()->value() );
    }
}

void CDataTree::slotServerInfoChanged()
{
    if ( !fServerInfo )
    {
        fImpl->serverLabel->setText( tr( "Server:" ) );
        return;
    }

    fImpl->serverImage->setHidden( fServerInfo->icon().isNull() );
    fImpl->serverImage->setPixmap( fServerInfo->icon().pixmap( fImpl->serverLabel->height() ) );
    fImpl->serverLabel->setText( tr( "Server: <a href=\"%1\">%2</a>" ).arg( fServerInfo->getUrl().toString( QUrl::RemoveQuery ) ).arg( fServerInfo->displayName( true ) ) );
}

void CDataTree::hideColumns()
{
    auto model = fImpl->data->model();
    if ( !model /*|| ( model->rowCount() == 0 )*/ )
        return;

    if ( !fServerInfo )
        return;

    auto proxyModel = dynamic_cast< QAbstractProxyModel * >( model );
    if ( proxyModel )
        model = proxyModel->sourceModel();

    if ( !model )
        return;

    auto serverModel = dynamic_cast< IServerForColumn * >( model );
    if ( !serverModel )
        return;

    auto numColumns = model->columnCount();
    for ( int ii = 0; ii < numColumns; ++ii )
    {
        auto server = serverModel->serverForColumn( ii );
        bool hide = ( server != "<ALL>" ) && server != fServerInfo->keyName();
        fImpl->data->setColumnHidden( ii, hide );
    }
}

void CDataTree::sort( int defColumn, Qt::SortOrder defOrder )
{
    int column = defColumn;
    auto order = defOrder;
    if ( fUserSort )
    {
        column = fImpl->data->header()->sortIndicatorSection();
        ;
        order = fImpl->data->header()->sortIndicatorOrder();
    }

    fImpl->data->sortByColumn( column, order );
}

bool CDataTree::eventFilter( QObject *obj, QEvent *event )
{
    if ( obj == fImpl->data->horizontalScrollBar() )
    {
        if ( event->type() == QEvent::Show )
        {
            for ( auto &&ii : fPeers )
                ii->fImpl->data->horizontalScrollBar()->setVisible( true );
        }
        else if ( event->type() == QEvent::Hide )
        {
            for ( auto &&ii : fPeers )
                ii->fImpl->data->horizontalScrollBar()->setHidden( true );
        }
    }
    return QWidget::eventFilter( obj, event );
}

bool CDataTree::hasCurrentItem() const
{
    return currentIndex().isValid();
}

QTreeView *CDataTree::dataTree() const
{
    return fImpl->data;
}

void CDataTree::slotContextMenuRequested( const QPoint &pos )
{
    emit sigDataContextMenuRequested( this, pos );
}

void CDataTree::slotHeaderClicked()
{
    fUserSort = true;
}
