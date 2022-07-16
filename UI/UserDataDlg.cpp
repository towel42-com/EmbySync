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

#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>

#include "ui_UserDataDlg.h"

CUserDataDlg::CUserDataDlg( std::shared_ptr< CUserData > userData, std::shared_ptr< CMediaData > mediaData, const QString & serverName, bool isLHS, QWidget * parentWidget )
    : QDialog( parentWidget ),
    fImpl( new Ui::CUserDataDlg ),
    fUserData( userData ),
    fServerInfo( serverName, isLHS ),
    fMediaData( mediaData )
{
    fImpl->setupUi( this );
    connect( fImpl->setTimeToNowBtn, &QToolButton::clicked,
             [ this ]()
             {
                 fImpl->lastPlayedDate->setDateTime( QDateTime::currentDateTimeUtc() );
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
    fImpl->serverName->setText( fServerInfo.first );
    fImpl->mediaName->setText( fMediaData->name() );

    fImpl->isFavorite->setChecked( fMediaData->isFavorite( fServerInfo.second ) );
    fImpl->hasBeenPlayed->setChecked( fMediaData->isPlayed( fServerInfo.second ) );
    fImpl->lastPlayedDate->setDateTime( fMediaData->lastPlayed( fServerInfo.second ) );
    fImpl->playbackPosition->setTime( fMediaData->playbackPositionTime( fServerInfo.second ) );
    fImpl->playCount->setValue( fMediaData->playCount( fServerInfo.second ) );
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

