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

#include "UserWindow.h"
#include "UserDataWidget.h"
#include "ui_UserWindow.h"
#include "Core/SyncSystem.h"
#include "Core/UserData.h"
#include "Core/Settings.h"
#include "Core/ServerInfo.h"
#include "SABUtils/WidgetChanged.h"

#include <QHBoxLayout>
#include <QMetaMethod>
#include <QMessageBox>

#include <QCloseEvent>

CUserWindow::CUserWindow( std::shared_ptr< CSettings> settings, std::shared_ptr< CSyncSystem > syncSystem, QWidget * parent )
    : QWidget( parent ),
    fImpl( new Ui::CUserWindow ),
    fSyncSystem( syncSystem ),
    fSettings( settings )
{
    fImpl->setupUi( this );
    connect( fImpl->process, &QPushButton::clicked, this, &CUserWindow::slotUploadUserData );

    auto horizontalLayout = new QHBoxLayout( fImpl->userDataWidgets );
    
    for ( int ii = 0; ii < fSettings->serverCnt(); ++ii )
    {
        auto serverInfo = settings->serverInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;

        auto userDataWidget = new CUserDataWidget( this );
        userDataWidget->setServerName( serverInfo->keyName() );
        horizontalLayout->addWidget( userDataWidget );
        fUserDataWidgets[ serverInfo->keyName() ] = userDataWidget;
        connect( userDataWidget, &CUserDataWidget::sigApplyFromServer, this, &CUserWindow::slotApplyFromServer );
        connect( userDataWidget, &CUserDataWidget::sigProcessToServer, this, &CUserWindow::slotProcessToServer );
    }

    NSABUtils::setupWidgetChanged( this, QMetaMethod::fromSignal( &CUserWindow::sigChanged ), { fImpl->process } );
    connect( this, &CUserWindow::sigChanged,
             [this]()
             {
                 fChanged = true;
                 fImpl->process->setEnabled( true );
             } );

    fImpl->process->setEnabled( false );
}

CUserWindow::~CUserWindow()
{
}

void CUserWindow::setUser( std::shared_ptr< CUserData > userData )
{
    loadUser( userData );
}

void CUserWindow::loadUser( std::shared_ptr<CUserData> & userData )
{
    fUserData = userData;

    setEnabled( fUserData.get() != nullptr );
    for ( auto && ii : fUserDataWidgets )
    {
        ii.second->setUserData( fUserData ? fUserData->userInfo( ii.first ) : std::shared_ptr< SUserServerData >() );
    }

    fChanged = false;
}

void CUserWindow::reloadUser()
{
    loadUser( fUserData );
}

void CUserWindow::closeEvent( QCloseEvent * event )
{
    if ( !okToClose() )
    {
        event->ignore();
        fChanged = false;
    }
    else
        event->accept();
}

bool CUserWindow::okToClose() 
{
    if ( fChanged && isVisible() )
    {
        QMessageBox msgBox;
        msgBox.setText( tr( "The user information has changed" ) );
        msgBox.setInformativeText( tr( "Would you like to upload the changes to the server?" ) );
        msgBox.setStandardButtons( QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel );
        msgBox.setDefaultButton( QMessageBox::Save );
        int status = msgBox.exec();

        if ( status == QMessageBox::StandardButton::Save )
            slotUploadUserData();
        if ( status == QMessageBox::StandardButton::Cancel )
            return false;
    }
    return true;
}

void CUserWindow::slotUploadUserData()
{
    if ( !fUserData )
        return;

    for ( auto && ii : fUserDataWidgets )
    {
        fSyncSystem->updateUserData( ii.first, fUserData, ii.second->createUserData() );
        if ( !ii.second->avatar().isNull() )
            fSyncSystem->requestSetUserAvatar( ii.first, fUserData->getUserID( ii.first ), ii.second->avatar() );
    }
    fChanged = false;
}

void CUserWindow::slotApplyFromServer( CUserDataWidget * which )
{
    if ( !which )
        return;

    auto newData = which->createUserData();
    for ( auto && ii : fUserDataWidgets )
    {
        if ( ii.second == which )
            continue;
        ii.second->applyUserData( newData );
    }
}

void CUserWindow::slotProcessToServer( CUserDataWidget * which )
{
    if ( !which )
        return;

    fSyncSystem->updateUserData( which->serverName(), fUserData, which->createUserData() );
    if ( !which->avatar().isNull() )
        fSyncSystem->requestSetUserAvatar( which->serverName(), fUserData->getUserID( which->serverName() ), which->avatar() );
}

