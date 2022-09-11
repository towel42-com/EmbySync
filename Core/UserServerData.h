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

#ifndef __USERSERVERDATA_H
#define __USERSERVERDATA_H

#include <QString>
#include <QJsonObject>
#include <QImage>
#include <QDateTime>
#include <utility>
#include <tuple>

struct SUserServerData
{
    bool isValid() const;
    bool userDataEqual( const SUserServerData & rhs ) const;

    static std::tuple< QString, QString, QString > getUserNames( const QJsonObject & userObj );
    QDateTime latestAccess() const;
    QJsonObject userDataJSON() const;
    void loadFromJSON( const QJsonObject & userObj );

    QString fName;
    QString fUserID;
    std::pair< QString, QString > fConnectedID;
    QString fPrefix;
    bool fEnableAutoLogin{ false };
    std::tuple< QString, double, QImage > fAvatarInfo;
    QDateTime fDateCreated;
    QDateTime fLastLoginDate;
    QDateTime fLastActivityDate;

    // configuration settings
    QString fAudioLanguagePreference;
    bool fPlayDefaultAudioTrack{ false };
    QString fSubtitleLanguagePreference;
    bool fDisplayMissingEpisodes{ false };
    QString fSubtitleMode;
    bool fEnableLocalPassword{ false };
    QStringList fOrderedViews;
    QStringList fLatestItemsExcludes;
    QStringList fMyMediaExcludes;
    bool fHidePlayedInLatest{ false };
    bool fRememberAudioSelections{ false };
    bool fRememberSubtitleSelections{ false };
    bool fEnableNextEpisodeAutoPlay{ false };
    int fResumeRewindSeconds{ 0 };
    QString fIntroSkipMode;
};

bool operator==( const SUserServerData & lhs, const SUserServerData & rhs );
inline bool operator!=( const SUserServerData & lhs, const SUserServerData & rhs )
{
    return !operator==( lhs, rhs );
}
#endif
