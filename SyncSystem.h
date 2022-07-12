// The MIT License( MIT )
//
// Copyright( c ) 2020-2022 Scott Aron Bloom
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

#ifndef __SYNCSYSTEM_H
#define __SYNCSYSTEM_H

#include <QObject>
#include <QMap>
#include <QDateTime>
#include <QNetworkRequest>
#include <unordered_set>
#include <optional>
#include <functional>

class QNetworkReply;
class QAuthenticator;
class QSslPreSharedKeyAuthenticator;
class QNetworkProxy;
class QSslError;
class QNetworkAccessManager;
class CUserData;
class CMediaData;
class CSettings;
class QProgressDialog;

enum class ERequestType
{
    eNone,
    eUsers,
    eMediaList,
    eMissingMedia,
    eMediaData,
    eUpdateData
};

QString toString( ERequestType request );

struct SServerReplyInfo
{
    void clear()
    {
        fType = ERequestType::eNone;
        fReply = nullptr;
        fExtraData.clear();
    }

    ERequestType fType{ ERequestType::eNone };
    QNetworkReply * fReply{ nullptr };
    QString fExtraData;
};

constexpr int kLHSServer = QNetworkRequest::User + 1; // boolean
constexpr int kRequestType = QNetworkRequest::User + 2; // ERequestType
constexpr int kExtraData = QNetworkRequest::User + 3; // QVariant

using TMediaIDToMediaData = std::map< QString, std::shared_ptr< CMediaData > >;

class CSyncSystem : public QObject
{
    Q_OBJECT
public:
    CSyncSystem( std::shared_ptr< CSettings > settings, QWidget * parent );
    void setUserItemFunc( std::function< void( std::shared_ptr< CUserData > userData ) > updateUserFunc );
    void setMediaItemFunc( std::function< void( std::shared_ptr< CMediaData > userData ) > mediaItemFunc );
    void setProcessNewMediaFunc( std::function< void( std::shared_ptr< CMediaData > userData ) > processMediaFunc );

    bool isRunning() const;

    void reset();
    void resetMedia();

    void loadUsers();
    void loadUsersMedia( std::shared_ptr< CUserData > user );
    void process();

    std::shared_ptr< CUserData > getUserData( const QString & name ) const;

    void forEachUser( std::function< void( std::shared_ptr< CUserData > userData ) > onUser );
    void forEachMedia( std::function< void( std::shared_ptr< CMediaData > media ) > onMediaItem );

    void resetProgressDlg() const;
Q_SIGNALS:
    void sigAddToLog( const QString & msg );
    void sigLoadingUsersFinished();
    void sigUserMediaLoaded();
    void sigMissingMediaLoaded();
public Q_SLOTS:
    void slotFindMissingMedia();
private:
    QWidget * parentWidget() const;

    void loadUsersPlayedMedia( bool isLHSServer );
    bool processData( std::shared_ptr< CMediaData > mediaData );

    void setPlayed( std::shared_ptr< CMediaData > mediaData );
    void setPlayPosition( std::shared_ptr< CMediaData > mediaData );
    void setFavorite( std::shared_ptr< CMediaData > mediaData );
    void setLastPlayed( std::shared_ptr< CMediaData > mediaData );
    void setMediaData( std::shared_ptr< CMediaData > mediaData, bool deleteUpdate, const QString & updateType );

    QProgressDialog * progressDlg() const;
    void setupProgressDlg( const QString & title, bool hasLHSServer, bool hasRHSServer );

    void updateProgressDlg( int count );
    void incProgressDlg();
    bool isProgressDlgFinished() const;
    void setProgressDlgFinished( bool isLHS );

    void loadUsers( bool forLHSServer );

    void setIsLHS( QNetworkReply * reply, bool isLHS );
    bool isLHSServer( QNetworkReply * reply );

    void setRequestType( QNetworkReply * reply, ERequestType requestType );
    ERequestType requestType( QNetworkReply * reply );

    void setExtraData( QNetworkReply * reply, QVariant extraData );
    QVariant extraData( QNetworkReply * reply );

private Q_SLOTS:
    void slotRequestFinished( QNetworkReply * reply );
    void slotMergeMedia();

    void slotAuthenticationRequired( QNetworkReply * reply, QAuthenticator * authenticator );
    void slotEncrypted( QNetworkReply * reply );
    void slotPreSharedKeyAuthenticationRequired( QNetworkReply * reply, QSslPreSharedKeyAuthenticator * authenticator );
    void slotProxyAuthenticationRequired( const QNetworkProxy & proxy, QAuthenticator * authenticator );
    void slotSSlErrors( QNetworkReply * reply, const QList<QSslError> & errors );

private:
    bool handleError( QNetworkReply * reply );
    void loadUsers( const QByteArray & data, bool isLHS );
    void loadMissingMediaItem( const QByteArray & data, const QString & mediaName, bool isLHS );
    bool checkForMissingMedia();
    void getMissingMedia( std::shared_ptr< CMediaData > mediaData );

    void loadMediaList( const QByteArray & data, bool isLHS );
    void loadMediaData();
    void loadMediaData( std::shared_ptr< CMediaData > mediaData, bool isLHS );
    void loadMediaData( const QByteArray & data, bool isLHS, const QString & id );
    void mergeMediaData( TMediaIDToMediaData & lhs, TMediaIDToMediaData & rhs, bool lhsIsLHS );
    void mergeMediaData( TMediaIDToMediaData & lhs, bool lhsIsLHS );

    std::shared_ptr< CMediaData > findMediaForProvider( const QString & providerName, const QString & providerID, bool fromLHS );
    void setMediaForProvider( const QString & providerName, const QString & providerID, std::shared_ptr< CMediaData > mediaData, bool isLHS );

    std::shared_ptr< CSettings > fSettings;

    QNetworkAccessManager * fManager{ nullptr };
    bool fLoadingMediaData{ false };
    std::map< QString, std::shared_ptr< CUserData > > fUsers;

    bool fShowMediaPending{ false };
    TMediaIDToMediaData fLHSMedia; //server media ID -> media Data
    TMediaIDToMediaData fRHSMedia;

    TMediaIDToMediaData fMissingMedia; //media name to mediaData

    std::unordered_set< std::shared_ptr< CMediaData > > fAllMedia;

    // provider name -> provider ID -> mediaData
    std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > > fLHSProviderSearchMap; // provider name, to map of id to mediadata
    std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< CMediaData > > > fRHSProviderSearchMap;

    std::unordered_map< QNetworkReply *, std::unordered_map< int, QVariant > > fAttributes;

    std::function< void( std::shared_ptr< CUserData > userData ) > fUpdateUserFunc;
    std::function< void( std::shared_ptr< CMediaData > userData ) > fUpdateMediaFunc;
    std::function< void( std::shared_ptr< CMediaData > userData ) > fProcessNewMediaFunc;

    std::tuple< QProgressDialog *, std::optional< bool >, std::optional< bool > > fProgressDlg{ nullptr, {}, {} };
    std::shared_ptr< CUserData > fCurrUserData;
};
#endif 
