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
    fSyncUserList = QStringList() << ".*";
}

CSettings::~CSettings()
{
    save();
}

bool CSettings::load( const QString & fileName, std::function<void( const QString & title, const QString & msg )> errorFunc, bool addToRecentFileList )
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

    setURL( json[ "lhs" ][ "url" ].toString(), true );
    setAPIKey( json[ "lhs" ][ "api_key" ].toString(), true );

    setURL( json[ "rhs" ][ "url" ].toString(), false );
    setAPIKey( json[ "rhs" ][ "api_key" ].toString(), false );

    if ( json.object().find( "OnlyShowSyncableUsers" ) == json.object().end() )
        setOnlyShowSyncableUsers( true );
    else
        setOnlyShowSyncableUsers( json[ "OnlyShowSyncableUsers" ].toBool() );

    if ( json.object().find( "OnlyShowMediaWithDifferences" ) == json.object().end() )
        setOnlyShowMediaWithDifferences( true );
    else
        setOnlyShowMediaWithDifferences( json[ "OnlyShowSyncableUsers" ].toBool() );

    if ( json.object().find( "ShowMediaWithIssues" ) == json.object().end() )
        setShowMediaWithIssues( false );
    else
        setShowMediaWithIssues( json[ "ShowMediaWithIssues" ].toBool() );
    

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

    if ( json.object().find( "SyncAudio" ) == json.object().end() )
        setSyncAudio( true );
    else
        setSyncAudio( json[ "SyncAudio" ].toBool() );

    if ( json.object().find( "SyncVideo" ) == json.object().end() )
        setSyncVideo( true );
    else
        setSyncVideo( json[ "SyncVideo" ].toBool() );

    if ( json.object().find( "SyncEpisode" ) == json.object().end() )
        setSyncEpisode( true );
    else
        setSyncEpisode( json[ "SyncEpisode" ].toBool() );

    if ( json.object().find( "SyncMovie" ) == json.object().end() )
        setSyncMovie( true );
    else
        setSyncMovie( json[ "SyncMovie" ].toBool() );

    if ( json.object().find( "SyncTrailer" ) == json.object().end() )
        setSyncTrailer( true );
    else
        setSyncTrailer( json[ "SyncTrailer" ].toBool() );

    if ( json.object().find( "SyncAdultVideo" ) == json.object().end() )
        setSyncAdultVideo( true );
    else
        setSyncAdultVideo( json[ "SyncAdultVideo" ].toBool() );

    if ( json.object().find( "SyncMusicVideo" ) == json.object().end() )
        setSyncMusicVideo( true );
    else
        setSyncMusicVideo( json[ "SyncMusicVideo" ].toBool() );

    if ( json.object().find( "SyncGame" ) == json.object().end() )
        setSyncGame( true );
    else
        setSyncGame( json[ "SyncGame" ].toBool() );

    if ( json.object().find( "SyncBook" ) == json.object().end() )
        setSyncBook( true );
    else
        setSyncBook( json[ "SyncBook" ].toBool() );

    if ( json.object().find( "SyncUserList" ) == json.object().end() )
        setSyncUserList( QStringList() << ".*" );
    else
    {
        QStringList userList;
        auto tmp = json[ "SyncUserList" ].toArray();
        for ( auto && ii : tmp )
            userList << ii.toString();
        setSyncUserList( userList );
    }

    fChanged = false;

    if ( addToRecentFileList )
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
    root[ "ShowMediaWithIssues" ] = showMediaWithIssues();
    

    root[ "MediaSourceColor" ] = mediaSourceColor().name();
    root[ "MediaDestColor" ] = mediaDestColor().name();
    root[ "MaxItems" ] = maxItems();

    root[ "SyncAudio" ] = syncAudio();
    root[ "SyncVideo" ] = syncVideo();
    root[ "SyncEpisode" ] = syncEpisode();
    root[ "SyncMovie" ] = syncMovie();
    root[ "SyncTrailer" ] = syncTrailer();
    root[ "SyncAdultVideo" ] = syncAdultVideo();
    root[ "SyncMusicVideo" ] = syncMusicVideo();
    root[ "SyncGame" ] = syncGame();
    root[ "SyncBook" ] = syncBook();
    
    auto userList = QJsonArray();
    for ( auto && ii : fSyncUserList )
        userList.push_back( ii );
    root[ "SyncUserList" ] = userList;
    auto lhs = json.object();

    lhs[ "url" ] = url( true );
    lhs[ "api_key" ] = apiKey( true );

    root[ "lhs" ] = lhs;

    auto rhs = json.object();

    rhs[ "url" ] = url( false );
    rhs[ "api_key" ] = apiKey( false );

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


