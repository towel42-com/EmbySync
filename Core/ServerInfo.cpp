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

#include "ServerInfo.h"
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QPixmap>

CServerInfo::CServerInfo( const QString & name, const QString & url, const QString & apiKey, bool enabled ) :
    fName( { name, false } ),
    fURL( url ),
    fAPIKey( apiKey ),
    fIsEnabled( enabled )
{

}

CServerInfo::CServerInfo( const QString & name ) :
    fName( { name, false } )
{

}

bool CServerInfo::operator==( const CServerInfo & rhs ) const
{
    return fName == rhs.fName
        && fURL == rhs.fURL
        && fAPIKey == rhs.fAPIKey
        && fIsEnabled == rhs.fIsEnabled
        ;
}

bool CServerInfo::setUrl( const QString & url )
{
    if ( url == fURL )
        return false;
    fURL = url;
    return true;
}

QString CServerInfo::url( bool clean ) const
{
    auto retVal = fURL;
    if ( clean )
    {
        if ( retVal.isEmpty() )
            return {};

        if ( retVal.indexOf( "://" ) == -1 )
            retVal = "http://" + retVal;
    }
    return retVal;
}

QUrl CServerInfo::getUrl() const
{
    return getUrl( QString(), std::list< std::pair< QString, QString > >() );
}

QUrl CServerInfo::getUrl( const QString & extraPath, const std::list< std::pair< QString, QString > > & queryItems ) const
{
    if ( extraPath.isEmpty() && queryItems.empty() )
    {
        if ( fDefaultURL.has_value() )
            return fDefaultURL.value();
        auto path = url( true );
        fDefaultURL = QUrl( path );
        return fDefaultURL.value();
    }
    auto path = url( true );

    if ( !extraPath.isEmpty() )
    {
        if ( !path.endsWith( "/" ) )
            path += "/";
        path += extraPath;
    }

    auto retVal = QUrl( path );

    QUrlQuery query;
    for ( auto && ii : queryItems )
    {
        query.addQueryItem( ii.first, ii.second );
    }
    query.addQueryItem( "api_key", fAPIKey );
    retVal.setQuery( query );

    //qDebug() << retVal;

    return retVal;
}

QString CServerInfo::displayName( bool verbose ) const
{
    QString name;

    if ( !fServerName.isEmpty() )
        name = fServerName;
    else if ( !fName.first.isEmpty() )
        name = fName.first;
    else 
        name = keyName();
    if ( !verbose )
        return name;
    name += tr( " - %1" ).arg( getUrl().toString( QUrl::RemoveUserInfo | QUrl::RemoveQuery ) );
    return name;
}

QString CServerInfo::keyName() const
{
    if ( fKeyName.isEmpty() )
        fKeyName = getUrl().toString( QUrl::RemoveUserInfo | QUrl::RemoveQuery );
    return fKeyName;
}

bool CServerInfo::isServer( const QString & serverName ) const
{
    if ( keyName() == serverName )
        return true;
    if ( displayName( false ) == serverName )
        return true;
    if ( displayName( true ) == serverName )
        return true;
    if ( !fServerName.isEmpty() && ( fServerName == serverName ) )
        return true;
    if ( !fName.first.isEmpty() && ( fName.first == serverName ) )
         return true;
    if ( getUrl().host() == serverName )
        return true;
    return false;
}

void CServerInfo::autoSetDisplayName( bool usePort )
{
    auto url = getUrl();
    QString retVal = url.host();
    if ( usePort )
        retVal += QString::number( url.port() );
    setDisplayName( retVal, true );
}

bool CServerInfo::setDisplayName( const QString & name, bool generated )
{
    if ( ( name == fName.first ) && ( generated == fName.second ) )
        return false;
    fName = { name, generated };
    return true;
}

bool CServerInfo::canSync() const
{
    return getUrl().isValid() && !fAPIKey.isEmpty();
}

bool CServerInfo::setAPIKey( const QString & key )
{
    if ( key != fAPIKey )
        return false;
    fAPIKey = key;
    return true;
}

QJsonObject CServerInfo::toJson() const
{
    QJsonObject retVal;

    retVal[ "url" ] = fURL;
    retVal[ "api_key" ] = fAPIKey;
    if ( !fName.second )
        retVal[ "name" ] = fName.first;
    retVal[ "enabled" ] = fIsEnabled;
    return retVal;
}

std::shared_ptr< CServerInfo > CServerInfo::fromJson( const QJsonObject & obj, QString & errorMsg )
{
    bool generated = !obj.contains( "name" ) || obj[ "name" ].toString().isEmpty();
    QString serverName;
    if ( generated )
        serverName = obj[ "url" ].toString();
    else
        serverName = obj[ "name" ].toString();

    if ( serverName.isEmpty() )
    {
        errorMsg = QString( "Missing name and url" );
        return false;
    }

    if ( !obj.contains( "url" ) )
    {
        errorMsg = QString( "Missing url" );
        return false;
    }

    if ( !obj.contains( "api_key" ) )
    {
        errorMsg = QString( "Missing api_key" );
        return false;
    }
    bool enabled = true;
    if ( obj.contains( "enabled" ) )
        enabled = obj[ "enabled" ].toBool();

    auto retVal = std::make_shared< CServerInfo >( serverName, obj[ "url" ].toString(), obj[ "api_key" ].toString(), enabled );
    retVal->fName.second = generated;

    return retVal;
}

bool CServerInfo::setIsEnabled( bool isEnabled )
{
    if ( fIsEnabled == isEnabled )
        return false;
    fIsEnabled = isEnabled;
    return true;
}

void CServerInfo::update( const QJsonObject & serverData )
{
    fLocalAddress = serverData[ "LocalAddress" ].toString();
    auto addresses = serverData[ "LocalAddresses" ].toArray();
    for ( auto && ii : addresses )
        fLocalAddresses.push_back( ii.toString() );
    fWANAddress = serverData[ "WANAddress" ].toString();
    addresses = serverData[ "WANAddresses" ].toArray();
    for ( auto && ii : addresses )
        fWANAddresses.push_back( ii.toString() );

    fServerName = serverData[ "ServerName" ].toString();
    fVersion = serverData[ "Version" ].toString();
    fID = serverData[ "Id" ].toString();

    emit sigServerInfoChanged();
}

void CServerInfo::setIcon( const QByteArray & data, const QString & type )
{
    (void)type;
    QPixmap pm;
    pm.loadFromData( data );
    if ( pm.isNull() )
    {
        //qDebug() << data << type;
        return;
    }

    fIcon = QIcon( pm );
    emit sigServerInfoChanged();
}

