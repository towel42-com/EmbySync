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

#ifndef _MAINWINDOW_H
#define _MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkRequest>
#include <memory>
#include <unordered_set>
#include <optional>
#include <QDateTime>
namespace Ui
{
    class CMainWindow;
}

class CSettings;
class QNetworkAccessManager;
class QNetworkReply;
class QAuthenticator;
class QSslPreSharedKeyAuthenticator;
class QNetworkProxy;
class QSslError;
class QTreeWidget;
class QTreeWidgetItem;
class QProgressDialog;
class QJsonValueRef;

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
struct SMediaDataBase
{
    QString fMediaID;
    uint64_t fLastPlayedPos;
    bool fIsFavorite{ false };
    bool fPlayed{ false };
    QDateTime getLastModifiedTime() const;

    QMap<QString, QVariant> fUserData;

    QTreeWidgetItem * fItem{ nullptr };

    bool fBeenLoaded{ false };

    void loadUserDataFromJSON( const QJsonObject & userDataObj, const QMap<QString, QVariant> & userDataVariant );


};

bool operator==( const SMediaDataBase & lhs, const SMediaDataBase & rhs );
inline bool operator!=( const SMediaDataBase & lhs, const SMediaDataBase & rhs )
{
    return !operator==( lhs, rhs );
}

struct SMediaData
{
    enum EColumn
    {
        eName = 0,
        eMediaID = 1,
        ePlayed = 2,
        eFavorite = 3,
        eLastPlayedPos = 4,
        eLastModified = 5
    };

    static QString computeName( QJsonObject & media );

    bool isMissing( bool isLHS ) const
    {
        return isLHS ? ( fLHSServer && fLHSServer->fMediaID.isEmpty() ) : ( fRHSServer && fRHSServer->fMediaID.isEmpty() );
    }
    
    bool hasMissingInfo() const
    {
        return isMissing( true ) || isMissing( false );
    }
       

    QUrlQuery getMissingItemQuery() const;

    QString getMediaID( bool isLHS ) const
    {
        if ( isLHS )
            return fLHSServer ? fLHSServer->fMediaID : QString();
        else
            return fRHSServer ? fRHSServer->fMediaID: QString();
    }
    bool beenLoaded( bool isLHS ) const 
    { 
        return isLHS ? ( fLHSServer && fLHSServer->fBeenLoaded ) : ( fRHSServer && fRHSServer->fBeenLoaded );
    }

    bool serverDataEqual() const;

    bool mediaWatchedOnServer( bool isLHS )
    {
        return isLHS ? ( fLHSServer && !fLHSServer->fMediaID.isEmpty() ) : ( fRHSServer && !fRHSServer->fMediaID.isEmpty() );
    }

    bool hasProviderIDs() const
    {
        return !fProviders.empty();
    }

    QString getProviderList() const
    {
        QStringList retVal;
        for ( auto && ii : fProviders )
        {
            retVal << ii.first.toLower() + "." + ii.second.toLower();
        }
        return retVal.join( "," );
    }

    bool isPlayed( bool lhs ) const
    {
        return lhs ? ( fLHSServer && fLHSServer->fPlayed ) : ( fRHSServer && fRHSServer->fPlayed );
    }

    bool isFavorite( bool lhs ) const
    {
        return lhs ? ( fLHSServer && fLHSServer->fIsFavorite ) : ( fRHSServer && fRHSServer->fIsFavorite );
    }

    QString lastModified( bool lhs ) const
    {
        return lhs ? fLHSServer->getLastModifiedTime().toString() : fRHSServer->getLastModifiedTime().toString();
    }
    QString lastPlayed( bool lhs ) const
    {
        return ( lastPlayedPos( lhs ) ? QString::number( lastPlayedPos( lhs ) ) : QString() );
    }

    bool bothPlayed() const
    {
        return ( played( true ) != 0 ) && ( played( false ) != 0 );
    }

    bool playPositionTheSame() const
    {
        return lastPlayedPos( true ) == lastPlayedPos( false );
    }

    bool bothFavorites() const
    {
        return isFavorite( true ) && isFavorite( false );
    }

