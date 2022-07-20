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
struct SMediaUserData;
class CSettings;

enum class ERequestType
{
    eNone,
    eUsers,
    eMediaList,
    eGetMediaInfo,
    eReloadMediaData,
    eUpdateData,
    eUpdateFavorite
};

QString toString( ERequestType request );

struct SProgressFunctions
{
    void setupProgress( const QString & title );

    void setMaximum( int count );
    void incProgress();
    void resetProgress() const;
    bool wasCanceled() const;

    std::function< void( const QString & title ) > fSetupFunc;
    std::function< void( int ) > fSetMaximumFunc;
    std::function< void() > fIncFunc;
    std::function< void() > fResetFunc;
    std::function< bool() > fWasCanceledFunc;
};

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
    CSyncSystem( std::shared_ptr< CSettings > settings, QObject * parent = nullptr );
    void setAddUserItemFunc( std::function< void( std::shared_ptr< CUserData > userData ) > updateUserFunc );
    void setMediaItemFunc( std::function< void( std::shared_ptr< CMediaData > userData ) > mediaItemFunc );
    void setProcessNewMediaFunc( std::function< void( std::shared_ptr< CMediaData > userData ) > processMediaFunc );
    void setUserMsgFunc( std::function< void( const QString & title, const QString & msg, bool isCritical ) > userMsgFunc );

    void setProgressFunctions( const SProgressFunctions & funcs );

    bool isRunning() const;

    void reset();
    void resetMedia();

    void loadUsers();
    void loadUsersMedia( std::shared_ptr< CUserData > user );
    void clearCurrUser();

    std::shared_ptr< CUserData > getUserData( const QString & name ) const;

    void forEachUser( std::function< void( std::shared_ptr< CUserData > userData ) > onUser );
    void forEachMedia( std::function< void( std::shared_ptr< CMediaData > media ) > onMediaItem );

    std::unordered_set< std::shared_ptr< CMediaData > > getAllMedia() const { return fAllMedia; }

    std::shared_ptr< CUserData > currUser() const;

    void updateUserDataForMedia( std::shared_ptr<CMediaData> mediaData, std::shared_ptr<SMediaUserData> newData, bool lhsNeedsUpdating );
Q_SIGNALS:
    void sigAddToLog( const QString & msg );
    void sigLoadingUsersFinished();
    void sigUserMediaLoaded();
    void sigUserMediaCompletelyLoaded();
    void sigFindingMediaInfoFinished();
public Q_SLOTS:
    void slotFindMissingMedia();

    void slotProcess();
    void slotProcessToLeft();
    void slotProcessToRight();
private:
    void process( bool forceLeft, bool forceRight );
    void requestUsersPlayedMedia( bool isLHSServer );
    bool processData( std::shared_ptr< CMediaData > mediaData, bool forceLeft, bool forceRight );

    void requestUsers( bool forLHSServer );

    void setIsLHS( QNetworkReply * reply, bool isLHS );
    bool isLHSServer( QNetworkReply * reply );

    void setRequestType( QNetworkReply * reply, ERequestType requestType );
    ERequestType requestType( QNetworkReply * reply );

    void setExtraData( QNetworkReply * reply, QVariant extraData );
    QVariant extraData( QNetworkReply * reply );

private Q_SLOTS:
    void slotRequestFinished( QNetworkReply * reply );
    void postHandlRequest( ERequestType requestType );
    void decRequestCount( ERequestType requestType );

    void slotMergeMedia();

    void slotAuthenticationRequired( QNetworkReply * reply, QAuthenticator * authenticator );
    void slotEncrypted( QNetworkReply * reply );
    void slotPreSharedKeyAuthenticationRequired( QNetworkReply * reply, QSslPreSharedKeyAuthenticator * authenticator );
    void slotProxyAuthenticationRequired( const QNetworkProxy & proxy, QAuthenticator * authenticator );
    void slotSSlErrors( QNetworkReply * reply, const QList<QSslError> & errors );

private:
    void loadMediaData( const QString & mediaID, bool isLHSServer );
    bool isLastRequestOfType( ERequestType type ) const;

    bool handleError( QNetworkReply * reply );
    void loadUsers( const QByteArray & data, bool isLHS );
    void loadMediaInfo( const QByteArray & data, const QString & mediaName, bool isLHS );
    bool checkForMissingMedia();
    void requestMediaInformation( std::shared_ptr< CMediaData > mediaData, bool forLHS );

    void loadMediaList( const QByteArray & data, bool isLHS );

    void loadMedia( QJsonObject & media, bool isLHSServer );

    void addMediaInfo( std::shared_ptr<CMediaData> mediaData, const QJsonObject & mediaInfo, bool isLHSServer );

    void requestReloadMediaData( std::shared_ptr< CMediaData > mediaData, bool isLHS );
    void loadMediaData( const QByteArray & data, bool isLHS, const QString & id );
    void reloadMediaData( const QByteArray & data, bool isLHS, const QString & id );
    void mergeMediaData( TMediaIDToMediaData & lhs, TMediaIDToMediaData & rhs, bool lhsIsLHS );
    void mergeMediaData( TMediaIDToMediaData & lhs, bool lhsIsLHS );

    std::shared_ptr< CMediaData > findMediaForProvider( const QString & providerName, const QString & providerID, bool fromLHS );
    void setMediaForProvider( const QString & providerName, const QString & providerID, std::shared_ptr< CMediaData > mediaData, bool isLHS );

    void requestSetFavorite( std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData, bool sendToLHS );
    void requestUpdateUserDataForMedia( std::shared_ptr< CMediaData > mediaData, std::shared_ptr< SMediaUserData > newData, bool sendToLHS );

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

    std::unordered_map< ERequestType, int > fRequests;
    std::unordered_map< QNetworkReply *, std::unordered_map< int, QVariant > > fAttributes;

    std::function< void( std::shared_ptr< CUserData > userData ) > fAddUserFunc;
    std::function< void( std::shared_ptr< CMediaData > mediaData ) > fUpdateMediaFunc;
    std::function< void( std::shared_ptr< CMediaData > mediaData ) > fProcessNewMediaFunc;
    std::function< void( const QString & title, const QString & msg, bool isCritical ) > fUserMsgFunc;
    SProgressFunctions fProgressFuncs;

    using TOptionalBoolPair = std::pair< std::optional< bool >, std::optional< bool > >;
    std::unordered_map< QString, TOptionalBoolPair > fLeftAndRightFinished;
    std::shared_ptr< CUserData > fCurrUserData;
};
#endif
