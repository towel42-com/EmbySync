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

#include "UserServerData.h"
#include "SABUtils/JsonUtils.h"
#include <QJsonDocument>
#include <QDebug>

//UserDto{
//Name	string
//ServerId	string
//ServerName	string
//Prefix	string
//ConnectUserName	string
//DateCreated	string( $date - time, nullable )
//ConnectLinkType	string( $enum )( LinkedUser, Guest )
//Id	string( $guid )
//PrimaryImageTag	string
//HasPassword	boolean
//HasConfiguredPassword	boolean
//HasConfiguredEasyPassword	boolean
//EnableAutoLogin	boolean( nullable )
//LastLoginDate	string( $date - time, nullable )
//LastActivityDate	string( $date - time, nullable )
//Configuration	Configuration.UserConfiguration{...}
//Policy	Users.UserPolicy{...}
//PrimaryImageAspectRatio	number( $double, nullable )
//}

QJsonObject SUserServerData::userDataJSON() const
{
    QJsonObject obj;
    obj[ "Name" ] = fName;
    if ( !fConnectedID.second.isEmpty() && !fConnectedID.first.isEmpty() )
    {
        obj[ "ConnectedUserName" ] = fConnectedID.second;
        obj[ "ConnectLinkType" ] = fConnectedID.first;
    }
    obj[ "Prefix" ] = fPrefix;
    obj[ "EnableAutoLogin" ] = fEnableAutoLogin;

    if ( fDateCreated.isNull() )
        obj[ "DateCreated" ] = QJsonValue::Null;
    else
        obj[ "DateCreated" ] = fDateCreated.toUTC().toString( Qt::ISODateWithMs );

    if ( fLastLoginDate.isNull() )
        obj[ "LastLoginDate" ] = QJsonValue::Null;
    else
        obj[ "LastLoginDate" ] = fLastLoginDate.toUTC().toString( Qt::ISODateWithMs );

    if ( fLastActivityDate.isNull() )
        obj[ "LastActivityDate" ] = QJsonValue::Null;
    else
        obj[ "LastActivityDate" ] = fLastActivityDate.toUTC().toString( Qt::ISODateWithMs );
    obj[ "PrimaryImageAspectRatio" ] = std::get< 1 >( fAvatarInfo );

//Configuration.UserConfiguration{
//AudioLanguagePreference	string
//PlayDefaultAudioTrack	boolean
//SubtitleLanguagePreference	string
//DisplayMissingEpisodes	boolean
//SubtitleMode	string( $enum )( Default, Always, OnlyForced, None, Smart )
//EnableLocalPassword	boolean
//OrderedViews[ string ]
//LatestItemsExcludes[ string ]
//MyMediaExcludes[ string ]
//HidePlayedInLatest	boolean
//RememberAudioSelections	boolean
//RememberSubtitleSelections	boolean
//EnableNextEpisodeAutoPlay	boolean
//ResumeRewindSeconds	integer( $int32 )
//IntroSkipMode	string( $enum )( ShowButton, AutoSkip, None )
//}
    QJsonObject configObj;

    configObj[ "AudioLanguagePreference" ] = fAudioLanguagePreference;
    configObj[ "PlayDefaultAudioTrack" ] = fPlayDefaultAudioTrack;
    configObj[ "SubtitleLanguagePreference" ] = fSubtitleLanguagePreference;
    configObj[ "DisplayMissingEpisodes" ] = fDisplayMissingEpisodes;
    configObj[ "SubtitleMode" ] = fSubtitleMode;
    configObj[ "EnableLocalPassword" ] = fEnableLocalPassword;
    NSABUtils::ToJson( fOrderedViews, configObj[ "OrderedViews" ] );
    NSABUtils::ToJson( fLatestItemsExcludes, configObj[ "LatestItemsExcludes" ] );
    NSABUtils::ToJson( fMyMediaExcludes, configObj[ "MyMediaExcludes" ] );
    configObj[ "HidePlayedInLatest" ] = fHidePlayedInLatest;
    configObj[ "RememberAudioSelections" ] = fRememberAudioSelections;
    configObj[ "RememberSubtitleSelections" ] = fRememberSubtitleSelections;
    configObj[ "EnableNextEpisodeAutoPlay" ] = fEnableNextEpisodeAutoPlay;
    configObj[ "ResumeRewindSeconds" ] = fResumeRewindSeconds;
    configObj[ "IntroSkipMode" ] = fIntroSkipMode;

    obj[ "Configuration" ] = configObj;

    return obj;
}

