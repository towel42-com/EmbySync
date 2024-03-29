﻿// The MIT License( MIT )
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

#include "UserData.h"
#include "UserServerData.h"

#include "Settings.h"
#include "ServerInfo.h"
#include "ServerModel.h"
#include "SABUtils/StringUtils.h"

#include <QRegularExpression>
#include <QDebug>
#include <QJsonObject>

CUserData::CUserData( const QString &serverName, const QJsonObject &userObj )
{
    loadFromJSON( serverName, userObj );
}

std::shared_ptr< SUserServerData > CUserData::userInfo( const QString &serverName ) const
{
    auto pos = fInfoForServer.find( serverName );
    if ( pos == fInfoForServer.end() )
        return {};
    return ( *pos ).second;
}

std::shared_ptr< SUserServerData > CUserData::userInfo( const QString &serverName, bool addIfMissing )
{
    auto retVal = userInfo( serverName );
    if ( retVal || !addIfMissing )
        return retVal;

    retVal = std::make_shared< SUserServerData >();
    fInfoForServer[ serverName ] = retVal;
    return retVal;
}

void CUserData::setConnectedID( const QString &serverName, const QString &type, const QString &connectedID )
{
    auto retVal = userInfo( serverName );
    if ( retVal )
        retVal->fConnectedID = { type, connectedID };

    updateConnectedID();
}

void CUserData::updateConnectedID()
{
    auto tmp = allSame< std::pair< QString, QString > >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fConnectedID; } );

    if ( tmp.has_value() )
        fConnectedID = tmp.value();
    else
        fConnectedID = {};
}

bool CUserData::isValidForServer( const QString &serverName ) const
{
    auto info = userInfo( serverName );
    if ( !info )
        return false;
    return info->isValid();
}

bool CUserData::isValid() const
{
    if ( !fConnectedID.second.isEmpty() )
        return true;
    for ( auto &&ii : fInfoForServer )
    {
        if ( !ii.second->isValid() )
            return true;
    }
    return false;
}

QString CUserData::connectedID( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fConnectedID.second;
}

QString CUserData::connectedIDType( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fConnectedID.first;
}

QString CUserData::name( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fName;
}

void CUserData::setName( const QString &serverName, const QString &name )
{
    auto serverInfo = userInfo( serverName, true );
    serverInfo->fName = name;
    updateCanBeSynced();
}

QString CUserData::userName( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return connectedID();

    auto retVal = serverInfo->fName;
    if ( !connectedID().isEmpty() )
        retVal += "(" + connectedID() + ")";
    return retVal;
}

QString CUserData::allNames() const
{
    QString retVal = connectedID();

    QStringList serverNames;
    for ( auto &&ii : fInfoForServer )
    {
        if ( !ii.second->fName.isEmpty() && !serverNames.contains( ii.second->fName ) )
            serverNames << ii.second->fName;
    }

    if ( !serverNames.isEmpty() )
    {
        if ( !retVal.isEmpty() )
            retVal += "(" + serverNames.join( "," ) + ")";
        else
            retVal = serverNames.join( "," );
    }

    return retVal;
}

QString CUserData::sortName( std::shared_ptr< CServerModel > serverModel ) const
{
    if ( !fSortKey.isEmpty() )
        return fSortKey;

    if ( !connectedID().isEmpty() )
        return connectedID();

    for ( auto &&serverInfo : *serverModel )
    {
        if ( !serverInfo->isEnabled() )
            continue;
        auto info = userInfo( serverInfo->keyName() );
        if ( !info )
            continue;

        // if ( !info->fConnectedIDOnServer.isEmpty() )
        //     return info->fConnectedIDOnServer;

        if ( !info->fName.isEmpty() )
            return fSortKey = info->fName;
    }

    return {};
}

QString CUserData::prefix( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fPrefix;
}

bool CUserData::enableAutoLogin( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fEnableAutoLogin;
}

QString CUserData::audioLanguagePreference( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fAudioLanguagePreference;
}

bool CUserData::playDefaultAudioTrack( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fPlayDefaultAudioTrack;
}

