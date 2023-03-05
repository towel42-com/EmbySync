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

#ifndef _DATATREE_H
#define _DATATREE_H

#include <QWidget>
#include <memory>

namespace Ui
{
    class CDataTree;
}

class QLabel;
class CSettings;
class CUserData;
class QAbstractItemModel;
class CServerInfo;
class QTreeView;
class CDataTree : public QWidget
{
    Q_OBJECT
public:
    CDataTree( const std::shared_ptr< const CServerInfo > &serverInfo, QWidget *parent = nullptr );

    virtual ~CDataTree() override;

    void setModel( QAbstractItemModel *model );
    QAbstractItemModel *model() const;

    void setServer( const std::shared_ptr< const CServerInfo > &serverInfo, bool hideColumns );
    void hideColumns();
    void sort( int column, Qt::SortOrder order );   // if the user hasnt changed the sort, use column and order, otherwise use existing settings

    void addPeerDataTree( CDataTree *peer );
    QModelIndex currentIndex() const;
    QModelIndex indexAt( const QPoint &pt ) const;
    void autoSize();

    virtual bool eventFilter( QObject *obj, QEvent *event ) override;

    bool hasCurrentItem() const;

    QTreeView *dataTree() const;
    std::shared_ptr< const CServerInfo > serverInfo() const { return fServerInfo; }
Q_SIGNALS:
    void sigCurrChanged( const QModelIndex &idx );
    void sigViewData( const QModelIndex &idx );
    void sigVSliderMoved( int position );
    void sigHSliderMoved( int position );
    void sigHScrollTo( int value, int max );
    void sigDataContextMenuRequested( CDataTree *tree, const QPoint &pos );
    void sigItemDoubleClicked( CDataTree * tree, const QModelIndex & idx );
private Q_SLOTS:
    void slotSetCurrentMediaItem( const QModelIndex &idx );
    void slotSetVSlider( int position );
    void slotSetHSlider( int position );
    void slotHScrollTo( int value, int max );
    void slotUpdateHorizontalScroll( int );
    void slotVActionTriggered( int action );
    void slotServerInfoChanged();
    void slotContextMenuRequested( const QPoint &pos );
    void slotDoubleClicked( const QModelIndex & idx );
    void slotHeaderClicked();

private:
    std::unique_ptr< Ui::CDataTree > fImpl;
    std::vector< CDataTree * > fPeers;

    bool fUserSort{ false };
    std::shared_ptr< const CServerInfo > fServerInfo;
};
#endif