void SUserServerData::loadFromJSON( const QJsonObject & userObj )
{
    //qDebug().noquote().nospace() << QJsonDocument( userObj ).toJson();

    fName = userObj[ "Name" ].toString();
    fUserID = userObj[ "Id" ].toString();

    fPrefix = userObj[ "Prefix" ].toString();
    fEnableAutoLogin = userObj[ "EnableAutoLogin" ].toBool();

    fConnectedID = { userObj[ "ConnectLinkType" ].toString(), userObj[ "ConnectUserName" ].toString() };
    auto dateCreated = userObj[ "DateCreated" ].toVariant().toDateTime();
    if ( dateCreated == QDateTime::fromString( "0001-01-01T00:00:00.000Z", Qt::ISODateWithMs ) )
        dateCreated = QDateTime();
    fDateCreated = dateCreated;
    fLastActivityDate = userObj[ "LastActivityDate" ].toVariant().toDateTime();
    fLastLoginDate = userObj[ "LastLoginDate" ].toVariant().toDateTime();
    std::get< 0 >( fAvatarInfo ) = userObj[ "PrimaryImageTag" ].toString();
    std::get< 1 >( fAvatarInfo ) = userObj[ "PrimaryImageAspectRatio" ].toDouble();

//Configuration.UserConfiguration{
//AudioLanguagePreference	string
//PlayDefaultAudioTrack	boolean
//SubtitleLanguagePreference	string
//DisplayMissingEpisodes	boolean
//SubtitleMode	string( $enum )( Default, Always, OnlyForced, None, Smart )
//EnableLocalPassword	boolean
//OrderedViews[ string ]
//LatestItemsExcludes[ string ]
//MyMediaExcludes[ string ]
//HidePlayedInLatest	boolean
//RememberAudioSelections	boolean
//RememberSubtitleSelections	boolean
//EnableNextEpisodeAutoPlay	boolean
//ResumeRewindSeconds	integer( $int32 )
//IntroSkipMode	string( $enum )( ShowButton, AutoSkip, None )
//}

    auto config = userObj[ "Configuration" ].toObject();
    //qDebug().noquote().nospace() << QJsonDocument( config ).toJson();
    fAudioLanguagePreference = config[ "AudioLanguagePreference" ].toString();
    fPlayDefaultAudioTrack = config[ "PlayDefaultAudioTrack" ].toBool();
    fSubtitleLanguagePreference = config[ "SubtitleLanguagePreference" ].toString();
    fDisplayMissingEpisodes = config[ "DisplayMissingEpisodes" ].toBool();
    fSubtitleMode = config[ "SubtitleMode" ].toString();
    fEnableLocalPassword = config[ "EnableLocalPassword" ].toBool();
    NSABUtils::FromJson( fOrderedViews, config[ "OrderedViews" ] );
    NSABUtils::FromJson( fLatestItemsExcludes, config[ "LatestItemsExcludes" ] );
    NSABUtils::FromJson( fMyMediaExcludes, config[ "MyMediaExcludes" ] );
    fHidePlayedInLatest = config[ "HidePlayedInLatest" ].toBool();
    fRememberAudioSelections = config[ "RememberAudioSelections" ].toBool();
    fRememberSubtitleSelections = config[ "RememberSubtitleSelections" ].toBool();
    fEnableNextEpisodeAutoPlay = config[ "EnableNextEpisodeAutoPlay" ].toBool();
    fResumeRewindSeconds = config[ "ResumeRewindSeconds" ].toInt();
    fIntroSkipMode = config[ "IntroSkipMode" ].toString();

    auto policy = userObj[ "Policy" ].toObject();
    //qDebug().noquote().nospace() << QJsonDocument( policy ).toJson();
    fIsAdmin = policy[ "IsAdministrator" ].toBool();
    fIsDisabled = policy[ "IsDisabled" ].toBool();
    fIsHidden = policy[ "IsHidden" ].toBool();
}

