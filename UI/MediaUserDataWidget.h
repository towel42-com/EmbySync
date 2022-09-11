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

#ifndef _MEDIAUSERDATAWIDGET_H
#define _MEDIAUSERDATAWIDGET_H

#include <QGroupBox>
#include <memory>

namespace Ui
{
    class CMediaUserDataWidget;
}

class CUserData;
struct SMediaUserData;
class CMediaUserDataWidget : public QGroupBox
{
    Q_OBJECT
public:
    CMediaUserDataWidget( QWidget * parentWidget = nullptr );
    CMediaUserDataWidget( const QString & title, QWidget * parentWidget = nullptr );
    CMediaUserDataWidget( std::shared_ptr< SMediaUserData > mediaData, QWidget * parentWidget = nullptr );

    virtual ~CMediaUserDataWidget() override;

    void setMediaUserData( std::shared_ptr< SMediaUserData > mediaData );
    void applyMediaUserData( std::shared_ptr< SMediaUserData > mediaData );  // mediaData is not owned, mediaID is NOT changed

    void setServerName( const QString & serverName ) { fServerName = serverName; }
    QString serverName() const { return fServerName; }

    void setReadOnly( bool readOnly );
    bool readOnly() const { return fReadOnly; }

    std::shared_ptr< SMediaUserData > createMediaUserData() const; // creates a new user data based on current settings

public Q_SLOTS:
    void slotChanged();
    void slotApplyFromServer();
    void slotProcessToServer();
Q_SIGNALS:
    void sigApplyFromServer( CMediaUserDataWidget * which );
    void sigProcessToServer( CMediaUserDataWidget * which );

private:
    void load( std::shared_ptr< SMediaUserData > mediaData );

    std::unique_ptr< Ui::CMediaUserDataWidget > fImpl;
    std::shared_ptr< SMediaUserData > fMediaUserData;
    QString fServerName;
    bool fReadOnly{ false };
};
#endif 
