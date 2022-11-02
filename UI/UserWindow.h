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

#ifndef _USERWINDOW_H
#define _USERWINDOW_H

#include <QWidget>
class CUserData;
class CSyncSystem;
class CServerModel;
class CUserDataWidget;
namespace Ui
{
    class CUserWindow;
}


class CUserWindow : public QWidget
{
    Q_OBJECT
public:
    CUserWindow( std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CSyncSystem > syncSystem, QWidget * parent = nullptr );
    virtual ~CUserWindow() override;

    void setUser( std::shared_ptr< CUserData > userData );
    void reloadUser();;

    bool okToClose();
    virtual void closeEvent( QCloseEvent * event ) override;
Q_SIGNALS:
    void sigChanged();
public Q_SLOTS:
private Q_SLOTS:
    void slotUploadUserData();
    void slotApplyFromServer( CUserDataWidget * which );
    void slotProcessToServer( CUserDataWidget * which );
private:
    void loadUser( std::shared_ptr<CUserData> & userData );

    bool fChanged{ false };
    std::unique_ptr< Ui::CUserWindow > fImpl;
    std::shared_ptr< CSyncSystem > fSyncSystem;
    //std::shared_ptr< CSettings > fSettings;

    std::map< QString, CUserDataWidget * > fUserDataWidgets;
    std::shared_ptr< CUserData > fUserData;
};
#endif 
