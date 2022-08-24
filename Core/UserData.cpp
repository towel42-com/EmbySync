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

#include <QRegularExpression>


CUserData::CUserData( const QString & serverName, const QString & name, const QString & connectedID, const QString & userID ) :
    fConnectedID( connectedID )
{
    setName( serverName, name );
    setUserID( serverName, userID );
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
    if ( !name.isEmpty() )
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

bool CUserData::connectIDNeedsUpdate() const
{
    if ( fConnectedID.isEmpty() )
        return false;
    auto regExpStr = R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))";
    //auto regExpStr = R"(/(?(DEFINE)(?<address>(?&mailbox) | (?&group)) (? (?&name_addr) | (?&addr_spec)) (? (?&display_name)? (?&angle_addr)) (? (?&CFWS)? < (?&addr_spec) > (?&CFWS)?) (? (?&display_name) : (?:(?&mailbox_list) | (?&CFWS))? ; (?&CFWS)?) (? (?&phrase)) (? (?&mailbox) (?: , (?&mailbox))*) (? (?&local_part) \@ (?&domain)) (? (?&dot_atom) | (?&quoted_string)) (? (?&dot_atom) | (?&domain_literal)) (? (?&CFWS)? \[ (?: (?&FWS)? (?&dcontent))* (?&FWS)? \] (?&CFWS)?) (? (?&dtext) | (?&quoted_pair)) (? (?&NO_WS_CTL) | [\x21-\x5a\x5e-\x7e]) (? (?&ALPHA) | (?&DIGIT) | [!#\$%&'*+-/=?^_`{|}~]) (? (?&CFWS)? (?&atext)+ (?&CFWS)?) (? (?&CFWS)? (?&dot_atom_text) (?&CFWS)?) (? (?&atext)+ (?: \. (?&atext)+)*) (? [\x01-\x09\x0b\x0c\x0e-\x7f]) (? \\ (?&text)) (? (?&NO_WS_CTL) | [\x21\x23-\x5b\x5d-\x7e]) (? (?&qtext) | (?&quoted_pair)) (? (?&CFWS)? (?&DQUOTE) (?:(?&FWS)? (?&qcontent))* (?&FWS)? (?&DQUOTE) (?&CFWS)?) (? (?&atom) | (?&quoted_string)) (? (?&word)+) # Folding white space (? (?: (?&WSP)* (?&CRLF))? (?&WSP)+) (? (?&NO_WS_CTL) | [\x21-\x27\x2a-\x5b\x5d-\x7e]) (? (?&ctext) | (?&quoted_pair) | (?&comment)) (? \( (?: (?&FWS)? (?&ccontent))* (?&FWS)? \) ) (? (?: (?&FWS)? (?&comment))* (?: (?:(?&FWS)? (?&comment)) | (?&FWS))) # No whitespace control (? [\x01-\x08\x0b\x0c\x0e-\x1f\x7f]) (? [A-Za-z]) (? [0-9]) (? \x0d \x0a) (? ") (? [\x20\x09]) ) (?&address)/x</address>)";
    QRegularExpression regExp( regExpStr );
    Q_ASSERT( regExp.isValid() );
    auto match = regExp.match( fConnectedID );
    bool retVal = match.hasMatch() && ( match.capturedLength() == fConnectedID.length() );
    return !retVal;
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
