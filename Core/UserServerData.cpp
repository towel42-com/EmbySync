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
#include <QJsonDocument>

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
    return obj;
}

void SUserServerData::loadFromJSON( const QJsonObject & userObj )
{
    qDebug().noquote().nospace() << QJsonDocument( userObj ).toJson();

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
    equal = equal && fConnectedID == rhs.fConnectedID;
    if ( !fDateCreated.isNull() && !rhs.fDateCreated.isNull() )
        equal = equal && fDateCreated == rhs.fDateCreated;
    if ( !fLastActivityDate.isNull() && !rhs.fLastActivityDate.isNull() )
        equal = equal && fLastActivityDate == rhs.fLastActivityDate;
    if ( !fLastLoginDate.isNull() && !rhs.fLastLoginDate.isNull() )
        equal = equal && fLastLoginDate == rhs.fLastLoginDate;
    equal = equal && std::get< 1 >( fAvatarInfo ) == std::get< 1 >( rhs.fAvatarInfo );
    equal = equal && std::get< 2 >( fAvatarInfo ) == std::get< 2 >( rhs.fAvatarInfo );
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