    uint64_t lastPlayedPos( bool lhs ) const
    {
        return lhs ? ( fLHSServer ? fLHSServer->fLastPlayedPos : 0 ) : ( fRHSServer ? fRHSServer->fLastPlayedPos : 0 );
    }

    bool played( bool lhs )  const
    {
        return lhs ? ( fLHSServer ? fLHSServer->fPlayed : false ) : ( fRHSServer ? fRHSServer->fPlayed : false );
    }

    void loadUserDataFromJSON( const QJsonObject & object, bool isLHSServer );

    bool lhsMoreRecent() const;

    void setItem( QTreeWidgetItem * item, bool lhs )
    {
        if ( lhs && fLHSServer )
            fLHSServer->fItem = item;
        else if ( !lhs && fRHSServer )
            fRHSServer->fItem = item;
    }

    QTreeWidgetItem * getItem( bool lhs )
    {
        return lhs ? ( fLHSServer ? fLHSServer->fItem : nullptr ) : ( fRHSServer ? fRHSServer->fItem : nullptr );
    }

    void updateFromOther( std::shared_ptr< SMediaData > other, bool toLHS )
    {
        if ( toLHS )
            fLHSServer = other->fLHSServer;
        else
            fRHSServer = other->fRHSServer;
    }

    void createItems( QTreeWidget * lhsTree, QTreeWidget * rhsTree );

    QStringList getColumns( bool unwatched, bool lhs );
    std::pair< QStringList, QStringList > getColumns( bool unwatched );

    void updateItems( bool unwatched , bool isLHS );
    void setItemColors();
    void setItemColor( QTreeWidgetItem * item, const QColor & clr );
    void setItemColor( QTreeWidgetItem * item, int column, const QColor & clr );

    QString fType;
    QString fName;
    std::map< QString, QString > fProviders;

    std::shared_ptr< SMediaDataBase > fLHSServer;
    std::shared_ptr < SMediaDataBase > fRHSServer;
};

struct SUserData
{
    QString getUserID( bool isLHS ) const
    {
        if ( isLHS )
            return fUserID.first;
        else
            return fUserID.second;
    }

    QString fName;
    std::pair< QString, QString > fUserID;
    QTreeWidgetItem * fItem{ nullptr };
    std::list< std::shared_ptr< SMediaData > > fWatchedMedia;
    std::pair< std::list< std::shared_ptr< SMediaData > >, std::list< std::shared_ptr< SMediaData > > > fWatchedMediaServer;
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

class CMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    CMainWindow( QWidget * parent = nullptr );
    virtual ~CMainWindow() override;

    virtual void closeEvent( QCloseEvent * closeEvent ) override;
    static std::unordered_set< QString > fProviderColumnsByName;
    static std::map< int, QString > fProviderColumnsByColumn;
public Q_SLOTS:
    void slotSettings();

    void loadSettings();

    void slotLoadProject();
    void slotSave();
    void slotRecentMenuAboutToShow();

private Q_SLOTS:
    void slotRequestFinished( QNetworkReply * reply );

    void slotAuthenticationRequired( QNetworkReply * reply, QAuthenticator * authenticator );
    void slotEncrypted( QNetworkReply * reply );

    void slotPreSharedKeyAuthenticationRequired( QNetworkReply * reply, QSslPreSharedKeyAuthenticator * authenticator );
    void slotProxyAuthenticationRequired( const QNetworkProxy & proxy, QAuthenticator * authenticator );
    void slotSSlErrors( QNetworkReply * reply, const QList<QSslError> & errors );

    void slotCurrentItemChanged( QTreeWidgetItem *, QTreeWidgetItem * );
    void slotShowMedia();
    void slotFindMissingMedia();

    void updateColumns( std::shared_ptr< SMediaData > ii );

    void slotReloadUser();
    void slotProcess();
    void slotToggleOnlyShowSyncableUsers();
    void slotToggleOnlyShowMediaWithDifferences();

private:
    void onlyShowSyncableUsers();
    void onlyShowMediaWithDifferences();

    void addToLog( const QString & msg );
    void loadFile( const QString & fileName );
    void setupProgressDlg( const QString & title, bool hasLHSServer, bool hasRHSServer );
    void updateProgressDlg( int count );
    QProgressDialog * progressDlg()
    {
        return std::get< 0 >( fProgressDlg );
    }