QString CUserData::subtitleLanguagePreference( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fSubtitleLanguagePreference;
}

bool CUserData::displayMissingEpisodes( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fDisplayMissingEpisodes;
}

QString CUserData::subtitleMode( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fSubtitleMode;
}

bool CUserData::enableLocalPassword( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fEnableLocalPassword;
}

QStringList CUserData::orderedViews( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fOrderedViews;
}

QStringList CUserData::latestItemsExcludes( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fLatestItemsExcludes;
}

QStringList CUserData::myMediaExcludes( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fMyMediaExcludes;
}

bool CUserData::hidePlayedInLatest( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fHidePlayedInLatest;
}

bool CUserData::rememberAudioSelections( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fRememberAudioSelections;
}

bool CUserData::rememberSubtitleSelections( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fRememberAudioSelections;
}

bool CUserData::enableNextEpisodeAutoPlay( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fEnableNextEpisodeAutoPlay;
}

int CUserData::resumeRewindSeconds( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fResumeRewindSeconds;
}

QString CUserData::introSkipMode( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fIntroSkipMode;
}

bool CUserData::isAdmin( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fIsAdmin;
}

QStringList CUserData::missingServers() const
{
    QStringList retVal;
    for ( auto &&ii : fInfoForServer )
    {
        if ( ii.second->isValid() )
            continue;
        retVal << ii.first;
    }
    return retVal;
}

bool CUserData::isUser( const QRegularExpression &regEx ) const
{
    if ( !regEx.isValid() || regEx.pattern().isEmpty() )
        return false;

    if ( isMatch( regEx, fConnectedID.second ) )
        return true;
    for ( auto &&ii : fInfoForServer )
    {
        if ( isMatch( regEx, ii.second->fName ) )
            return true;
    }

    return false;
}

bool CUserData::isUser( const QString &name ) const
{
    if ( name.isEmpty() )
        return false;

    if ( connectedID() == name )
        return true;
    for ( auto &&ii : fInfoForServer )
    {
        if ( ii.second->fName == name )
            return true;
    }

    return allNames() == name;
}

bool CUserData::isUser( const QString &serverName, const QString &userID ) const
{
    auto pos = fInfoForServer.find( serverName );
    if ( pos == fInfoForServer.end() )
        return false;

    return ( *pos ).second->fUserID == userID;
}

bool CUserData::connectedIDNeedsUpdate() const
{
    return !fConnectedID.second.isEmpty() && !NSABUtils::NStringUtils::isValidEmailAddress( fConnectedID.second );
}

bool CUserData::isMatch( const QRegularExpression &regEx, const QString &value ) const
{
    auto match = regEx.match( value );
    return ( match.hasMatch() && ( match.captured( 0 ).length() == value.length() ) );
}

QString CUserData::getUserID( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fUserID;
}

void CUserData::loadFromJSON( const QString &serverName, const QJsonObject &userObj )
{
    auto retVal = userInfo( serverName, true );
    if ( retVal )
        retVal->loadFromJSON( userObj );
    updateCanBeSynced();
    updateConnectedID();
}

void CUserData::setUserID( const QString &serverName, const QString &id )
{
    auto serverInfo = userInfo( serverName, true );
    serverInfo->fUserID = id;
    updateCanBeSynced();
}

std::tuple< QString, double, QImage > CUserData::getAvatarInfo( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};

    return serverInfo->fAvatarInfo;
}

bool CUserData::hasAvatarInfo( const QString &serverName ) const
{
    return !std::get< 0 >( getAvatarInfo( serverName ) ).isEmpty();
}

void CUserData::setAvatarInfo( const QString &serverName, const QString &tag, double ratio )
{
    auto serverInfo = userInfo( serverName, true );
    serverInfo->fAvatarInfo = { tag, ratio, {} };
}

QImage CUserData::globalAvatar() const   // when all servers use the same image
{
    return fGlobalImage.has_value() ? fGlobalImage.value().scaled( QSize( 32, 32 ) ) : QImage();
}

