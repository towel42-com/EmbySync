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

#include "UserData.h"
#include "Settings.h"
#include "ServerInfo.h"
#include "SABUtils/StringUtils.h"

#include <QRegularExpression>

CUserData::CUserData( const QString & serverName, const QString & name, const QString & connectedID, const QString & userID ) :
    fConnectedID( connectedID )
{
    setName( serverName, name );
    setUserID( serverName, userID );
    setConnectedID( serverName, connectedID );
}

std::shared_ptr< SUserServerData > CUserData::getServerInfo( const QString & serverName ) const
{
    auto pos = fInfoForServer.find( serverName );
    if ( pos == fInfoForServer.end() )
        return {};
    return ( *pos ).second;
}

std::shared_ptr< SUserServerData > CUserData::getServerInfo( const QString & serverName, bool addIfMissing )
{
    auto retVal = getServerInfo( serverName );
    if ( retVal || !addIfMissing )
        return retVal;

    retVal = std::make_shared< SUserServerData >();
    fInfoForServer[ serverName ] = retVal;
    return retVal;
}

void CUserData::setConnectedID( const QString & serverName, const QString & connectedID )
{
    fConnectedID = connectedID;
    auto retVal = getServerInfo( serverName );
    if ( retVal )
        retVal->fConnectedIDOnServer = connectedID;
}

bool CUserData::isValid() const
{
    if ( !fConnectedID.isEmpty() )
        return true;
    for ( auto && ii : fInfoForServer )
    {
        if ( !ii.second->isValid() )
            return true;
    }
    return false;
}

QString CUserData::connectedID( const QString & serverName ) const
{
    auto serverInfo = getServerInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fConnectedIDOnServer;
}

QString CUserData::name( const QString & serverName ) const
{
    auto serverInfo = getServerInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fName;
}

void CUserData::setName( const QString & serverName, const QString & name )
{
    auto serverInfo = getServerInfo( serverName, true );
    serverInfo->fName = name;
    updateCanBeSynced();
}

QString CUserData::userName( const QString & serverName ) const
{
    auto serverInfo = getServerInfo( serverName );
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
    for ( auto && ii : fInfoForServer )
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

QString CUserData::sortName( std::shared_ptr< CSettings > settings ) const
{
    if ( !connectedID().isEmpty() )
        return connectedID();
    for ( int ii = 0; ii < settings->serverCnt(); ++ii )
    {
        auto info = getServerInfo( settings->serverInfo( ii )->keyName() );
        if ( !info )
            continue;
        if ( info->fName.isEmpty() )
            continue;
        return info->fName;
    }

    return {};
}

QStringList CUserData::missingServers() const
{
    QStringList retVal;
    for ( auto && ii : fInfoForServer )
    {
        if ( ii.second->isValid() )
            continue;
        retVal << ii.first;
    }
    return retVal;
}

bool CUserData::isUser( const QRegularExpression & regEx ) const
{
    if ( !regEx.isValid() || regEx.pattern().isEmpty() )
        return false;

    if ( isMatch( regEx, fConnectedID ) )
        return true;
    for ( auto && ii : fInfoForServer )
    {
        if ( isMatch( regEx, ii.second->fName ) )
            return true;
    }

    return false;
}

bool CUserData::isUser( const QString & name ) const
{
    if ( name.isEmpty() )
        return false;

    if ( connectedID() == name )
        return true;
    for ( auto && ii : fInfoForServer )
    {
        if ( ii.second->fName == name )
            return true;
    }

    return allNames() == name;
}

bool CUserData::isUser( const QString & serverName, const QString & userID ) const
{
    auto pos = fInfoForServer.find( serverName );
    if ( pos == fInfoForServer.end() )
        return false;

    return ( *pos ).second->fUserID == userID;
}

bool CUserData::connectedIDNeedsUpdate() const
{
    return !fConnectedID.isEmpty() && !NSABUtils::NStringUtils::isValidEmailAddress( fConnectedID );
}

bool CUserData::isMatch( const QRegularExpression & regEx, const QString & value ) const
{
    auto match = regEx.match( value );
    return ( match.hasMatch() && ( match.captured( 0 ).length() == value.length() ) );
}

QString CUserData::getUserID( const QString & serverName ) const
{
    auto serverInfo = getServerInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fUserID;
}

void CUserData::setUserID( const QString & serverName, const QString & id )
{
    auto serverInfo = getServerInfo( serverName, true );
    serverInfo->fUserID = id;
    updateCanBeSynced();
}

bool CUserData::hasImageTagInfo( const QString & serverName ) const
{
    auto serverInfo = getServerInfo( serverName );
    if ( !serverInfo )
        return false;
    return !serverInfo->fImageTagInfo.first.isEmpty();
}

std::pair< QString, double > CUserData::getImageTagInfo( const QString & serverName ) const
{
    auto serverInfo = getServerInfo( serverName );
    if ( !serverInfo )
        return {};

    return serverInfo->fImageTagInfo;
}

void CUserData::setImageTagInfo( const QString & serverName, const QString & tag, double ratio )
{
    auto serverInfo = getServerInfo( serverName, true );
    serverInfo->fImageTagInfo = { tag, ratio };
}

QImage CUserData::globalAvatar() const // when all servers use the same image
{
    return fImage.has_value() ? fImage.value().scaled( QSize( 32, 32 ) ) : QImage();
}

QImage CUserData::anyAvatar() const
{
    for ( auto && ii : fInfoForServer )
    {
        if ( !ii.second->fImage.isNull() )
            return ii.second->fImage;
    }
    return {};
}

void CUserData::checkAllAvatarsTheSame( int serverNum )
{
    if ( fImage.has_value() )
        return;
    if ( serverNum != this->fInfoForServer.size() )
        return;

    std::optional< QImage > prev;
    for ( auto && ii : fInfoForServer )
    {
        if ( prev.has_value() )
        {
            if ( prev.value() != ii.second->fImage )
                return;
        }
        else
            prev = ii.second->fImage;
    }
    if ( prev.has_value() )
        fImage = prev;
}

QImage CUserData::getAvatar( const QString & serverName ) const
{
    if ( fImage.has_value() )
    {
        return fImage.value().scaled( QSize( 32, 32 ) );
    }

    auto serverInfo = getServerInfo( serverName );
    if ( !serverInfo )
        return {};
    return serverInfo->fImage.scaled( QSize( 32,32 ) );
}

void CUserData::setAvatar( const QString & serverName, int serverCnt, const QImage & image )
{
    auto serverInfo = getServerInfo( serverName, true );
    serverInfo->fImage = image;

    checkAllAvatarsTheSame( serverCnt );
}

bool CUserData::onServer( const QString & serverName ) const
{
    auto serverInfo = getServerInfo( serverName );
    return serverInfo != nullptr;
}

bool CUserData::canBeSynced() const
{
    return fCanBeSynced;
}

void CUserData::updateCanBeSynced()
{
    int serverCnt = 0;
    for ( auto && ii : fInfoForServer )
    {
        if ( ii.second->isValid() )
            serverCnt++;
    }
    fCanBeSynced = serverCnt > 1;
}

bool SUserServerData::isValid() const
{
    return !fName.isEmpty() && !fUserID.isEmpty();
}