    void resetProgressDlg();

    bool isProgressDlgFinished();
    void setProgressDlgFinished( bool isLHS )
    {
        if ( isLHS )
            std::get< 1 >( fProgressDlg ) = true;
        else
            std::get< 2 >( fProgressDlg ) = true;
    }

    void reset();

    void setIsLHS( QNetworkReply * reply, bool isLHS );
    bool isLHSServer( QNetworkReply * reply );

    void setRequestType( QNetworkReply * reply, ERequestType requestType );
    ERequestType requestType( QNetworkReply * reply );

    void setExtraData( QNetworkReply * reply, QVariant extraData );
    QVariant extraData( QNetworkReply * reply );

    void loadUsers( bool isLHS );

    void loadUsers( const QByteArray & data, bool isLHS );

    void incProgressDlg();

    void loadMediaList( const QByteArray & data, bool isLHS, const QString & name );

    void loadMissingMediaItem( const QByteArray & data, const QString & mediaName, bool isLHS );

    void slotMediaLoaded();

    void loadAllMedia();
    void loadAllMedia( bool isLHS );

    void getMissingMedia( std::shared_ptr< SMediaData > mediaData );

    void loadMediaData();
    void loadMediaData( std::shared_ptr< SUserData > user );
    void loadMediaData( const QByteArray & data, bool isLHS, const QString & id );

    void loadUsersPlayedMedia( std::shared_ptr< SUserData > userData, bool isLHS );

    void loadMediaData( std::shared_ptr< SUserData > userData, std::shared_ptr< SMediaData > mediaData, bool isLHS );

    std::shared_ptr< SUserData > getUserData( const QString & name ) const;
    std::tuple< std::shared_ptr< SUserData >, bool, bool > getCurrUserData() const;

    std::shared_ptr< SMediaData > findMediaForProvider( const QString & providerName, const QString & providerID, bool fromLHS );
    void setMediaForProvider( const QString & providerName, const QString & providerID, std::shared_ptr< SMediaData > mediaData, bool isLHS );

    using TMediaIDToMediaData = std::map< QString, std::shared_ptr< SMediaData > >;
    void mergeMediaData(
        TMediaIDToMediaData & lhs,
        TMediaIDToMediaData & rhs,
        bool lhsIsLHS
    );

    void mergeMediaData(
        TMediaIDToMediaData & lhs,
        bool lhsIsLHS
    );

    QStringList getColumns( const QString & name, bool isLHS ) const;

    bool processData( std::shared_ptr< SMediaData > mediaData );
    void setPlayPosition( std::shared_ptr< SMediaData > mediaData );
    void setPlayed( std::shared_ptr< SMediaData > mediaData );
    void setFavorite( std::shared_ptr< SMediaData > mediaData );
    void setLastPlayed( std::shared_ptr< SMediaData > mediaData );

    void updateMedia( std::shared_ptr<SMediaData> mediaData, bool deleteUpdate, const QString & updateType );

    bool handleError( QNetworkReply * reply );

    std::unique_ptr< Ui::CMainWindow > fImpl;
    std::shared_ptr< CSettings > fSettings;

    QNetworkAccessManager * fManager{ nullptr };
    bool fShowMediaPending{ false };

    std::map< QString, std::shared_ptr< SUserData > > fUsers;
    TMediaIDToMediaData fLHSMedia; //server media ID -> media Data
    TMediaIDToMediaData fRHSMedia;

    TMediaIDToMediaData fMissingMedia; //media name to mediaData

    std::unordered_set< std::shared_ptr< SMediaData > > fAllMedia;

    // provider name -> provider ID -> mediaData
    std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< SMediaData > > > fLHSProviderSearchMap; // provider name, to map of id to mediadata
    std::unordered_map< QString, std::unordered_map< QString, std::shared_ptr< SMediaData > > > fRHSProviderSearchMap;

    std::unordered_map< QNetworkReply *, std::unordered_map< int, QVariant > > fAttributes;
    std::tuple< QProgressDialog *, std::optional< bool >, std::optional< bool > > fProgressDlg{ nullptr, {}, {} };
};
#endif 