QImage CUserData::anyAvatar() const
{
    for ( auto &&ii : fInfoForServer )
    {
        if ( !std::get< 2 >( ii.second->fAvatarInfo ).isNull() )
            return std::get< 2 >( ii.second->fAvatarInfo );
    }
    return {};
}

QDateTime CUserData::getDateCreated( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo || !serverInfo->fDateCreated.isValid() )
        return {};
    return serverInfo->fDateCreated;
}

QDateTime CUserData::getLastActivityDate( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo || !serverInfo->fLastActivityDate.isValid() )
        return {};
    return serverInfo->fLastActivityDate;
}

QDateTime CUserData::getLastLoginDate( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo || !serverInfo->fLastLoginDate.isValid() )
        return {};
    return serverInfo->fLastLoginDate;
}

void CUserData::checkAllAvatarsTheSame( int serverNum )
{
    if ( fGlobalImage.has_value() )
        return;
    if ( serverNum != this->fInfoForServer.size() )
        return;

    fGlobalImage = allSame< QImage >( []( std::shared_ptr< SUserServerData > rhs ) { return std::get< 2 >( rhs->fAvatarInfo ); } );
}

bool CUserData::allUserDataTheSame() const
{
    // std::map< QString, std::shared_ptr< SUserServerData > >
    auto &&ii = fInfoForServer.cbegin();
    auto lhs = ( *ii ).second;
    auto jj = fInfoForServer.cbegin();
    ++jj;
    for ( ; jj != fInfoForServer.cend(); ++jj )
    {
        auto rhs = ( *jj ).second;
        if ( lhs == rhs )
            continue;

        if ( !lhs->userDataEqual( *rhs ) )
            return false;
    }
    return true;
}

bool CUserData::allUserNamesTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fName; } ).has_value();
}

bool CUserData::allPrefixTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fPrefix; } ).has_value();
}

bool CUserData::allEnableAutoLoginTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fEnableAutoLogin; } ).has_value();
}

bool CUserData::allConnectIDTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fConnectedID.second; } ).has_value();
}

bool CUserData::allConnectIDTypeTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fConnectedID.first; } ).has_value();
}

bool CUserData::allIconInfoTheSame() const
{
    return allSame< std::pair< double, QImage > >( []( std::shared_ptr< SUserServerData > rhs ) { return std::make_pair( std::get< 1 >( rhs->fAvatarInfo ), std::get< 2 >( rhs->fAvatarInfo ) ); } ).has_value();
}

bool CUserData::allDateCreatedSame() const
{
    return allSame< QDateTime >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fDateCreated; } ).has_value();
}

bool CUserData::allLastActivityDateSame() const
{
    return allSame< QDateTime >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fLastActivityDate; } ).has_value();
}

bool CUserData::allLastLoginDateSame() const
{
    return allSame< QDateTime >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fLastLoginDate; } ).has_value();
}

bool CUserData::allAudioLanguagePreferenceTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fAudioLanguagePreference; } ).has_value();
}

bool CUserData::allPlayDefaultAudioTrackTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fPlayDefaultAudioTrack; } ).has_value();
}

bool CUserData::allSubtitleLanguagePreferenceTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fSubtitleLanguagePreference; } ).has_value();
}

bool CUserData::allDisplayMissingEpisodesTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fDisplayMissingEpisodes; } ).has_value();
}

bool CUserData::allSubtitleModeTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fSubtitleMode; } ).has_value();
}

bool CUserData::allEnableLocalPasswordTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fEnableLocalPassword; } ).has_value();
}

bool CUserData::allOrderedViewsTheSame() const
{
    return allSame< QStringList >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fOrderedViews; } ).has_value();
}

bool CUserData::allLatestItemsExcludesTheSame() const
{
    return allSame< QStringList >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fLatestItemsExcludes; } ).has_value();
}

bool CUserData::allMyMediaExcludesTheSame() const
{
    return allSame< QStringList >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fMyMediaExcludes; } ).has_value();
}

bool CUserData::allHidePlayedInLatestTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fHidePlayedInLatest; } ).has_value();
}

bool CUserData::allRememberAudioSelectionsTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fRememberAudioSelections; } ).has_value();
}

