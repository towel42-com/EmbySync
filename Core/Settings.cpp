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
#include "ServerModel.h"

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

CSettings::CSettings( std::shared_ptr< CServerModel > serverModel ) :
    CSettings( true, serverModel )
{
}

CSettings::CSettings( bool saveOnDelete, std::shared_ptr< CServerModel > serverModel ) :
    fServerModel( serverModel ),
    fSaveOnDelete( saveOnDelete )
{
    fSyncUserList = QStringList() << ".*";
    fServerModel->setSettings( this );
}

CSettings::~CSettings()
{
    if ( fSaveOnDelete )
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

QVariant CSettings::getValue( const QJsonObject &json, const QString &fieldName, const QVariant &defaultValue ) const
{
    if ( json.find( fieldName ) == json.end() )
        return defaultValue;
    else
        return json[ fieldName ].toVariant();
}

bool CSettings::load( const QString &fileName, std::function< void( const QString &title, const QString &msg ) > errorFunc, bool addToRecentFileList )
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
        if ( !fServerModel->loadServer( json[ "lhs" ].toObject(), errorMsg, false ) )
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
        if ( !fServerModel->loadServer( json[ "rhs" ].toObject(), errorMsg, true ) )
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
        if ( !fServerModel->loadServer( servers[ ii ].toObject(), errorMsg, ( ii == ( servers.count() - 1 ) ) ) )
        {
            if ( errorFunc )
                errorFunc( QObject::tr( "Could not read" ), QObject::tr( "Could not read file '%1' - '%2' for server[%3]" ).arg( fFileName ).arg( errorMsg ).arg( ii ) );
            fFileName.clear();
            return false;
        }
    }

    setOnlyShowSyncableUsers( getValue( json.object(), "OnlyShowSyncableUsers", true ).toBool() );
    setOnlyShowMediaWithDifferences( getValue( json.object(), "OnlyShowSyncableUsers", true ).toBool() );
    setShowMediaWithIssues( getValue( json.object(), "ShowMediaWithIssues", false ).toBool() );
    setOnlyShowEnabledServers( getValue( json.object(), "OnlyShowEnabledServers", true ).toBool() );
    setOnlyShowUsersWithDifferences( getValue( json.object(), "OnlyShowUsersWithDifferences", true ).toBool() );
    setShowUsersWithIssues( getValue( json.object(), "ShowUsersWithIssues", false ).toBool() );
    setMediaSourceColor( getValue( json.object(), "MediaSourceColor", "yellow" ).toString() );
    setMediaDestColor( getValue( json.object(), "MediaDestColor", "yellow" ).toString() );
    setMaxItems( getValue( json.object(), "MaxItems", -1 ).toInt() );
    setSyncAudio( getValue( json.object(), "SyncAudio", true ).toBool() );
    setSyncVideo( getValue( json.object(), "SyncVideo", true ).toBool() );
    setSyncEpisode( getValue( json.object(), "SyncEpisode", true ).toBool() );
    setSyncMovie( getValue( json.object(), "SyncMovie", true ).toBool() );
    setSyncTrailer( getValue( json.object(), "SyncTrailer", true ).toBool() );
    setSyncAdultVideo( getValue( json.object(), "SyncAdultVideo", true ).toBool() );
    setSyncMusicVideo( getValue( json.object(), "SyncMusicVideo", true ).toBool() );
    setSyncGame( getValue( json.object(), "SyncGame", true ).toBool() );
    setSyncBook( getValue( json.object(), "SyncBook", true ).toBool() );
    setSyncUserList( getValue( json.object(), "SyncUserList", QStringList() << ".*" ).toStringList() );
    setIgnoreShowList( getValue( json.object(), "IgnoreShowList", QStringList() ).toStringList() );

    setPrimaryServer( getValue( json.object(), "PrimaryServer", QString() ).toString() );
    fChanged = false;

    if ( addToRecentFileList )
        addRecentProject( fFileName );
    return true;
}

void CSettings::addRecentProject( const QString &fileName )
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

void CSettings::setPrimaryServer( const QString &serverName )
{
    updateValue( fPrimaryServer, serverName );
}

QString CSettings::primaryServer() const
{
    return fPrimaryServer;
}

bool CSettings::save()
{
    if ( fFileName.isEmpty() )
        return false;
    return save( std::function< void( const QString &title, const QString &msg ) >() );
}

bool CSettings::save( std::function< void( const QString &title, const QString &msg ) > errorFunc )
{
    if ( fFileName.isEmpty() )
        return false;

    QJsonDocument json;
    auto root = json.object();

    root[ "OnlyShowSyncableUsers" ] = onlyShowSyncableUsers();
    root[ "OnlyShowMediaWithDifferences" ] = onlyShowMediaWithDifferences();
    root[ "ShowMediaWithIssues" ] = showMediaWithIssues();
    root[ "OnlyShowEnabledServers" ] = onlyShowEnabledServers();

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

    root[ "PrimaryServer" ] = primaryServer();

    auto userList = QJsonArray();
    for ( auto &&ii : fSyncUserList )
        userList.push_back( ii );
    root[ "SyncUserList" ] = userList;

    auto showList = QJsonArray();
    for ( auto &&ii : fIgnoreShowList )
        showList.push_back( ii );
    root[ "IgnoreShowList" ] = showList;

    fServerModel->save( root );

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

QColor CSettings::getColor( const QColor &clr, bool forBackground /*= true */ ) const
{
    if ( clr == Qt::black )
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
    fServerModel->clear();
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

void CSettings::setMediaSourceColor( const QColor &color )
{
    updateValue( fMediaSourceColor, color );
}

QColor CSettings::mediaDestColor( bool forBackground /*= true */ ) const
{
    return getColor( fMediaDestColor, forBackground );
}

void CSettings::setMediaDestColor( const QColor &color )
{
    updateValue( fMediaDestColor, color );
}

QColor CSettings::dataMissingColor( bool forBackground /*= true */ ) const
{
    return getColor( fMediaDataMissingColor, forBackground );
}

void CSettings::setDataMissingColor( const QColor &color )
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

void CSettings::setOnlyShowEnabledServers( bool value )
{
    fOnlyShowEnabledServers = value;
}

void CSettings::setSyncUserList( const QStringList &value )
{
    updateValue( fSyncUserList, value );
}

QRegularExpression CSettings::ignoreShowRegEx() const
{
    QStringList regExList;
    for ( auto &&ii : fIgnoreShowList )
    {
        if ( ii.isEmpty() )
            continue;
        if ( !QRegularExpression( ii ).isValid() )
            continue;
        regExList << "(" + ii + ")";
    }
    auto regExpStr = regExList.join( "|" );
    QRegularExpression regExp;
    if ( !regExpStr.isEmpty() )
        regExp = QRegularExpression( regExpStr );
    return regExp;
}

void CSettings::setIgnoreShowList( const QStringList &value )
{
    std::set< QString > tmp = { value.begin(), value.end() };
    updateValue( fIgnoreShowList, tmp );
}
