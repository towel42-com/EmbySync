#ifndef __SHARED_DOWNLOADLATESTVERSION_H
#define __SHARED_DOWNLOADLATESTVERSION_H

#include "qt_prolog.h"
#include <QObject>
#include <QUrl>
#include <QDate>
#include "qt_epilog.h"

class QEventLoop;
class QNetworkAccessManager;
class QNetworkReply;
class QAuthenticator;
class QXmlStreamReader;

namespace NCheckForLatestVersion
{
struct SVersion
{
    SVersion() :
        fMajor( -1 ),
        fMinor( -1 ),
        fBuild( -1 )
    {
    }

    SVersion( const SVersion & rhs ) :
        fMajor( rhs.fMajor ),
        fMinor( rhs.fMinor ),
        fBuild( rhs.fBuild )
    {
    }

    SVersion & operator=( const SVersion & rhs )
    {
        fMajor = rhs.fMajor;
        fMinor = rhs.fMinor;
        fBuild = rhs.fBuild;
        return *this;
    }

    QString getVersion() const;
    bool load( QXmlStreamReader & reader );
    int fMajor;
    int fMinor;
    int fBuild;
};


struct SDownloadInfo
{
    SDownloadInfo() :
        fSize( 0 )
    {
    }

    QString fApplicationName;
    QUrl fUrl;
    QString fPlatform;
    int fSize{0};
    QString fMD5;


    bool isPlatform( const QString & platform ) const
    {
        return fPlatform == platform;
    }
    bool load( QXmlStreamReader & reader );
    QString getSize() const;
};

struct SReleaseInfo
{
    SReleaseInfo() :
        fPatchRelease( false ),
        fDownloadable( false ),
        fInMaintenance( false )
    {
    }

    QList< SReleaseInfo > splitDownloads() const;
    QString getKey() const;

    SVersion fVersion;
    QDate fReleaseDate;
    bool fPatchRelease;
    bool fDownloadable;
    bool fInMaintenance;
    QString fReleaseInfo;
    QString fMD5;
    QList< SDownloadInfo > fDownloads;
};

class CDownloadLatestVersion : public QObject
{
Q_OBJECT;
public:
    CDownloadLatestVersion( QObject * parent = nullptr );
    ~CDownloadLatestVersion();

    void setMaintenanceDate( int month, int year );
    void setCurrentVersion( int major, int minor, int build );
    void setVerbose( bool verbose );

    void setMajorReleasesOnly( bool majorOnly ){ fMajorReleasesOnly = majorOnly; }
    void downloadLatestVersion( bool hdlc, bool forHTML );

    QString latestVersion() const;
    QList< SReleaseInfo > getLatestVersionInfo() const{ return fReleases; }

    static bool getVersionOverride( int & major, int & minor, int & buildNumber, int & month, int & year );
    static QString getOverrideOS();

    bool hasError()const { return fHasError; }
    QString errorString() const{ return fErrorString; }

    static bool isActive(){ return fReply != nullptr; }
private Q_SLOTS:
    void slotAuthenticationRequired( QNetworkReply * reply, QAuthenticator * authenticator );
    void slotFinished( QNetworkReply * reply );
    void slotKillCurrentReplay();

Q_SIGNALS:
    void sigKillLoop();
    void sigConnectionChanged();

private:
    int getTimeOutDelay() const;
    void loadResults( const QString & xml );

    std::pair< int, int > fMaintenanceDate;
    SVersion fCurrentVersion;
    bool fVerbose;
    bool fMajorReleasesOnly{ true };

    QNetworkAccessManager * fManager;
    static QNetworkReply * fReply;
    bool fHasError;
    QString fErrorString;
    QString fRetVal;
    QList< SReleaseInfo > fReleases;
};
}

#endif
