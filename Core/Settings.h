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

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QString>
#include <QColor>
#include <QUrl>

#include <memory>
#include <tuple>
#include <functional>
#include <vector>
#include <map>
#include <optional>

class QWidget;
class QJsonObject;
namespace Ui
{
    class CSettings;
}


struct SServerInfo
{
    SServerInfo() = default;
    SServerInfo( const QString & name, const QString & url, const QString & apiKey );
    SServerInfo( const QString & name );

    bool operator==( const SServerInfo & rhs ) const;
    bool operator!=( const SServerInfo & rhs ) const
    {
        return !operator==( rhs );
    }

    QString url() const;
    QUrl getUrl() const;
    QUrl getUrl( const QString & extraPath, const std::list< std::pair< QString, QString > > & queryItems ) const;

    QString friendlyName() const; // returns the name, if empty returns the fqdn, if the same fqdn is used more than once, it use fqdn:port
    QString keyName() const;// getUrl().toString()

    void autoSetFriendlyName( bool usePort );

    void setFriendlyName( const QString & name, bool generated );
    bool canSync() const;

    std::pair< QString, bool > fName; // may be automatically generated or not
    mutable QString fKeyName;
    QString fURL;
    QString fAPIKey;
};

class CSettings
{
public:
    CSettings();
    virtual ~CSettings();

    QString fileName() const { return fFileName; }

    bool load( bool addToRecentFileList, QWidget * parent );
    bool load( const QString & fileName, bool addToRecentFileList, QWidget * parent );
    bool load( const QString & fileName, std::function<void( const QString & title, const QString & msg )> errorFunc, bool addToRecentFileList );

    bool save();

    void setServers( const std::vector < std::shared_ptr< SServerInfo > > & servers );

    int serverCnt() const;
    int getServerPos( const QString & serverName ) const;
    std::shared_ptr< const SServerInfo > getServerInfo( int serverNum ) const;
    std::shared_ptr< const SServerInfo > getServerInfo( const QString & serverName ) const;
    bool canSync() const;

    bool save( QWidget * parent );
    bool maybeSave( QWidget * parent );
    bool save( QWidget * parent, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc );
    bool save( std::function<void( const QString & title, const QString & msg )> errorFunc );

    bool changed() const { return fChanged; }
    void reset();

    // Various server calls
    QUrl getUrl( int serverNum ) const;
    QUrl getUrl( const QString & serverName ) const;
    QUrl getUrl( const QString & extraPath, const std::list< std::pair< QString, QString > > & queryItems, const QString & serverName ) const;
    template <class T>
    QUrl getUrl( T ) const = delete;

    QString url( int serverNum ) const;
    QString url( const QString & serverName ) const;
    template <class T>
    QString url( T ) const = delete;
    void setURL( const QString & url, const QString & serverName );

    QString apiKey( int serverNum ) const;
    QString apiKey( const QString & serverName ) const;
    template <class T>
    QString apiKey( T ) const = delete;
    void setAPIKey( const QString & apiKey, const QString & serverName );

    QString serverKeyName( int serverNum ) const;
    template <class T>
    QString serverKeyName( T ) const = delete;

    QString friendlyServerName( int serverNum ) const;  // returns name of the server, if its blank returns the serverKeyName
    template <class T>
    QString friendlyServerName( T ) const = delete;
    void setServerFriendlyName( const QString & serverName, const QString & oldServerName );

    // other settings
    QColor mediaSourceColor( bool forBackground = true ) const;
    void setMediaSourceColor( const QColor & color );

    QColor mediaDestColor( bool forBackground = true ) const;
    void setMediaDestColor( const QColor & color );

    QColor dataMissingColor( bool forBackground = true ) const;
    void setDataMissingColor( const QColor & color );

    int maxItems() const { return fMaxItems; }
    void setMaxItems( int maxItems );

    bool syncAudio() const { return fSyncAudio; }
    void setSyncAudio( bool value );

    bool syncVideo() const { return fSyncVideo; }
    void setSyncVideo( bool value );

    bool syncEpisode() const { return fSyncEpisode; }
    void setSyncEpisode( bool value );

    bool syncMovie() const { return fSyncMovie; }
    void setSyncMovie( bool value );

    bool syncTrailer() const { return fSyncTrailer; }
    void setSyncTrailer( bool value );

    bool syncAdultVideo() const { return fSyncAdultVideo; }
    void setSyncAdultVideo( bool value );

    bool syncMusicVideo() const { return fSyncMusicVideo; }
    void setSyncMusicVideo( bool value );

    bool syncGame() const { return fSyncGame; }
    void setSyncGame( bool value );

    bool syncBook() const { return fSyncBook; }
    void setSyncBook( bool value );

    QString getSyncItemTypes() const;
    bool onlyShowSyncableUsers() { return fOnlyShowSyncableUsers; };
    void setOnlyShowSyncableUsers( bool value );;

    bool onlyShowMediaWithDifferences() { return fOnlyShowMediaWithDifferences; };
    void setOnlyShowMediaWithDifferences( bool value );;

    bool showMediaWithIssues() { return fShowMediaWithIssues; };
    void setShowMediaWithIssues( bool value );;

    QStringList syncUserList() const { return fSyncUserList; }
    void setSyncUserList( const QStringList & value );

    void addRecentProject( const QString & fileName );
    QStringList recentProjectList() const;
private:
    bool serversChanged( const std::vector< std::shared_ptr< SServerInfo > > & lhs, const std::vector< std::shared_ptr< SServerInfo > > & rhs ) const;
    bool loadServer( const QJsonObject & obj, QString & errorMsg );
    void updateServerMap();

    std::shared_ptr< SServerInfo > getServerInfo( const QString & serverName, bool addIfMissing );
    void updateFriendlyServerNames();
    QColor getColor( const QColor & clr, bool forBackground /*= true */ ) const;
    bool maybeSave( QWidget * parent, std::function<QString()> selectFileFunc, std::function<void( const QString & title, const QString & msg )> errorFunc );

    template< typename T >
    void updateValue( T & lhs, const T & rhs )
    {
        if ( lhs != rhs )
        {
            lhs = rhs;
            fChanged = true;
        }
    }


    QString fFileName;
    std::vector< std::shared_ptr< SServerInfo > > fServers;
    std::map< QString, std::pair< std::shared_ptr< SServerInfo >, size_t > > fServerMap;

    QColor fMediaSourceColor{ "yellow" };
    QColor fMediaDestColor{ "yellow" };
    QColor fMediaDataMissingColor{ "red" };
    int fMaxItems{ -1 };

    bool fOnlyShowSyncableUsers{ true };
    bool fOnlyShowMediaWithDifferences{ true };
    bool fShowMediaWithIssues{ false };

    bool fSyncAudio{ true };
    bool fSyncVideo{ true };
    bool fSyncEpisode{ true };
    bool fSyncMovie{ true };
    bool fSyncTrailer{ true };
    bool fSyncAdultVideo{ true };
    bool fSyncMusicVideo{ true };
    bool fSyncGame{ true };
    bool fSyncBook{ true };

    QStringList fSyncUserList;

    bool fChanged{ false };
};

#endif 
