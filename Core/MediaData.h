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

class QVariant;
class QTreeWidgetItem;
class QListWidgetItem;
class QJsonObject;
class QTreeWidget;
class QListWidget;
class QColor;

class CSettings;

struct SMediaUserData
{
    QString fMediaID;
    bool fIsFavorite{ false };
    bool fPlayed{ false };
    QDateTime fLastPlayedDate;
    uint64_t fPlayCount;
    uint64_t fPlaybackPositionTicks; // 1 tick = 10000 ms

    bool userDataEqual( const SMediaUserData & rhs, bool includeTimeStamp ) const;

    QJsonObject toJSON() const;
    void loadUserDataFromJSON( const QJsonObject & userDataObj );

    QTreeWidgetItem * fItem{ nullptr };
    bool fBeenLoaded{ false };
};

bool operator==( const SMediaUserData & lhs, const SMediaUserData & rhs );
inline bool operator!=( const SMediaUserData & lhs, const SMediaUserData & rhs )
{
    return !operator==( lhs, rhs );
}

class CMediaData
{
public:
    enum EColumn
    {
        eName = 0,
        eMediaID,
        eFavorite,
        ePlayed,
        eLastPlayed,
        ePlayCount,
        ePlaybackPosition
    };

    static QStringList getHeaderLabels();
    static void setMSecsToStringFunc( std::function< QString( uint64_t ) > func );
    static QString computeName( QJsonObject & media );

    CMediaData( const QString & name, const QString & type );
    QString name() const;
    bool beenLoaded( bool isLHS ) const;

    void loadUserDataFromJSON( const QJsonObject & object, bool isLHSServer );
    void updateFromOther( std::shared_ptr< CMediaData > other, bool toLHS );
    QUrlQuery getSearchForMediaQuery() const;

    void updateItems( const std::map< int, QString > & providersByColumn, std::shared_ptr< CSettings > settings );
    std::pair< QTreeWidgetItem *, QTreeWidgetItem * > createItems( QTreeWidget * lhsTree, QTreeWidget * rhsTree, QTreeWidget * dirTree, const std::map< int, QString > & providersByColumn, std::shared_ptr< CSettings > settings );

    const std::map< QString, QString > & getProviders() const { return fProviders; }
    bool userDataEqual( bool includeLastPlayedTimestamp ) const;

    QTreeWidgetItem * getItem( bool lhs );
    QTreeWidgetItem * dirItem() const { return fDirItem; }

    QString getMediaID( bool isLHS ) const;
    void setMediaID( const QString & id, bool isLHS );
    bool isMissing( bool isLHS ) const;


    bool hasMissingInfo() const;

    bool hasProviderIDs() const;
    void addProvider( const QString & providerName, const QString & providerID );

    bool lastPlayedTheSame() const;
    bool rhsLastPlayedOlder() const;
    bool lhsLastPlayedOlder() const;

    bool bothPlayed() const;
    bool isPlayed( bool lhs ) const;

    bool playbackPositionTheSame() const;
    QString playbackPosition( bool lhs ) const;
    uint64_t playbackPositionMSecs( bool lhs ) const;
    uint64_t playbackPositionTicks( bool lhs ) const;// 10,000 ticks = 1 ms
    QTime playbackPositionTime( bool lhs ) const;

    bool bothFavorites() const;
    bool isFavorite( bool lhs ) const;

    QDateTime lastPlayed( bool lhs ) const;
    uint64_t playCount( bool lhs ) const;
    std::shared_ptr<SMediaUserData> lhsUserData() const { return fLHSUserData; }
    std::shared_ptr<SMediaUserData> rhsUserData() const { return fRHSUserData; }
private:

    QString getProviderList() const;

    void setItem( QTreeWidgetItem * item, bool lhs );
    QStringList getColumns( bool forLHS );
    std::pair< QStringList, QStringList > getColumns( const std::map< int, QString > & providersByColumn );

    QTreeWidgetItem * getItem( bool forLHS ) const;

    QString getDirectionLabel() const;


    void setItemColors( std::shared_ptr< CSettings > settings );
    void updateItems( const QStringList & columnsData, bool forLHS, std::shared_ptr< CSettings > settings );
    QString fType;
    QString fName;
    std::map< QString, QString > fProviders;

    QTreeWidgetItem * fDirItem{ nullptr };
    std::shared_ptr< SMediaUserData > fLHSUserData;
    std::shared_ptr< SMediaUserData > fRHSUserData;

    static std::function< QString( uint64_t ) > sMSecsToStringFunc;
};
#endif 
