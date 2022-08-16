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
#include "ui_MediaWindow.h"
#include "Core/SyncSystem.h"
#include "Core/MediaData.h"
#include "SABUtils/WidgetChanged.h"

#include <QMetaMethod>
#include <QMessageBox>

CMediaWindow::CMediaWindow( std::shared_ptr< CSyncSystem > syncSystem, QWidget * parent )
    : QWidget( parent ),
    fImpl( new Ui::CMediaWindow ),
    fSyncSystem( syncSystem )
{
    fImpl->setupUi( this );

    connect( fImpl->applyToLeft, &QToolButton::clicked, this, &CMediaWindow::slotApplyToLeft );
    connect( fImpl->applyToRight, &QToolButton::clicked, this, &CMediaWindow::slotApplyToRight );
    connect( fImpl->process, &QToolButton::clicked, this, &CMediaWindow::slotUploadUserMediaData );

    NSABUtils::setupWidgetChanged( this, QMetaMethod::fromSignal( &CMediaWindow::sigChanged ) );
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
    fMediaInfo = mediaInfo;

    setEnabled( mediaInfo.get() != nullptr );
    fImpl->currMediaName->setText( mediaInfo ? mediaInfo->name() : QString() );
    fImpl->currMediaType->setText( mediaInfo ? mediaInfo->mediaType() : QString() );
    fImpl->externalUrls->setText( mediaInfo ? mediaInfo->externalUrlsText() : tr( "External Urls:" ) );
    fImpl->externalUrls->setTextFormat( Qt::RichText );
    fImpl->lhsUserMediaData->setMediaUserData( mediaInfo ? mediaInfo->userMediaData( true ) : std::shared_ptr< SMediaUserData >() );
    fImpl->rhsUserMediaData->setMediaUserData( mediaInfo ? mediaInfo->userMediaData( false ) : std::shared_ptr< SMediaUserData >() );
}

bool CMediaWindow::okToClose() 
{
    if ( fChanged )
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

void CMediaWindow::slotApplyToLeft()
{
    fImpl->lhsUserMediaData->applyMediaUserData( fImpl->rhsUserMediaData->createMediaUserData() );
}

void CMediaWindow::slotApplyToRight()
{
    fImpl->rhsUserMediaData->applyMediaUserData( fImpl->lhsUserMediaData->createMediaUserData() );
}

void CMediaWindow::slotUploadUserMediaData()
{
    if ( !fMediaInfo )
        return;

    fSyncSystem->updateUserDataForMedia( fMediaInfo, fImpl->lhsUserMediaData->createMediaUserData(), true );
    fSyncSystem->updateUserDataForMedia( fMediaInfo, fImpl->rhsUserMediaData->createMediaUserData(), false );
    fChanged = false;
}

