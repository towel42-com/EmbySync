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

#include "MediaTree.h"
#include "ui_MediaTree.h"
#include "SABUtils/QtUtils.h"
#include "Core/MediaModel.h"
#include "Core/Settings.h"
#include "Core/ServerInfo.h"

#include <QScrollBar>

CMediaTree::CMediaTree( const std::shared_ptr< const CServerInfo > & serverInfo, QWidget * parentWidget )
    : QWidget( parentWidget ),
    fImpl( new Ui::CMediaTree ),
    fServerInfo( serverInfo )
{
    fImpl->setupUi( this );
    fImpl->media->setExpandsOnDoubleClick( false );
    fImpl->serverLabel->setText( tr( "Server: <a href=\"%1\">%2 - %1</a>" ).arg( serverInfo->getUrl().toString( QUrl::RemoveUserInfo | QUrl::RemoveQuery ) ).arg( serverInfo->displayName() ) );
    installEventFilter( this );
}

CMediaTree::~CMediaTree()
{
}

void CMediaTree::setModel( QAbstractItemModel * model )
{
    fImpl->media->setModel( model );

    connect( fImpl->media->verticalScrollBar(), &QScrollBar::sliderMoved, this, &CMediaTree::sigVSliderMoved );
    connect( fImpl->media->verticalScrollBar(), &QScrollBar::actionTriggered, this, &CMediaTree::slotVActionTriggered );
    connect( fImpl->media->horizontalScrollBar(), &QScrollBar::sliderMoved, this, &CMediaTree::slotUpdateHorizontalScroll );
    connect( fImpl->media->horizontalScrollBar(), &QScrollBar::actionTriggered, this, &CMediaTree::slotUpdateHorizontalScroll );
    connect( fImpl->media->selectionModel(), &QItemSelectionModel::currentChanged, this, &CMediaTree::sigCurrChanged );
    connect( fImpl->media, &QTreeView::doubleClicked, this, &CMediaTree::sigViewMedia );
}

void CMediaTree::addPeerMediaTree( CMediaTree * peer )
{
    fPeers.push_back( peer );
    connect( this, &CMediaTree::sigCurrChanged, peer, &CMediaTree::slotSetCurrentMediaItem );

    connect( this, &CMediaTree::sigVSliderMoved, peer, &CMediaTree::slotSetVSlider );
    connect( peer, &CMediaTree::sigVSliderMoved, this, &CMediaTree::slotSetVSlider );

    
    connect( this, &CMediaTree::sigHScrollTo, peer, &CMediaTree::slotHScrollTo );
    connect( this, &CMediaTree::sigHSliderMoved, peer, &CMediaTree::slotSetHSlider );
    connect( peer, &CMediaTree::sigHSliderMoved, this, &CMediaTree::slotSetHSlider );
}

QModelIndex CMediaTree::currentIndex() const
{
    return fImpl->media->selectionModel()->currentIndex();
}

void CMediaTree::autoSize()
{
    NSABUtils::autoSize( fImpl->media );
}

void CMediaTree::slotSetCurrentMediaItem( const QModelIndex & idx )
{
    fImpl->media->setCurrentIndex( idx );
}

void CMediaTree::slotSetVSlider( int position )
{
    fImpl->media->verticalScrollBar()->setValue( position );
}

void CMediaTree::slotSetHSlider( int position )
{
    fImpl->media->horizontalScrollBar()->setValue( position );
}

void CMediaTree::slotHScrollTo( int value, int max )
{
    auto newPos = fImpl->media->horizontalScrollBar()->maximum() * value / max;
    fImpl->media->horizontalScrollBar()->setValue( newPos );
}

void CMediaTree::slotUpdateHorizontalScroll( int /*action*/ )
{
    auto value = fImpl->media->horizontalScrollBar()->value();
    auto max = fImpl->media->horizontalScrollBar()->maximum();
    emit sigHScrollTo( value, max );
}

void CMediaTree::slotVActionTriggered( int action )
{
    if ( action == QAbstractSlider::SliderMove )
    {
        emit sigVSliderMoved( fImpl->media->verticalScrollBar()->value() );
    }
}

void CMediaTree::hideColumns()
{
    auto model = fImpl->media->model();
    auto proxyModel = dynamic_cast<QSortFilterProxyModel * >( model );
    if ( proxyModel )
        model = proxyModel->sourceModel();

    if ( !model )
        return;

    auto numColumns = model->columnCount();
    
    for ( int ii = 0; ii < numColumns; ++ii )
    {
        auto idx = model->index( 0, ii );
        Q_ASSERT( idx.isValid() );
        auto server = model->data( idx, CMediaModel::eServerNameForColumnRole ).toString();
        bool hide = server != fServerInfo->keyName();
        fImpl->media->setColumnHidden( ii, hide );
    }
}


bool CMediaTree::eventFilter( QObject * obj, QEvent * event )
{
    if ( obj == fImpl->media->horizontalScrollBar() )
    {
        if ( event->type() == QEvent::Show )
        {
            for ( auto && ii : fPeers )
                ii->fImpl->media->horizontalScrollBar()->setVisible( true );
        }
        else if ( event->type() == QEvent::Hide )
        {
            for ( auto && ii : fPeers )
                ii->fImpl->media->horizontalScrollBar()->setHidden( true );
        }
    }
    return QWidget::eventFilter( obj, event );
}