QString CSettings::serverName( bool lhs )
{
    if ( lhs && fLHSServer.first.isEmpty() )
        return QObject::tr( "LHS Name" );
    if ( !lhs && fRHSServer.first.isEmpty() )
        return QObject::tr( "RHS Name" );

    auto retVal = getUrl( lhs );
    if ( retVal.host() == getUrl( !lhs ).host() )
        return QString( "%1:%2" ).arg( retVal.host() ).arg( retVal.port() );
    return retVal.host();
}

QUrl CSettings::getUrl( bool lhs ) const
{
    return getUrl( QString(), std::list< std::pair< QString, QString > >(), lhs );
}

QUrl CSettings::getUrl( const QString & extraPath, const std::list< std::pair< QString, QString > > & queryItems, bool lhs ) const
{
    auto path = url( lhs );
    if ( path.isEmpty() )
        return {};

    if ( !extraPath.isEmpty() )
    {
        if ( !path.endsWith( "/" ) )
            path += "/";
        path += extraPath;
    }

    QUrl retVal( path );

    QUrlQuery query;
    query.addQueryItem( "api_key", apiKey( lhs ) );
    for ( auto && ii : queryItems )
    {
        query.addQueryItem( ii.first, ii.second );
    }
    retVal.setQuery( query );

    //qDebug() << url;

    return retVal;
}

void CSettings::setURL( const QString & url, bool lhs )
{
    if ( lhs )
        updateValue( fLHSServer.first, url );
    else
        updateValue( fRHSServer.first, url );
}

void CSettings::setAPIKey( const QString & apiKey, bool lhs )
{
    if ( lhs )
        updateValue( fLHSServer.second, apiKey );
    else
        updateValue( fRHSServer.second, apiKey );
}

bool CSettings::canSync() const
{
    return getUrl( true ).isValid()
        && !apiKey( true ).isEmpty()
        && getUrl( false ).isValid()
        && !apiKey( false ).isEmpty()
        ;
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


QString CSettings::getSyncItemTypes() const
{
    QStringList values;
    if ( syncAudio() )
        values << "Audio";
    if ( syncVideo() )
        values << "Video";
    if ( syncEpisode() )
        values << "Episode";
    if ( syncTrailer() )
        values << "Trailer";
    if ( syncMovie() )
        values << "Movie";
    if ( syncAdultVideo() )
        values << "AdultVideo";
    if ( syncMusicVideo() )
        values << "MusicVideo";
    if ( syncGame() )
        values << "Game";
    if ( syncBook() )
        values << "Book";
    return values.join( "," );
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

void CSettings::setSyncAudio( bool value )
{
    updateValue( fSyncAudio, value );
}

void CSettings::setSyncVideo( bool value )
{
    updateValue( fSyncVideo, value );
}

void CSettings::setSyncEpisode( bool value )
{
    updateValue( fSyncEpisode, value );
}

void CSettings::setSyncMovie( bool value )
{
    updateValue( fSyncMovie, value );
}

void CSettings::setSyncTrailer( bool value )
{
    updateValue( fSyncTrailer, value );
}

void CSettings::setSyncAdultVideo( bool value )
{
    updateValue( fSyncAdultVideo, value );
}

void CSettings::setSyncMusicVideo( bool value )
{
    updateValue( fSyncMusicVideo, value );
}

void CSettings::setSyncGame( bool value )
{
    updateValue( fSyncGame, value );
}

void CSettings::setSyncBook( bool value )
{
    updateValue( fSyncBook, value );
}

void CSettings::setOnlyShowSyncableUsers( bool value )
{
    fOnlyShowSyncableUsers = value;
}

void CSettings::setOnlyShowMediaWithDifferences( bool value )
{
    fOnlyShowMediaWithDifferences = value;
}

void CSettings::setShowMediaWithIssues( bool value )
{
    fShowMediaWithIssues = value;
}

void CSettings::setSyncUserList( const QStringList & value )
{
    updateValue( fSyncUserList, value );
}


