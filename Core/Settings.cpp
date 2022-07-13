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

#include <QMessageBox>
#include <QFileDialog>
#include <QUrlQuery>

CSettings::CSettings()
{

}

CSettings::CSettings( const QString & fileName, QWidget * parentWidget ) :
    CSettings()
{
    fFileName = fileName;
    load( parentWidget );
}
   

CSettings::~CSettings()
{
    save( nullptr );
}

bool CSettings::load( const QString & fileName, QWidget * parentWidget )
{
    fFileName = fileName;
    return load( parentWidget );
}

bool CSettings::load( QWidget * parentWidget )
{
    if ( fFileName.isEmpty() )
    {
        auto fileName = QFileDialog::getOpenFileName( parentWidget, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
        if ( fileName.isEmpty() )
            return false;
        fFileName = QFileInfo( fileName ).absoluteFilePath();
    }

    QFile file( fFileName );
    if ( !file.open( QFile::ReadOnly | QFile::Text ) )
    {
        QMessageBox::critical( parentWidget, QObject::tr( "Could not open" ), QObject::tr( "Could not open file '%1'" ).arg( fFileName ) );
        fFileName.clear();
        return false;
    }

    QJsonParseError error;
    auto json = QJsonDocument::fromJson( file.readAll(), &error );
    if ( error.error != QJsonParseError::NoError )
    {
        QMessageBox::critical( parentWidget, QObject::tr( "Could not read" ), QObject::tr( "Could not read file '%1' - '%2' @ %3" ).arg( fFileName ).arg( error.errorString() ).arg( error.offset ) );
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


bool CSettings::save( QWidget * parent )
{
    if ( fFileName.isEmpty() )
        return maybeSave( parent );

    QJsonDocument json;
    auto root = json.object();

    root[ "OnlyShowSyncableUsers" ] = onlyShowSyncableUsers();

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
        QMessageBox::critical( nullptr, QObject::tr( "Could not open" ), QObject::tr( "Could not open file '%1' for writing" ).arg( fFileName ) );
        return false;
    }

    file.write( jsonData );
    fChanged = false;
    return true;
}

bool CSettings::maybeSave( QWidget * parent )
{
    if ( !fChanged )
        return true;

    if ( fFileName.isEmpty() )
    {
        auto fileName = QFileDialog::getSaveFileName( parent, QObject::tr( "Select File" ), QString(), QObject::tr( "Settings File (*.json);;All Files (* *.*)" ) );
        if ( fileName.isEmpty() )
            return true;

        fFileName = fileName;
    }

    if ( fFileName.isEmpty() )
        return false;
    return save( parent );
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

void CSettings::updateValue( QString & lhs, const QString & rhs )
{
    if ( lhs != rhs )
    {
        lhs = rhs;
        fChanged = true;
    }
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

QString CSettings::getServerName( bool lhs )
{
    return lhs ? fLHSServer.first : fRHSServer.first;
}

