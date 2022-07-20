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

#include "MediaUserDataWidget.h"
#include "Core/MediaData.h"

#include "ui_MediaUserDataWidget.h"

#include <QDebug>


CMediaUserDataWidget::CMediaUserDataWidget( QWidget * parentWidget /*= nullptr */ ) :
    QGroupBox( parentWidget ),
    fImpl( new Ui::CMediaUserDataWidget )

{
    fImpl->setupUi( this );
    connect( fImpl->setTimeToNowBtn, &QToolButton::clicked,
             [this]()
             {
                 fImpl->lastPlayedDate->setDateTime( QDateTime::currentDateTimeUtc() );
             } );

    connect( fImpl->hasBeenPlayed, &QCheckBox::clicked,
             [this]()
             {
                 if ( fImpl->hasBeenPlayed->isChecked() )
                     fImpl->lastPlayedDate->setDateTime( QDateTime::currentDateTimeUtc() );
             } );

    connect( fImpl->isFavorite, &QCheckBox::toggled, this, &CMediaUserDataWidget::slotChanged );
    connect( fImpl->hasBeenPlayed, &QCheckBox::toggled, this, &CMediaUserDataWidget::slotChanged );
    connect( fImpl->lastPlayedDate, &QDateTimeEdit::dateTimeChanged, this, &CMediaUserDataWidget::slotChanged );
    connect( fImpl->playbackPosition, &QTimeEdit::timeChanged, this, &CMediaUserDataWidget::slotChanged );
    connect( fImpl->playCount, qOverload< int >( &QSpinBox::valueChanged ), this, &CMediaUserDataWidget::slotChanged );
}

CMediaUserDataWidget::CMediaUserDataWidget( const QString & title, QWidget * parentWidget /*= nullptr */ ) :
    CMediaUserDataWidget( parentWidget )
{
    setTitle( title );
}

CMediaUserDataWidget::CMediaUserDataWidget( std::shared_ptr< SMediaUserData > mediaData, QWidget * parentWidget /*= nullptr */ ) :
    CMediaUserDataWidget( "User Data:", parentWidget )
{
    setMediaUserData( mediaData );
}

CMediaUserDataWidget::~CMediaUserDataWidget()
{
}

void CMediaUserDataWidget::setMediaUserData( std::shared_ptr< SMediaUserData > mediaData )
{
    fMediaUserData = mediaData;
    load( fMediaUserData );
}

void CMediaUserDataWidget::applyMediaUserData( std::shared_ptr< SMediaUserData > mediaData )
{
    if ( !mediaData )
        return;

    load( mediaData );
}

void CMediaUserDataWidget::load( std::shared_ptr< SMediaUserData > mediaData )
{
    if ( !mediaData )
    {
        fImpl->isFavorite->setChecked( false );
        fImpl->hasBeenPlayed->setChecked( false );
        fImpl->lastPlayedDate->setDateTime( QDateTime() );
        fImpl->playbackPosition->setTime( QTime() );
        fImpl->playCount->setValue( 0 );
    }
    else
    {
        fImpl->isFavorite->setChecked( mediaData->fIsFavorite );
        fImpl->hasBeenPlayed->setChecked( mediaData->fPlayed );
        fImpl->lastPlayedDate->setDateTime( mediaData->fLastPlayedDate );
        fImpl->playbackPosition->setTime( mediaData->playbackPositionTime() );
        fImpl->playCount->setValue( mediaData->fPlayCount );
    }
}

std::shared_ptr< SMediaUserData > CMediaUserDataWidget::createMediaUserData() const
{
    auto retVal = std::make_shared< SMediaUserData >();
    retVal->fIsFavorite = fImpl->isFavorite->isChecked();
    retVal->fPlayed = fImpl->hasBeenPlayed->isChecked();
    retVal->fLastPlayedDate = fImpl->lastPlayedDate->dateTime();
    retVal->fPlayCount = fImpl->playCount->value();
    retVal->setPlaybackPosition( fImpl->playbackPosition->time() );
    return retVal;
}

void CMediaUserDataWidget::setReadOnly( bool readOnly )
{
    fReadOnly = readOnly;
    fImpl->setTimeToNowBtn->setHidden( readOnly );
    fImpl->isFavorite->setEnabled( readOnly );
    fImpl->hasBeenPlayed->setEnabled( readOnly );
    fImpl->lastPlayedDate->setEnabled( readOnly );
    fImpl->playbackPosition->setEnabled( readOnly );
    fImpl->playCount->setEnabled( readOnly );
}

void CMediaUserDataWidget::slotChanged()
{
}
