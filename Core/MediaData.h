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
class QListWidgetItem;
class QJsonObject;
class QTreeWidget;
class QListWidget;
class QColor;
class QStandardItemModel;
class QStandardItem;
class CSettings;

struct SMediaUserData
{
    QString fMediaID;
    bool fIsFavorite{ false };
    bool fPlayed{ false };
    QDateTime fLastPlayedDate;
    uint64_t fPlayCount;
    uint64_t fPlaybackPositionTicks; // 1 tick = 10000 ms

    uint64_t playbackPositionMSecs() const;
    void setPlaybackPositionMSecs( uint64_t msecs );

    QTime playbackPositionTime() const;
    void setPlaybackPosition( const QTime & time );

    QString playbackPosition() const;

    bool userDataEqual( const SMediaUserData & rhs ) const;

    QJsonObject userDataJSON() const;
    void loadUserDataFromJSON( const QJsonObject & userDataObj );

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
    static std::function< QString( uint64_t ) > mecsToStringFunc();
    static QString computeName( QJsonObject & media );

    CMediaData( const QString & name, const QString & type );
    QString name() const;
    bool beenLoaded( bool isLHS ) const;

    void loadUserDataFromJSON( const QJsonObject & object, bool isLHSServer );
    void updateFromOther( std::shared_ptr< CMediaData > other, bool toLHS );
    QUrlQuery getSearchForMediaQuery() const;

    QString getProviderID( const QString & provider );
    std::map< QString, QString > getProviders( bool addKeyIfEmpty = false ) const;
    std::map< QString, QString > getExternalUrls() const { return fExternalUrls; }

    QString externalUrlsText() const;

    bool userDataEqual() const;

    QString getMediaID( bool isLHS ) const;
    void setMediaID( const QString & id, bool isLHS );

    bool isMissingOnServer( bool isLHS ) const;
    bool isMissingOnEitherServer() const;

    bool hasProviderIDs() const;
    void addProvider( const QString & providerName, const QString & providerID );

    bool lastPlayedTheSame() const;
    bool rhsNeedsUpdating() const;
    bool lhsNeedsUpdating() const;

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

    std::shared_ptr<SMediaUserData> userMediaData( bool lhs ) const { return lhs ? fLHSUserMediaData : fRHSUserMediaData; }

    std::shared_ptr<SMediaUserData> lhsUserMediaData() const { return fLHSUserMediaData; }
    std::shared_ptr<SMediaUserData> rhsUserMediaData() const { return fRHSUserMediaData; }

    QString getDirectionLabel() const;
    int getDirectionValue() const;
private:
    QString getProviderList() const;

    QString fType;
    QString fName;
    std::map< QString, QString > fProviders;
    std::map< QString, QString > fExternalUrls;


    std::shared_ptr< SMediaUserData > fLHSUserMediaData;
    std::shared_ptr< SMediaUserData > fRHSUserMediaData;

    static std::function< QString( uint64_t ) > sMSecsToStringFunc;
};
#endif 
