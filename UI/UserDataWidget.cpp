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

#include "UserDataWidget.h"
#include "Core/UserData.h"
#include "Core/UserServerData.h"


#include "ui_UserDataWidget.h"

#include <QDebug>
#include <QFileDialog>
#include <QImageReader>

CUserDataWidget::CUserDataWidget(QWidget* parentWidget /*= nullptr */) :
    QGroupBox(parentWidget),
    fImpl(new Ui::CUserDataWidget)

{
    fImpl->setupUi(this);
    connect(fImpl->setLastLoginDateToNow, &QToolButton::clicked,
        [this]()
        {
            fImpl->lastLoginDate->setDateTime(QDateTime::currentDateTimeUtc());
        });
    connect(fImpl->setLastActivityDateToNow, &QToolButton::clicked,
        [this]()
        {
            fImpl->lastActivityDate->setDateTime(QDateTime::currentDateTimeUtc());
        });
    connect(fImpl->setCreatedDateToNow, &QToolButton::clicked,
        [this]()
        {
            fImpl->creationDate->setDateTime(QDateTime::currentDateTimeUtc());
        });

    connect(fImpl->setAvatarBtn, &QToolButton::clicked, this, &CUserDataWidget::slotSelectChangeAvatar);
    connect(fImpl->apply, &QPushButton::clicked, this, &CUserDataWidget::slotApplyFromServer);
    connect(fImpl->process, &QPushButton::clicked, this, &CUserDataWidget::slotProcessToServer);

    fImpl->tabWidget->setCurrentIndex(0);
}

void CUserDataWidget::slotApplyFromServer()
{
    emit sigApplyFromServer(this);
}

void CUserDataWidget::slotProcessToServer()
{
    emit sigProcessToServer(this);
}

CUserDataWidget::CUserDataWidget(const QString& title, QWidget* parentWidget /*= nullptr */) :
    CUserDataWidget(parentWidget)
{
    setTitle(title);
}

CUserDataWidget::CUserDataWidget(std::shared_ptr< SUserServerData > userData, QWidget* parentWidget /*= nullptr */) :
    CUserDataWidget("User Data:", parentWidget)
{
    setUserData(userData);
}

CUserDataWidget::~CUserDataWidget()
{
}

void CUserDataWidget::setUserData(std::shared_ptr< SUserServerData > userData)
{
    fUserData = userData;
    load(fUserData);
}

void CUserDataWidget::applyUserData(std::shared_ptr< SUserServerData > userData)
{
    if (!userData)
        return;

    load(userData);
}

void CUserDataWidget::load(std::shared_ptr< SUserServerData > userData)
{
    if (!userData)
    {
        fAvatar = {};
        fImpl->avatar->setPixmap(QPixmap(":/resources/missingAvatar.png").scaled(32, 32));
        fImpl->name->setText(QString());
        fImpl->prefix->setText(QString());
        fImpl->enableAutoLogin->setChecked(false);
        fImpl->avatarAspectRatio->setValue(0.0);
        fImpl->connectID->setText(QString());
        fImpl->connectIDType->setCurrentIndex(1);
        fImpl->creationDate->setDateTime(QDateTime());
        fImpl->lastActivityDate->setDateTime(QDateTime());
        fImpl->lastLoginDate->setDateTime(QDateTime());

        fImpl->audioLanguagePreference->setText(QString());
        fImpl->playDefaultAudioTrack->setChecked(false);
        fImpl->subtitleLanguagePreference->setText(QString());
        fImpl->displayMissingEpisodes->setChecked(false);
        fImpl->subtitleMode->setCurrentIndex(0);
        fImpl->enableLocalPassword->setChecked(false);
        fImpl->orderedViews->clear();
        fImpl->latestItemsExcludes->clear();
        fImpl->myMediaExcludes->clear();
        fImpl->hidePlayedInLatest->setChecked(false);
        fImpl->rememberAudioSelections->setChecked(false);
        fImpl->rememberSubtitleSelections->setChecked(false);
        fImpl->enableNextEpisodeAutoPlay->setChecked(false);
        fImpl->resumeRewindSeconds->setValue(0);
        fImpl->introSkipMode->setCurrentIndex(0);
    }
    else
    {
        setAvatar(std::get< 2 >(userData->fAvatarInfo));
        fImpl->avatarAspectRatio->setValue(std::get< 1 >(userData->fAvatarInfo));
        fImpl->name->setText(userData->fName);
        fImpl->prefix->setText(userData->fPrefix);
        fImpl->enableAutoLogin->setChecked(userData->fEnableAutoLogin);
        fImpl->connectID->setText(userData->fConnectedID.second);
        auto pos = fImpl->connectIDType->findText(userData->fConnectedID.first);
        if (pos != -1)
            fImpl->connectIDType->setCurrentIndex(pos);
        else
            fImpl->connectIDType->setCurrentIndex(0);
        fImpl->creationDate->setDateTime(userData->fDateCreated);
        fImpl->lastActivityDate->setDateTime(userData->fLastActivityDate);
        fImpl->lastLoginDate->setDateTime(userData->fLastLoginDate);

        fImpl->audioLanguagePreference->setText(userData->fAudioLanguagePreference);
        fImpl->playDefaultAudioTrack->setChecked(userData->fPlayDefaultAudioTrack);
        fImpl->subtitleLanguagePreference->setText(userData->fSubtitleLanguagePreference);
        fImpl->displayMissingEpisodes->setChecked(userData->fDisplayMissingEpisodes);

        pos = fImpl->subtitleMode->findText(userData->fSubtitleMode);
        if (pos != -1)
            fImpl->subtitleMode->setCurrentIndex(pos);
        else
            fImpl->subtitleMode->setCurrentIndex(0);

        fImpl->enableLocalPassword->setChecked(userData->fEnableLocalPassword);
        fImpl->orderedViews->clear();
        fImpl->orderedViews->addItems(userData->fOrderedViews);
        fImpl->latestItemsExcludes->clear();
        fImpl->latestItemsExcludes->addItems(userData->fLatestItemsExcludes);
        fImpl->myMediaExcludes->clear();
        fImpl->myMediaExcludes->addItems(userData->fMyMediaExcludes);
        fImpl->hidePlayedInLatest->setChecked(userData->fHidePlayedInLatest);
        fImpl->rememberAudioSelections->setChecked(userData->fRememberAudioSelections);
        fImpl->rememberSubtitleSelections->setChecked(userData->fRememberSubtitleSelections);
        fImpl->enableNextEpisodeAutoPlay->setChecked(userData->fEnableNextEpisodeAutoPlay);
        fImpl->resumeRewindSeconds->setValue(userData->fResumeRewindSeconds);

        pos = fImpl->introSkipMode->findText(userData->fIntroSkipMode);
        if (pos != -1)
            fImpl->introSkipMode->setCurrentIndex(pos);
        else
            fImpl->introSkipMode->setCurrentIndex(0);
    }
}