bool SUserServerData::isValid() const
{
    return !fName.isEmpty() && !fUserID.isEmpty();
}

bool SUserServerData::userDataEqual( const SUserServerData & rhs ) const
{
    if ( isValid() != rhs.isValid() )
        return false;

    auto equal = true;
    equal = equal && fName == rhs.fName;
    // user id is not checked
    equal = equal && fConnectedID == rhs.fConnectedID;
    equal = equal && fPrefix == rhs.fPrefix;
    equal = equal && fEnableAutoLogin == rhs.fEnableAutoLogin;
    
    // avatar infos image id is not checked
    equal = equal && std::get< 1 >( fAvatarInfo ) == std::get< 1 >( rhs.fAvatarInfo );
    equal = equal && std::get< 2 >( fAvatarInfo ) == std::get< 2 >( rhs.fAvatarInfo );

    if ( !fDateCreated.isNull() && !rhs.fDateCreated.isNull() )
        equal = equal && fDateCreated == rhs.fDateCreated;
    if ( !fLastActivityDate.isNull() && !rhs.fLastActivityDate.isNull() )
        equal = equal && fLastActivityDate == rhs.fLastActivityDate;
    if ( !fLastLoginDate.isNull() && !rhs.fLastLoginDate.isNull() )
        equal = equal && fLastLoginDate == rhs.fLastLoginDate;

    equal = equal && fAudioLanguagePreference == rhs.fAudioLanguagePreference;
    equal = equal && fPlayDefaultAudioTrack == rhs.fPlayDefaultAudioTrack;
    equal = equal && fSubtitleLanguagePreference == rhs.fSubtitleLanguagePreference;
    equal = equal && fDisplayMissingEpisodes == rhs.fDisplayMissingEpisodes;
    equal = equal && fSubtitleMode == rhs.fSubtitleMode;
    equal = equal && fEnableLocalPassword == rhs.fEnableLocalPassword;
    equal = equal && fOrderedViews == rhs.fOrderedViews;
    equal = equal && fLatestItemsExcludes == rhs.fLatestItemsExcludes;
    equal = equal && fMyMediaExcludes == rhs.fMyMediaExcludes;
    equal = equal && fHidePlayedInLatest == rhs.fHidePlayedInLatest;
    equal = equal && fRememberAudioSelections == rhs.fRememberAudioSelections;
    equal = equal && fRememberSubtitleSelections == rhs.fRememberSubtitleSelections;
    equal = equal && fEnableNextEpisodeAutoPlay == rhs.fEnableNextEpisodeAutoPlay;
    equal = equal && fResumeRewindSeconds == rhs.fResumeRewindSeconds;
    equal = equal && fIntroSkipMode == rhs.fIntroSkipMode;

    equal = equal && fIsAdmin == rhs.fIsAdmin;
    equal = equal && fIsDisabled == rhs.fIsDisabled;
    equal = equal && fIsHidden == rhs.fIsHidden;

    return equal;
}

std::tuple< QString, QString, QString > SUserServerData::getUserNames( const QJsonObject & userObj )
{
    auto name = userObj[ "Name" ].toString();
    auto userID = userObj[ "Id" ].toString();

    auto linkType = userObj[ "ConnectLinkType" ].toString();
    QString connectedID;
    if ( linkType == "LinkedUser" )
        connectedID = userObj[ "ConnectUserName" ].toString();
    return std::make_tuple( name, userID, connectedID );
}

QDateTime SUserServerData::latestAccess() const
{
    return std::max( { fDateCreated, fLastLoginDate, fLastActivityDate } );
}

bool operator==( const SUserServerData & lhs, const SUserServerData & rhs )
{
    return lhs.userDataEqual( rhs );
}
