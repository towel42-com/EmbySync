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

QString CServerInfo::url() const
{
    return fURL;
}

QUrl CServerInfo::getUrl() const
{
    return getUrl( QString(), std::list< std::pair< QString, QString > >() );
}


QUrl CServerInfo::getUrl( const QString & extraPath, const std::list< std::pair< QString, QString > > & queryItems ) const
{
    auto path = fURL;
    if ( path.isEmpty() )
        return {};

    if ( !extraPath.isEmpty() )
    {
        if ( !path.endsWith( "/" ) )
            path += "/";
        path += extraPath;
    }

    if ( path.indexOf( "://" ) == -1 )
        path = "http://" + path;

    QUrl retVal( path );

    QUrlQuery query;
    query.addQueryItem( "api_key", fAPIKey );
    for ( auto && ii : queryItems )
    {
        query.addQueryItem( ii.first, ii.second );
    }
    retVal.setQuery( query );

    //qDebug() << url;

    return retVal;
}

QString CServerInfo::displayName() const
{
    if ( !fName.first.isEmpty() )
        return fName.first;
    else
        return keyName();
}

QString CServerInfo::keyName() const
{
    if ( fKeyName.isEmpty() )
        fKeyName = getUrl().toString( QUrl::RemoveUserInfo | QUrl::RemoveQuery );
    return fKeyName;
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

