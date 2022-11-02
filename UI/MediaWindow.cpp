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

#include "MediaWindow.h"
#include "MediaDataWidget.h"

#include "ui_MediaWindow.h"
#include "Core/SyncSystem.h"
#include "Core/MediaData.h"
#include "Core/ServerModel.h"
#include "Core/ServerInfo.h"
#include "SABUtils/WidgetChanged.h"

#include <QHBoxLayout>
#include <QMetaMethod>
#include <QMessageBox>

#include <QCloseEvent>

CMediaWindow::CMediaWindow( std::shared_ptr< CServerModel > serverModel, std::shared_ptr< CSyncSystem > syncSystem, QWidget * parent )
    : QWidget( parent ),
    fImpl( new Ui::CMediaWindow ),
    fSyncSystem( syncSystem )
{
    fImpl->setupUi( this );
    connect( fImpl->process, &QPushButton::clicked, this, &CMediaWindow::slotUploadUserMediaData );

    auto horizontalLayout = new QHBoxLayout( fImpl->userDataWidgets );
    
    for ( int ii = 0; ii < serverModel->serverCnt(); ++ii )
    {
        auto serverInfo = serverModel->getServerInfo( ii );
        if ( !serverInfo->isEnabled() )
            continue;

        auto userDataWidget = new CMediaDataWidget( this );
        userDataWidget->setServerName( serverInfo->keyName() );
        horizontalLayout->addWidget( userDataWidget );
        fUserDataWidgets[ serverInfo->keyName() ] = userDataWidget;
        connect( userDataWidget, &CMediaDataWidget::sigApplyFromServer, this, &CMediaWindow::slotApplyFromServer );
        connect( userDataWidget, &CMediaDataWidget::sigProcessToServer, this, &CMediaWindow::slotProcessToServer );
    }

    NSABUtils::setupWidgetChanged( this, QMetaMethod::fromSignal( &CMediaWindow::sigChanged ), { fImpl->process } );
    connect( this, &CMediaWindow::sigChanged,
             [this]()
             {
                 fChanged = true;
                 fImpl->process->setEnabled( true );
             } );

    fImpl->process->setEnabled( false );
}

CMediaWindow::~CMediaWindow()
{
}

void CMediaWindow::setMedia( std::shared_ptr< CMediaData > mediaInfo )
{
    loadMedia( mediaInfo );
}

void CMediaWindow::loadMedia( std::shared_ptr<CMediaData> & mediaInfo )
{
    fMediaInfo = mediaInfo;

    setEnabled( fMediaInfo.get() != nullptr );
    fImpl->currMediaName->setText( fMediaInfo ? fMediaInfo->name() : QString() );
    fImpl->currMediaType->setText( fMediaInfo ? fMediaInfo->mediaType() : QString() );
    fImpl->externalUrls->setText( fMediaInfo ? fMediaInfo->externalUrlsText() : tr( "External Urls:" ) );
    fImpl->externalUrls->setTextFormat( Qt::RichText );

    for ( auto && ii : fUserDataWidgets )
    {
        ii.second->setMediaUserData( fMediaInfo ? fMediaInfo->userMediaData( ii.first ) : std::shared_ptr< SMediaServerData >() );
    }

    fChanged = false;
}

void CMediaWindow::reloadMedia()
{
    loadMedia( fMediaInfo );
}

void CMediaWindow::closeEvent( QCloseEvent * event )
{
    if ( !okToClose() )
    {
        event->ignore();
        fChanged = false;
    }
    else
    {
        event->accept();
    }
}

bool CMediaWindow::okToClose() 
{
    if ( fChanged && isVisible() )
    {
        QMessageBox msgBox;
        msgBox.setText( tr( "The media information has changed" ) );
        msgBox.setInformativeText( tr( "Would you like to upload the changes to the server?" ) );
        msgBox.setStandardButtons( QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel );
        msgBox.setDefaultButton( QMessageBox::Save );
        int status = msgBox.exec();

        if ( status == QMessageBox::StandardButton::Save )
            slotUploadUserMediaData();
        if ( status == QMessageBox::StandardButton::Cancel )
            return false;
    }
    return true;
}

void CMediaWindow::slotUploadUserMediaData()
{
    if ( !fMediaInfo )
        return;

    for ( auto && ii : fUserDataWidgets )
    {
        fSyncSystem->updateUserDataForMedia( ii.first, fMediaInfo, ii.second->createMediaUserData() );
    }
    fChanged = false;
}

void CMediaWindow::slotApplyFromServer( CMediaDataWidget * which )
{
    if ( !which )
        return;

    auto newData = which->createMediaUserData();
    for ( auto && ii : fUserDataWidgets )
    {
        if ( ii.second == which )
            continue;
        ii.second->applyMediaUserData( newData );
    }
}

void CMediaWindow::slotProcessToServer( CMediaDataWidget * which )
{
    if ( !which )
        return;

    fSyncSystem->updateUserDataForMedia( which->serverName(), fMediaInfo, which->createMediaUserData() );
}