bool CUserData::allRememberSubtitleSelectionsTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fRememberSubtitleSelections; } ).has_value();
}

bool CUserData::allEnableNextEpisodeAutoPlayTheSame() const
{
    return allSame< bool >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fEnableNextEpisodeAutoPlay; } ).has_value();
}

bool CUserData::allResumeRewindSecondsTheSame() const
{
    return allSame< int >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fResumeRewindSeconds; } ).has_value();
}

bool CUserData::allIntroSkipModeTheSame() const
{
    return allSame< QString >( []( std::shared_ptr< SUserServerData > rhs ) { return rhs->fIntroSkipMode; } ).has_value();
}

bool CUserData::needsUpdating( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo != newestUserInfo();
}

std::shared_ptr< SUserServerData > CUserData::newestUserInfo() const
{
    std::shared_ptr< SUserServerData > retVal;
    for ( auto &&ii : fInfoForServer )
    {
        if ( !ii.second )
            continue;

        if ( !retVal )
            retVal = ii.second;
        else
        {
            if ( ii.second->latestAccess() > retVal->latestAccess() )
                retVal = ii.second;
        }
    }
    return retVal;
}

QImage CUserData::getAvatar( const QString &serverName, bool useUnset ) const
{
    QImage retVal;
    if ( fGlobalImage.has_value() )
        retVal = fGlobalImage.value();
    else
    {
        auto serverInfo = userInfo( serverName );
        if ( !serverInfo || std::get< 2 >( serverInfo->fAvatarInfo ).isNull() )
            retVal = useUnset ? QImage( ":/resources/missingAvatar.png" ) : QImage();
        else
            retVal = std::get< 2 >( serverInfo->fAvatarInfo );
    }
    if ( !retVal.isNull() )
        retVal = retVal.scaled( QSize( 32, 32 ) );
    return retVal;
}

void CUserData::setAvatar( const QString &serverName, int serverCnt, const QImage &image )
{
    auto serverInfo = userInfo( serverName, true );
    std::get< 2 >( serverInfo->fAvatarInfo ) = image;

    checkAllAvatarsTheSame( serverCnt );
}

bool CUserData::onServer( const QString &serverName ) const
{
    auto serverInfo = userInfo( serverName );
    return serverInfo != nullptr;
}

bool CUserData::canBeSynced() const
{
    return fCanBeSynced;
}

void CUserData::updateCanBeSynced()
{
    int serverCnt = 0;
    for ( auto &&ii : fInfoForServer )
    {
        if ( ii.second->isValid() )
            serverCnt++;
    }
    fCanBeSynced = serverCnt > 1;
}

bool CUserData::validUserDataEqual() const
{
    std::list< std::pair< QString, std::shared_ptr< SUserServerData > > > validServerData;
    for ( auto &&ii : fInfoForServer )
    {
        if ( ii.second->isValid() )
            validServerData.push_back( ii );
    }

    auto pos = validServerData.begin();
    auto nextPos = validServerData.begin();
    nextPos++;
    for ( ; ( pos != validServerData.end() ) && ( nextPos != validServerData.end() ); ++pos, ++nextPos )
    {
        if ( !( *pos ).second->userDataEqual( *( ( *nextPos ).second ) ) )
            return false;
    }
    return true;
}

QIcon CUserData::getDirectionIcon( const QString &serverName ) const
{
    static QIcon sErrorIcon( ":/resources/error.png" );
    static QIcon sEqualIcon( ":/resources/equal.png" );
    static QIcon sArrowUpIcon( ":/resources/arrowup.png" );
    static QIcon sArrowDownIcon( ":/resources/arrowdown.png" );
    QIcon retVal;
    if ( !isValidForServer( serverName ) )
        retVal = sErrorIcon;
    else if ( !canBeSynced() )
        return {};
    else if ( validUserDataEqual() )
        retVal = sEqualIcon;
    else if ( needsUpdating( serverName ) )
        retVal = sArrowDownIcon;
    else
        retVal = sArrowUpIcon;

    return retVal;
}
