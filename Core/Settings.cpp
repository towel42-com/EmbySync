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

#include "Settings.h"
#include <QJsonDocument>
#include <QFile>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QSettings>

#include <QUrlQuery>
#include <QColor>

CSettings::CSettings()
{

}

CSettings::~CSettings()
{
    save();
}

bool CSettings::load( const QString & fileName, std::function<void( const QString & title, const QString & msg )> errorFunc )
{
    fFileName = fileName;

    QFile file( fFileName );
    if ( !file.open( QFile::ReadOnly | QFile::Text ) )
    {
        if ( errorFunc )
            errorFunc( QObject::tr( "Could not open" ), QObject::tr( "Could not open file '%1'" ).arg( fFileName ) );
        fFileName.clear();
        return false;
    }

    QJsonParseError error;
    auto json = QJsonDocument::fromJson( file.readAll(), &error );
    if ( error.error != QJsonParseError::NoError )
    {
        if ( errorFunc )
            errorFunc( QObject::tr( "Could not read" ), QObject::tr( "Could not read file '%1' - '%2' @ %3" ).arg( fFileName ).arg( error.errorString() ).arg( error.offset ) );
        fFileName.clear();
        return false;
    }

    setLHSURL( json[ "lhs" ][ "url" ].toString() );
    setLHSAPI( json[ "lhs" ][ "api" ].toString() );

    setRHSURL( json[ "rhs" ][ "url" ].toString() );
    setRHSAPI( json[ "rhs" ][ "api" ].toString() );

    if ( json.object().find( "OnlyShowSyncableUsers" ) == json.object().end() )
        setOnlyShowSyncableUsers( true );
    else
        setOnlyShowSyncableUsers( json[ "OnlyShowSyncableUsers" ].toBool() );

    if ( json.object().find( "OnlyShowMediaWithDifferences" ) == json.object().end() )
        setOnlyShowMediaWithDifferences( true );
    else
        setOnlyShowMediaWithDifferences( json[ "OnlyShowSyncableUsers" ].toBool() );

    if ( json.object().find( "MediaSourceColor" ) == json.object().end() )
        setMediaSourceColor( "yellow" );
    else
        setMediaSourceColor( json[ "MediaSourceColor" ].toString() );

    if ( json.object().find( "MediaDestColor" ) == json.object().end() )
        setMediaDestColor( "yellow" );
    else
        setMediaDestColor( json[ "MediaDestColor" ].toString() );

    if ( json.object().find( "MaxItems" ) == json.object().end() )
        setMaxItems( -1 );
    else
        setMaxItems( json[ "MaxItems" ].toInt() );


    fChanged = false;

    addRecentProject( fFileName );
    return true;
}

void CSettings::addRecentProject( const QString & fileName )
{
    auto fileList = recentProjectList();
    fileList.removeAll( fileName );
    fileList.push_front( fileName );

    QSettings settings;
    settings.setValue( "RecentProjects", fileList );
}

QStringList CSettings::recentProjectList() const
{
    QSettings settings;
    return settings.value( "RecentProjects", QStringList() ).toStringList();
}

bool CSettings::save()
{
    if ( fFileName.isEmpty() )
        return false;
    return save( std::function<void( const QString & title, const QString & msg )>() );
}

bool CSettings::save( std::function<void( const QString & title, const QString & msg )> errorFunc )
{
    if ( fFileName.isEmpty() )
        return false;

    QJsonDocument json;
    auto root = json.object();

    root[ "OnlyShowSyncableUsers" ] = onlyShowSyncableUsers();
    root[ "OnlyShowMediaWithDifferences" ] = onlyShowMediaWithDifferences();

    root[ "MediaSourceColor" ] = mediaSourceColor().name();
    root[ "MediaDestColor" ] = mediaDestColor().name();
    root[ "MaxItems" ] = maxItems();

    auto lhs = json.object();

    lhs[ "url" ] = lhsURL();
    lhs[ "api" ] = lhsAPI();

    root[ "lhs" ] = lhs;

    auto rhs = json.object();

    rhs[ "url" ] = rhsURL();
    rhs[ "api" ] = rhsAPI();

    root[ "rhs" ] = rhs;

    json = QJsonDocument( root );

    auto jsonData = json.toJson( QJsonDocument::Indented );

    QFile file( fFileName );
    if ( !file.open( QFile::WriteOnly | QFile::Text | QFile::Truncate ) )
    {
        if ( errorFunc )
            errorFunc( QObject::tr( "Could not open" ), QObject::tr( "Could not open file '%1' for writing" ).arg( fFileName ) );
        return false;
    }

    file.write( jsonData );
    fChanged = false;
    return true;
}

QUrl CSettings::getUrl( bool lhs ) const
{
    auto path = lhs ? lhsURL() : rhsURL();
    if ( path.isEmpty() )
        return {};

    if ( !path.endsWith( "/" ) )
        path += "/";

    QUrl retVal( path );

    const auto && api = lhs ? lhsAPI() : rhsAPI();

    QUrlQuery query;
    query.addQueryItem( "api_key", api );
    retVal.setQuery( query );
    return retVal;
}

void CSettings::setLHSURL( const QString & url )
{
    updateValue( fLHSServer.first, url );
}

void CSettings::setLHSAPI( const QString & api )
{
    updateValue( fLHSServer.second, api );
}

void CSettings::setRHSURL( const QString & url )
{
    updateValue( fRHSServer.first, url );
}

void CSettings::setRHSAPI( const QString & api )
{
    updateValue( fRHSServer.second, api );
}

QColor CSettings::getColor( const QColor & clr, bool forBackground /*= true */ ) const
{
    if (clr == Qt::black )
    {
        if ( !forBackground )
            return QColor( Qt::white ).name();
    }

    if ( !forBackground )
        return QString();
    return clr;
}

QColor CSettings::mediaSourceColor( bool forBackground /*= true */ ) const
{
    return getColor( fMediaSourceColor, forBackground );
}

void CSettings::setMediaSourceColor( const QColor & color )
{
    updateValue( fMediaSourceColor, color );
}

QColor CSettings::mediaDestColor( bool forBackground /*= true */ ) const
{
    return getColor( fMediaDestColor, forBackground );
}

void CSettings::setMediaDestColor( const QColor & color )
{
    updateValue( fMediaDestColor, color );
}

QColor CSettings::dataMissingColor( bool forBackground /*= true */ ) const
{
    return getColor( fMediaDataMissingColor, forBackground );
}

void CSettings::setDataMissingColor( const QColor & color )
{
    updateValue( fMediaDataMissingColor, color );
}

void CSettings::setMaxItems( int maxItems )
{
    updateValue( fMaxItems, maxItems );
}

QString CSettings::getServerName( bool lhs )
{
    return lhs ? fLHSServer.first : fRHSServer.first;
}

