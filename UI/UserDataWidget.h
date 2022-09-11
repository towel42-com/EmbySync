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

#ifndef _USERDATAWIDGET_H
#define _USERDATAWIDGET_H

#include <QGroupBox>
#include <memory>

namespace Ui
{
    class CUserDataWidget;
}

class CUserData;
struct SUserServerData;
class CUserDataWidget : public QGroupBox
{
    Q_OBJECT
public:
    CUserDataWidget( QWidget * parentWidget = nullptr );
    CUserDataWidget( const QString & title, QWidget * parentWidget = nullptr );
    CUserDataWidget( std::shared_ptr< SUserServerData > userData, QWidget * parentWidget = nullptr );

    virtual ~CUserDataWidget() override;

    void setServerName( const QString & serverName ) { fServerName = serverName; }
    QString serverName() const { return fServerName; }
    void setUserData( std::shared_ptr< SUserServerData > userData );
    void applyUserData( std::shared_ptr< SUserServerData > userData );  

    QImage avatar() const { return fAvatar; }

    std::shared_ptr< SUserServerData > createUserData() const; // creates a new user data based on current settings

public Q_SLOTS:
    void slotApplyFromServer();
    void slotProcessToServer();
    void slotSelectChangeAvatar();
Q_SIGNALS:
    void sigApplyFromServer( CUserDataWidget * which );
    void sigProcessToServer( CUserDataWidget * which );

private:
    void load( std::shared_ptr< SUserServerData > userData );
    void setAvatar( const QImage & image );

    std::unique_ptr< Ui::CUserDataWidget > fImpl;
    std::shared_ptr< SUserServerData > fUserData;
    QString fServerName;
    QImage fAvatar;
};
#endif 
