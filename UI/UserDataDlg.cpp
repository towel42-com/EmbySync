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

#include "UserDataDlg.h"
#include "Core/UserData.h"
#include "Core/MediaData.h"
#include "Core/SyncSystem.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QPushButton>

#include "ui_UserDataDlg.h"

CUserDataDlg::CUserDataDlg( bool origFromLHS, std::shared_ptr< CUserData > userData, std::shared_ptr< CMediaData > mediaData, std::shared_ptr< CSyncSystem > syncSystem, std::shared_ptr< CSettings > settings, QWidget * parentWidget )
    : QDialog( parentWidget ),
    fImpl( new Ui::CUserDataDlg ),
    fUserData( userData ),
    fFromLHS( origFromLHS ),
    fMediaData( mediaData ),
    fSyncSystem( syncSystem ),
    fSettings( settings )
{
    fImpl->setupUi( this );
    connect( fImpl->setTimeToNowBtn, &QToolButton::clicked,
             [ this ]()
             {
                 fImpl->lastPlayedDate->setDateTime( QDateTime::currentDateTimeUtc() );
             } );
    connect( fImpl->hasBeenPlayed, &QCheckBox::clicked,
             [this]()
             {
                 if ( fImpl->hasBeenPlayed->isChecked() )
                     fImpl->lastPlayedDate->setDateTime( QDateTime::currentDateTimeUtc() );
             } );

    connect( fImpl->buttonBox->addButton( tr( "Apply to LHS Server" ), QDialogButtonBox::ButtonRole::ActionRole ), &QPushButton::clicked,
             [this]()
             {
                 fSyncSystem->updateUserDataForMedia( fMediaData, getMediaData(), true );
             } );

    connect( fImpl->buttonBox->addButton( tr( "Apply to RHS Server" ), QDialogButtonBox::ButtonRole::ActionRole ), &QPushButton::clicked,
             [this]()
             {
                 fSyncSystem->updateUserDataForMedia( fMediaData, getMediaData(), false );
             } );

    connect( fImpl->buttonBox->addButton( tr( "Apply to All Servers" ), QDialogButtonBox::ButtonRole::ActionRole ), &QPushButton::clicked,
             [this]()
             {
                 fSyncSystem->updateUserDataForMedia( fMediaData, getMediaData(), true );
                 fSyncSystem->updateUserDataForMedia( fMediaData, getMediaData(), false );
             } );

    load();
}

CUserDataDlg::~CUserDataDlg()
{
}

void CUserDataDlg::accept()
{
    save();
    QDialog::accept();
}


void CUserDataDlg::load()
{
    fImpl->userName->setText( fUserData->name() );
    fImpl->mediaName->setText( fMediaData->name() );

    fImpl->isFavorite->setChecked( fMediaData->isFavorite( fFromLHS ) );
    fImpl->hasBeenPlayed->setChecked( fMediaData->isPlayed( fFromLHS ) );
    fImpl->lastPlayedDate->setDateTime( fMediaData->lastPlayed( fFromLHS ) );
    fImpl->playbackPosition->setTime( fMediaData->playbackPositionTime( fFromLHS ) );
    fImpl->playCount->setValue( fMediaData->playCount( fFromLHS ) );
}

void CUserDataDlg::save()
{
}

std::shared_ptr< SMediaUserData > CUserDataDlg::getMediaData() const
{
    auto retVal = std::make_shared< SMediaUserData >();
    retVal->fIsFavorite = fImpl->isFavorite->isChecked();
    retVal->fPlayed = fImpl->hasBeenPlayed->isChecked();
    retVal->fLastPlayedDate = fImpl->lastPlayedDate->dateTime();
    retVal->fPlayCount = fImpl->playCount->value();
    retVal->fPlaybackPositionTicks = ( 10000ULL * fImpl->playbackPosition->time().msecsSinceStartOfDay() );
    return retVal;
}

