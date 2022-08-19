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

#ifndef __MEDIADATA_H
#define __MEDIADATA_H

#include <QString>
#include <QMap>
#include <QUrlQuery>
#include <QDateTime>
#include <QIcon>

#include <memory>

class QVariant;
class QListWidgetItem;
class QJsonObject;
class QTreeWidget;
class QListWidget;
class QColor;
class CSettings;
struct SMediaUserData;

class CMediaData
{
public:
    static QStringList getHeaderLabels();
    static void setMSecsToStringFunc( std::function< QString( uint64_t ) > func );
    static std::function< QString( uint64_t ) > mecsToStringFunc();
    static QString computeName( const QJsonObject & media );

    CMediaData( const QJsonObject & mediaObj, std::shared_ptr< CSettings > settings );
    bool hasProviderIDs() const;
    void addProvider( const QString & providerName, const QString & providerID );

    QString name() const;
    QString mediaType() const;
    bool beenLoaded( const QString & serverName ) const;

    void loadUserDataFromJSON( const QString & serverName, const QJsonObject & object );
    void updateFromOther( const QString & otherServerName, std::shared_ptr< CMediaData > other );

    QUrlQuery getSearchForMediaQuery() const;

    QString getProviderID( const QString & provider );
    std::map< QString, QString > getProviders( bool addKeyIfEmpty = false ) const;
    std::map< QString, QString > getExternalUrls() const { return fExternalUrls; }

    QString externalUrlsText() const;

    bool userDataEqual() const;

    QString getMediaID( const QString & serverName ) const;
    void setMediaID( const QString & serverName, const QString & id );

    void updateCanBeSynced();

    bool isValidForServer( const QString & serverName ) const;
    bool isValidForAllServers() const;
    bool canBeSynced() const;

    bool needsUpdating( const QString & serverName ) const;
    template <class T>
    void needsUpdating( T ) const = delete;

    bool allPlayedEqual() const;
    bool isPlayed( const QString & serverName ) const;

    QString playbackPosition( const QString & serverName ) const;
    uint64_t playbackPositionMSecs( const QString & serverName ) const;
    uint64_t playbackPositionTicks( const QString & serverName ) const;
    QTime playbackPositionTime( const QString & serverName ) const;

    bool allPlaybackPositionTicksEqual() const;

    bool allFavoriteEqual() const;
    bool isFavorite( const QString & serverName ) const;

    QDateTime lastPlayed( const QString & serverName ) const;
    bool allLastPlayedEqual() const;

    uint64_t playCount( const QString & serverName ) const;
    bool allPlayCountEqual() const;

    std::shared_ptr<SMediaUserData> userMediaData( const QString & serverName ) const;
    std::shared_ptr<SMediaUserData> newestMediaData() const;

    QIcon getDirectionIcon( const QString & serverName ) const;
private:
    template< typename T >
    bool allEqual( std::function< T( std::shared_ptr< SMediaUserData > ) > func ) const
    {
        std::optional< T > prevValue;
        for ( auto && ii : fInfoForServer )
        {
            if ( !ii.second->isValid() )
                continue;

            if ( !prevValue.has_value() )
                prevValue = func( ii.second );
            else if ( prevValue.value() != func( ii.second ) )
                      return false;
        }
        return true;
    }
    QString getProviderList() const;

    QString fType;
    QString fName;
    std::map< QString, QString > fProviders;
    std::map< QString, QString > fExternalUrls;

    bool fCanBeSynced{ false };
    std::map< QString, std::shared_ptr< SMediaUserData > > fInfoForServer;

    static std::function< QString( uint64_t ) > sMSecsToStringFunc;

};
#endif 
