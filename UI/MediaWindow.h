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

#ifndef _MEDIAWINDOW_H
#define _MEDIAWINDOW_H

#include <QWidget>
class CMediaData;
class CSyncSystem;
class CSettings;
class CMediaDataWidget;
namespace Ui
{
    class CMediaWindow;
}


class CMediaWindow : public QWidget
{
    Q_OBJECT
public:
    CMediaWindow( std::shared_ptr< CSettings > settings, std::shared_ptr< CSyncSystem > syncSystem, QWidget * parent = nullptr );
    virtual ~CMediaWindow() override;

    void setMedia( std::shared_ptr< CMediaData > media );
    void reloadMedia();

    bool okToClose();
    virtual void closeEvent( QCloseEvent * event ) override;
Q_SIGNALS:
    void sigChanged();
public Q_SLOTS:
private Q_SLOTS:
    void slotUploadUserMediaData();
    void slotApplyFromServer( CMediaDataWidget * which );
    void slotProcessToServer( CMediaDataWidget * which );
private:
    void loadMedia( std::shared_ptr<CMediaData> & mediaInfo );

    bool fChanged{ false };
    std::unique_ptr< Ui::CMediaWindow > fImpl;
    std::shared_ptr< CSyncSystem > fSyncSystem;
    std::shared_ptr< CSettings > fSettings;

    std::map< QString, CMediaDataWidget * > fUserDataWidgets;
    std::shared_ptr< CMediaData > fMediaInfo;
};
#endif 
