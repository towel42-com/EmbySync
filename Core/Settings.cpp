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

bool CSettings::checkForLatest()
{
    QSettings settings;
    return settings.value( "CheckForLatest", true ).toBool();
}

void CSettings::setCheckForLatest( bool value )
{
    QSettings settings;
    settings.setValue( "CheckForLatest", value );
}

bool CSettings::loadLastProject()
{
    QSettings settings;
    return settings.value( "LoadLastProject", true ).toBool();
}

void CSettings::setLoadLastProject( bool value )
{
    QSettings settings;
    settings.setValue( "LoadLastProject", value );
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
    
    if ( json.object().find( "OnlyShowUsersWithDifferences" ) == json.object().end() )
        setOnlyShowUsersWithDifferences( true );
    else
        setOnlyShowUsersWithDifferences( json[ "OnlyShowSyncableUsers" ].toBool() );

    if ( json.object().find( "ShowUsersWithIssues" ) == json.object().end() )
        setShowUsersWithIssues( false );
    else
        setShowUsersWithIssues( json[ "ShowUsersWithIssues" ].toBool() );

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
        servers.push_back( serverInfo( ii )->toJson() );
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

    addRecentProject( fFileName );
    return true;
}

bool CSettings::canAllServersSync() const
{
    for ( auto && ii : fServers )
    {
        if ( !ii->canSync() )
            return false;
    }
    return true;
}

bool CSettings::canAnyServerSync() const
{
    for ( auto && ii : fServers )
    {
        if ( ii->canSync() )
            return true;
    }
    return false;
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

void CSettings::setOnlyShowUsersWithDifferences( bool value )
{
    fOnlyShowUsersWithDifferences = value;
}

void CSettings::setShowUsersWithIssues( bool value )
{
    fShowUsersWithIssues = value;
}

void CSettings::setSyncUserList( const QStringList & value )
{
    updateValue( fSyncUserList, value );
}

void CSettings::setServers( const std::vector < std::shared_ptr< CServerInfo > > & servers )
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


int CSettings::enabledServerCnt() const
{
    int retVal = 0;
    for ( int ii = 0; ii < serverCnt(); ++ii )
    {
        if ( fServers[ ii ]->isEnabled() )
            retVal++;
    }
    return retVal;
}

bool CSettings::serversChanged( const std::vector< std::shared_ptr< CServerInfo > > & lhs, const std::vector< std::shared_ptr< CServerInfo > > & rhs ) const
{
    if ( lhs.size() != rhs.size() )
        return true;

    bool retVal = false;
    for ( auto && ii : lhs )
    {
        std::shared_ptr< CServerInfo > found;
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
    auto serverInfo = CServerInfo::fromJson( obj, errorMsg );
    if ( !serverInfo )
        return false;

    if ( this->findServerInfo( serverInfo->keyName() ) )
    {
        errorMsg = QString( "Server %1(%2)' already exists" ).arg( serverInfo->displayName() ).arg( serverInfo->keyName() );
        return false;
    }

    fServers.push_back( serverInfo );
    fServerMap[ serverInfo->keyName() ] = { serverInfo, fServers.size() - 1 };

    updateFriendlyServerNames();
    return true;
}

int CSettings::getServerPos( const QString & serverName ) const
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return static_cast<int>( ( *pos ).second.second );
    return -1;
}

std::shared_ptr< const CServerInfo > CSettings::serverInfo( int serverNum ) const
{
    if ( serverNum < 0 )
        return {};
    if ( serverNum >= serverCnt() )
        return {};

    return fServers[ serverNum ];
}

std::shared_ptr< const CServerInfo > CSettings::findServerInfo( const QString & serverName ) const
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return ( *pos ).second.first;
    return {};
}

std::shared_ptr< const CServerInfo > CSettings::findServerInfo( const QString & serverName )
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return ( *pos ).second.first;
    return {};
}

void CSettings::updateServerInfo( const QString & serverName, const QJsonObject & serverData )
{
    auto serverInfo = findServerInfoInternal( serverName );
    if ( !serverInfo )
        return;

    serverInfo->update( serverData );
}

void CSettings::setServerIcon( const QString & serverName, const QByteArray & data, const QString & type )
{
    auto serverInfo = findServerInfoInternal( serverName );
    if ( !serverInfo )
        return;

    serverInfo->setIcon( data, type );
}

std::shared_ptr< CServerInfo > CSettings::findServerInfoInternal( const QString & serverName )
{
    auto pos = fServerMap.find( serverName );
    if ( pos != fServerMap.end() )
        return ( *pos ).second.first;
    return {};
}

int CSettings::serverCnt() const
{
    return static_cast<int>( fServers.size() );
}

void CSettings::setAPIKey( const QString & serverName, const QString & apiKey )
{
    auto serverInfo = this->findServerInfoInternal( serverName );
    if ( serverInfo->setAPIKey( apiKey ) )
        fChanged = true;
}

void CSettings::changeServerDisplayName( const QString & newServerName, const QString & oldServerName )
{
    auto serverInfo = this->findServerInfoInternal( oldServerName );
    if ( serverInfo )
    {
        if ( serverInfo->setDisplayName( newServerName, false ) )
            fChanged = true;

        auto pos = fServerMap.find( oldServerName );
        if ( pos != fServerMap.end() )
            fServerMap.erase( pos );
    }
}

void CSettings::setURL( const QString & serverName, const QString & url )
{
    auto serverInfo = this->findServerInfoInternal( serverName );
    if ( serverInfo->setUrl( url ) )
        fChanged = true;
}

void CSettings::updateFriendlyServerNames()
{
    std::multimap< QString, std::shared_ptr< CServerInfo > > servers;
    for ( int ii = 0; ii < serverCnt(); ++ii )
    {
        auto serverInfo = fServers[ ii ];
        if ( serverInfo->displayNameGenerated() ) // generatedName
            servers.insert( { serverInfo->displayName(), serverInfo } );
    }

    for ( auto && ii : servers )
    {
        auto cnt = servers.count( ii.first );
        ii.second->autoSetDisplayName( cnt > 1 );
    }
    updateServerMap();
}
