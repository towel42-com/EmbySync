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

#include "MediaDataWidget.h"
#include "Core/MediaServerData.h"

#include "ui_MediaDataWidget.h"

#include <QDebug>


CMediaDataWidget::CMediaDataWidget(QWidget* parentWidget /*= nullptr */) :
    QGroupBox(parentWidget),
    fImpl(new Ui::CMediaDataWidget)

{
    fImpl->setupUi(this);
    connect(fImpl->setTimeToNowBtn, &QToolButton::clicked,
        [this]()
        {
            fImpl->lastPlayedDate->setDateTime(QDateTime::currentDateTimeUtc());
        });

    connect(fImpl->hasBeenPlayed, &QCheckBox::clicked,
        [this]()
        {
            if (fImpl->hasBeenPlayed->isChecked())
                fImpl->lastPlayedDate->setDateTime(QDateTime::currentDateTimeUtc());
        });

    connect(fImpl->apply, &QPushButton::clicked, this, &CMediaDataWidget::slotApplyFromServer);
    connect(fImpl->process, &QPushButton::clicked, this, &CMediaDataWidget::slotProcessToServer);
}

void CMediaDataWidget::slotApplyFromServer()
{
    emit sigApplyFromServer(this);
}

void CMediaDataWidget::slotProcessToServer()
{
    emit sigProcessToServer(this);
}

CMediaDataWidget::CMediaDataWidget(const QString& title, QWidget* parentWidget /*= nullptr */) :
    CMediaDataWidget(parentWidget)
{
    setTitle(title);
}

CMediaDataWidget::CMediaDataWidget(std::shared_ptr< SMediaServerData > mediaData, QWidget* parentWidget /*= nullptr */) :
    CMediaDataWidget("User Data:", parentWidget)
{
    setMediaUserData(mediaData);
}

CMediaDataWidget::~CMediaDataWidget()
{
}

void CMediaDataWidget::setMediaUserData(std::shared_ptr< SMediaServerData > mediaData)
{
    fMediaUserData = mediaData;
    load(fMediaUserData);
}

void CMediaDataWidget::applyMediaUserData(std::shared_ptr< SMediaServerData > mediaData)
{
    if (!mediaData)
        return;

    load(mediaData);
}

void CMediaDataWidget::load(std::shared_ptr< SMediaServerData > mediaData)
{
    if (!mediaData)
    {
        fImpl->isFavorite->setChecked(false);
        fImpl->hasBeenPlayed->setChecked(false);
        fImpl->lastPlayedDate->setDateTime(QDateTime());
        fImpl->playbackPosition->setTime(QTime());
        fImpl->playCount->setValue(0);
    }
    else
    {
        fImpl->isFavorite->setChecked(mediaData->fIsFavorite);
        fImpl->hasBeenPlayed->setChecked(mediaData->fPlayed);
        fImpl->lastPlayedDate->setDateTime(mediaData->fLastPlayedDate);
        fImpl->playbackPosition->setTime(mediaData->playbackPositionTime());
        fImpl->playCount->setValue(mediaData->fPlayCount);
    }
}

std::shared_ptr< SMediaServerData > CMediaDataWidget::createMediaUserData() const
{
    auto retVal = std::make_shared< SMediaServerData >();
    retVal->fIsFavorite = fImpl->isFavorite->isChecked();
    retVal->fPlayed = fImpl->hasBeenPlayed->isChecked();
    retVal->fLastPlayedDate = fImpl->lastPlayedDate->dateTime();
    retVal->fPlayCount = fImpl->playCount->value();
    retVal->setPlaybackPosition(fImpl->playbackPosition->time());
    return retVal;
}

