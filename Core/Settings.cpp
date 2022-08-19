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
#include "ServerInfo.h"
#include <QJsonDocument>
#include <QFile>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QSettings>

#include <QUrlQuery>
#include <QColor>

#include <map>

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

    if ( json.object().contains( "lhs" ) )
    {
        QString errorMsg;
        if ( !loadServer( json[ "lhs" ].toObject(), errorMsg ) )
        {
            if ( errorFunc )
                errorFunc( QObject::tr( "Could not read" ), QObject::tr( "Could not read file '%1' - '%2' for lhs server" ).arg( fFileName ).arg( errorMsg ) );
            fFileName.clear();
            return false;
        }
    }

    if ( json.object().contains( "rhs" ) )
    {
        QString errorMsg;
        if ( !loadServer( json[ "rhs" ].toObject(), errorMsg ) )
        {
            if ( errorFunc )
                errorFunc( QObject::tr( "Could not read" ), QObject::tr( "Could not read file '%1' - '%2' for rhs server" ).arg( fFileName ).arg( errorMsg ) );

            fFileName.clear();
            return false;
        }
    }

    auto servers = json[ "servers" ].toArray();
    for ( int ii = 0; ii < servers.count(); ++ii )
    {
        QString errorMsg;
        if ( !loadServer( servers[ ii ].toObject(), errorMsg ) )
        {
            if ( errorFunc )
                errorFunc( QObject::tr( "Could not read" ), QObject::tr( "Could not read file '%1' - '%2' for server[%3]" ).arg( fFileName ).arg( errorMsg ).arg( ii ) );
                fFileName.clear();
                return false;
        }
    }
    
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
    
    auto servers = json.array();

    for ( int ii = 0; ii < serverCnt(); ++ii )
    {
        auto serverInfo = this->serverInfo( ii );

        auto curr = json.object();

        curr[ "url" ] = serverInfo->url();
        curr[ "api_key" ] = serverInfo->fAPIKey;
        if ( !serverInfo->fName.second )
            curr[ "name" ] = serverInfo->friendlyName();

        servers.push_back( curr );
    }

    root[ "servers" ] = servers;

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

bool CSettings::canSync() const
{
    for ( auto && ii : fServers )
    {
        if ( !ii->canSync() )
            return false;
    }
    return true;
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

void CSettings::reset()
{
    fServers.clear();
    fServerMap.clear();
    fChanged = false;
    fFileName.clear();
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


void CSettings::setServers( const std::vector < std::shared_ptr< SServerInfo > > & servers )
{
    fChanged = fChanged || serversChanged( servers, fServers );
    fChanged = fChanged || serversChanged( fServers, servers );

    if ( !fChanged )
        return;

    fServers = servers;
    fServerMap.clear();
    updateServerMap();
    updateFriendlyServerNames();
}


bool CSettings::serversChanged( const std::vector< std::shared_ptr< SServerInfo > > & lhs, const std::vector< std::shared_ptr< SServerInfo > > & rhs ) const
{
    if ( lhs.size() != rhs.size() )
        return false;

    bool retVal = false;
    for ( auto && ii : lhs )
    {
        std::shared_ptr< SServerInfo > found;
        for ( auto && jj : rhs )
        {
            if ( ii->keyName() == jj->keyName() )
            {
                found = jj;
                break;
            }
        }
        if ( found )
        {
            if ( *ii != *found )
            {
                retVal = true;
                break;
            }
        }
        else
        {
            retVal = true;
            break;
        }
    }        
    return retVal;
}

void CSettings::updateServerMap()
{
    for ( size_t ii = 0; ii < fServers.size(); ++ii )
    {
        auto server = fServers[ ii ];
        fServerMap[ server->keyName() ] = { server, ii };
    }
}

bool CSettings::loadServer( const QJsonObject & obj, QString & errorMsg )
{
    bool generated = !obj.contains( "name" ) || obj["name"].toString().isEmpty();
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

    if ( !obj.contains(  "url"  ) )
    {
        errorMsg = QString( "Missing url" );
        return false;
    }

    if ( !obj.contains( "api_key" ) )
    {
        errorMsg = QString( "Missing api_key" );
        return false;
    }
    auto serverInfo = this->serverInfo( serverName, false );
    if ( serverInfo )
    {
        errorMsg = QString( "Server '%1' already exists" ).arg( serverName );
        return false;
    }

    serverInfo = this->serverInfo( serverName, true );
    serverInfo->fName.second = generated;

    setURL( serverName, obj[ "url" ].toString() );
    setAPIKey( serverName, obj[ "api_key" ].toString() );
    updateFriendlyServerNames();
    return true;
}

std::shared_ptr< const SServerInfo > CSettings::serverInfo( int serverNum ) const
{
    if ( serverNum < 0 )
        return {};
    if ( serverNum >= serverCnt() )
        return {};

    return fServers[ serverNum ];
}


int CSettings::getServerPos( const QString & serverName ) const
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return static_cast<int>( ( *pos ).second.second );
    return -1;
}


std::shared_ptr< const SServerInfo > CSettings::serverInfo( const QString & serverName ) const
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return ( *pos ).second.first;
    return {};
}

std::shared_ptr< SServerInfo > CSettings::serverInfo( const QString & serverName, bool addIfMissing )
{
    auto retVal = serverInfo( serverName );
    if ( retVal || !addIfMissing )
        return std::const_pointer_cast<SServerInfo>( retVal );

    auto realRetVal = std::make_shared< SServerInfo >( serverName );
    fServers.push_back( realRetVal );
    fServerMap[ serverName ] = { realRetVal, fServers.size() - 1 };
    return realRetVal;
}


int CSettings::serverCnt() const
{
    return static_cast<int>( fServers.size() );
}

void CSettings::setAPIKey( const QString & serverName, const QString & apiKey )
{
    auto serverInfo = this->serverInfo( serverName, true );
    updateValue( serverInfo->fAPIKey, apiKey );
}

void CSettings::changeServerFriendlyName( const QString & newServerName, const QString & oldServerName )
{
    auto serverInfo = this->serverInfo( oldServerName, false );
    if ( serverInfo )
    {
        auto tmp = serverInfo->friendlyName();
        updateValue( tmp, newServerName );
        serverInfo->setFriendlyName( newServerName, false );

        auto pos = fServerMap.find( oldServerName );
        if ( pos != fServerMap.end() )
            fServerMap.erase( pos );
    }
    else
    {
        this->serverInfo( newServerName, true );
    }
}

void CSettings::setURL( const QString & serverName, const QString & url )
{
    auto serverInfo = this->serverInfo( serverName, true );
    updateValue( serverInfo->fURL, url );
}

void CSettings::updateFriendlyServerNames()
{
    std::multimap< QString, std::shared_ptr< SServerInfo > > servers;
    for ( int ii = 0; ii < serverCnt(); ++ii )
    {
        auto serverInfo = fServers[ ii ];
        if ( serverInfo->fName.second ) // generatedName
            servers.insert( { serverInfo->friendlyName(), serverInfo } );
    }

    for ( auto && ii : servers )
    {
        auto cnt = servers.count( ii.first );
        ii.second->autoSetFriendlyName( cnt > 1 );
    }
    updateServerMap();
}