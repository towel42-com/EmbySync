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


SServerInfo::SServerInfo( const QString & name, const QString & url, const QString & apiKey ) :
    fName( { name, false } ),
    fURL( url ),
    fAPIKey( apiKey )
{

}

SServerInfo::SServerInfo( const QString & name ) :
    fName( { name, false } )
{

}

bool SServerInfo::operator==( const SServerInfo & rhs ) const
{
    return fName == rhs.fName
        && fURL == rhs.fURL
        && fAPIKey == rhs.fAPIKey
        ;
}

QString SServerInfo::url() const
{
    return fURL;
}

QUrl SServerInfo::getUrl() const
{
    return getUrl( QString(), std::list< std::pair< QString, QString > >() );
}


QUrl SServerInfo::getUrl( const QString & extraPath, const std::list< std::pair< QString, QString > > & queryItems ) const
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

QString SServerInfo::friendlyName() const
{
    if ( !fName.first.isEmpty() )
        return fName.first;
    else
        return keyName();
}

QString SServerInfo::keyName() const
{
    if ( fKeyName.isEmpty() )
        fKeyName = getUrl().toString( QUrl::RemoveUserInfo | QUrl::RemoveQuery );
    return fKeyName;
}

void SServerInfo::autoSetFriendlyName( bool usePort )
{
    auto url = getUrl();
    QString retVal = url.host();
    if ( usePort )
        retVal += QString::number( url.port() );
    setFriendlyName( retVal, true );
}

void SServerInfo::setFriendlyName( const QString & name, bool generated )
{
    fName = { name, generated };
}

bool SServerInfo::canSync() const
{
    return getUrl().isValid() && !fAPIKey.isEmpty();
}

