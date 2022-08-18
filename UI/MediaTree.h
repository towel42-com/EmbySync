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

#ifndef _MEDIATREE_H
#define _MEDIATREE_H

#include <QWidget>
#include <memory>

namespace Ui
{
    class CMediaTree;
}

class QLabel;
class CSettings;
class CUserData;
class QAbstractItemModel;
struct SServerInfo;
class CMediaTree : public QWidget
{
    Q_OBJECT
public:
    CMediaTree( const std::shared_ptr< SServerInfo > & serverInfo, QWidget * parent = nullptr );
    virtual ~CMediaTree() override;

    void setModel( QAbstractItemModel * model );
    void hideColumns();

    void addPeerMediaTree( CMediaTree * peer );
    QModelIndex currentIndex() const;
    void autoSize();

    virtual bool eventFilter( QObject * obj, QEvent * event ) override;

Q_SIGNALS:
    void sigCurrChanged( const QModelIndex & idx );
    void sigViewMedia( const QModelIndex & idx );
    void sigVSliderMoved( int position );
    void sigHSliderMoved( int position );

private Q_SLOTS:
    void slotSetCurrentMediaItem( const QModelIndex & idx );
    void slotSetVSlider( int position );
    void slotSetHSlider( int position );
private:
    std::unique_ptr< Ui::CMediaTree > fImpl;
    std::vector< CMediaTree * > fPeers;

    const std::shared_ptr< SServerInfo > fServerInfo;
};
#endif 