void CUserDataWidget::setAvatar(const QImage& image)
{
    fAvatar = image;
    auto scaled = fAvatar.isNull() ? fAvatar : fAvatar.scaled(32, 32);
    fImpl->avatar->setPixmap(QPixmap::fromImage(scaled));
}

QStringList getStrings(QListWidget* listWidget)
{
    if (!listWidget)
        return {};
    QStringList retVal;
    for (int ii = 0; ii < listWidget->count(); ++ii)
    {
        auto item = listWidget->item(ii);
        if (!item)
            continue;
        retVal << item->text();
    }
    return retVal;
}

std::shared_ptr< SUserServerData > CUserDataWidget::createUserData() const
{
    if (!fUserData)
        return {};
    auto retVal = std::make_shared< SUserServerData >();
    retVal->fName = fImpl->name->text();
    retVal->fUserID = fUserData->fUserID;
    retVal->fPrefix = fImpl->prefix->text();
    retVal->fEnableAutoLogin = fImpl->enableAutoLogin->isChecked();
    std::get< 1 >(retVal->fAvatarInfo) = fImpl->avatarAspectRatio->value();
    std::get< 2 >(retVal->fAvatarInfo) = fAvatar;
    retVal->fConnectedID.first = fImpl->connectIDType->currentText();
    retVal->fConnectedID.second = fImpl->connectID->text();
    retVal->fDateCreated = fImpl->creationDate->dateTime();
    retVal->fLastActivityDate = fImpl->lastActivityDate->dateTime();
    retVal->fLastLoginDate = fImpl->lastLoginDate->dateTime();

    retVal->fAudioLanguagePreference = fImpl->audioLanguagePreference->text();
    retVal->fPlayDefaultAudioTrack = fImpl->playDefaultAudioTrack->isChecked();
    retVal->fSubtitleLanguagePreference = fImpl->subtitleLanguagePreference->text();
    retVal->fDisplayMissingEpisodes = fImpl->displayMissingEpisodes->isChecked();
    retVal->fSubtitleMode = fImpl->subtitleMode->currentText();
    retVal->fEnableLocalPassword = fImpl->enableLocalPassword->isChecked();
    retVal->fOrderedViews = getStrings(fImpl->orderedViews);
    retVal->fLatestItemsExcludes = getStrings(fImpl->latestItemsExcludes);;
    retVal->fMyMediaExcludes = getStrings(fImpl->myMediaExcludes);
    retVal->fHidePlayedInLatest = fImpl->hidePlayedInLatest->isChecked();
    retVal->fRememberAudioSelections = fImpl->rememberAudioSelections->isChecked();
    retVal->fRememberSubtitleSelections = fImpl->rememberSubtitleSelections->isChecked();
    retVal->fEnableNextEpisodeAutoPlay = fImpl->enableNextEpisodeAutoPlay->isChecked();
    retVal->fResumeRewindSeconds = fImpl->resumeRewindSeconds->value();
    retVal->fIntroSkipMode = fImpl->introSkipMode->currentText();
    return retVal;
}

void CUserDataWidget::slotSelectChangeAvatar()
{
    auto formats = QImageReader::supportedImageFormats();
    QStringList exts;
    for (auto&& ii : formats)
    {
        exts << "*." + ii;
    }

    auto extensions = tr("Image Files (%1);;All Files (* *.*)").arg(exts.join(" "));
    auto fileName = QFileDialog::getOpenFileName(this, QObject::tr("Select Image File"), QString(), extensions);
    if (fileName.isEmpty())
        return;
    setAvatar(QImage(fileName));
}
