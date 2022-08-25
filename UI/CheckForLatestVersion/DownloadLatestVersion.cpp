#include "DownloadLatestVersion.h"
#include "qt_prolog.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QNetworkReply>
#include <QDebug>
#include <QAuthenticator>
#include <QXmlStreamReader>
#include <QUrlQuery>
#include <QTimer>
#include "qt_epilog.h"

#include "Shared/Version/Version.h"
#include "Shared/GenUtils/QtUtils.h"
#include "Shared/GenUtils/MD5.h"


namespace NCheckForLatestVersion
{

QNetworkReply * CDownloadLatestVersion::fReply = nullptr;
CDownloadLatestVersion::CDownloadLatestVersion( QObject * parent /*= nullptr */ ) : 
    QObject( parent ),
    fMaintenanceDate( std::make_pair( -1, -1 ) ),
    fCurrentVersion(),
    fManager( nullptr ),
    fHasError( false )
{

    fManager = new QNetworkAccessManager( this );
    connect( fManager, &QNetworkAccessManager::authenticationRequired, this, &CDownloadLatestVersion::slotAuthenticationRequired );
    connect( fManager, &QNetworkAccessManager::finished, this, &CDownloadLatestVersion::slotFinished );
}

CDownloadLatestVersion::~CDownloadLatestVersion()
{

}

void CDownloadLatestVersion::setMaintenanceDate( int month, int year )
{
    fMaintenanceDate = std::make_pair( month, year );
}

void CDownloadLatestVersion::setCurrentVersion( int major, int minor, int build )
{
    fCurrentVersion.fMajor = major; 
    fCurrentVersion.fMinor = minor; 
    fCurrentVersion.fBuild = build;
}

void CDownloadLatestVersion::slotAuthenticationRequired( QNetworkReply * /*reply*/, QAuthenticator * authenticator )
{
    authenticator->setUser( "version_checker" );
    authenticator->setPassword( "sLfhg82!c2" );
}

void CDownloadLatestVersion::slotFinished( QNetworkReply * /*reply*/ )
{
}

void CDownloadLatestVersion::downloadLatestVersion( bool hdlc, bool forHTML )
{
    QString urlStr = "http://licensing.bluepearlsoftware.com/checkLatestVersion/checkLatestVersion.php";

    const char * envVar = "BPS_CHECK_LATEST_VERSION_URL";
    if ( qEnvironmentVariableIsSet( envVar ) )
    {
        QString envUrl = qgetenv( envVar );
        if ( envUrl.isEmpty() )
            return;
        urlStr = envUrl;
    }
    
    QUrl url( urlStr );

    int delay = getTimeOutDelay();

    fErrorString = "";
    fHasError = false;
    QUrlQuery query;
    query.addQueryItem( "build", QString::number( fCurrentVersion.fBuild ) );
    query.addQueryItem( "maintenance_month", QString::number( fMaintenanceDate.first ) );
    query.addQueryItem( "maintenance_year", QString::number( fMaintenanceDate.second ) );
    query.addQueryItem( "html", forHTML ? "1" : "0" );
    query.addQueryItem( "major_only", fMajorReleasesOnly ? "1" : "0" );
    if ( fMajorReleasesOnly )
    {
        query.addQueryItem( "current_major", QString::number( fCurrentVersion.fMajor ) );
        query.addQueryItem( "current_minor", QString::number( fCurrentVersion.fMinor ) );
    }


    if ( hdlc )
        query.addQueryItem( "hdlc", "1" );


    url.setQuery( query );

    QNetworkRequest request;
    request.setUrl( url );
    fReply = fManager->get( request );
    Q_EMIT sigConnectionChanged();
    QEventLoop eLoop;
    QTimer::singleShot( delay, this, &CDownloadLatestVersion::slotKillCurrentReplay );
    connect( this, &CDownloadLatestVersion::sigKillLoop, &eLoop, &QEventLoop::quit );
    connect( fManager, &QNetworkAccessManager::finished, &eLoop, &QEventLoop::quit );
    eLoop.exec( QEventLoop::AllEvents );

    QString txt = fReply->readAll();
    if ( fErrorString.isEmpty() )
        fErrorString = fReply->errorString();
    fHasError = ( fReply->error() != QNetworkReply::NoError );
    fReply->deleteLater();
    fReply = nullptr;
    if ( txt.indexOf( "<b>Error" ) == 0 )
    {
        fHasError = true;
        fErrorString = txt;
    }

    if ( fHasError )
        return;

    Q_EMIT sigConnectionChanged();
    loadResults( txt );
}

int CDownloadLatestVersion::getTimeOutDelay() const
{
    int delay = 10000;
    QString value = qgetenv( "BPS_PHONE_HOME_MAX" );
    if( !value.isEmpty() )
    {
        bool aOK;
        int val = value.toInt( &aOK );
        if( aOK && ( val > 0 ) )
            delay = static_cast< int >( 1.2 * val );
    }
    return delay;
}

void CDownloadLatestVersion::slotKillCurrentReplay()
{
    if ( fReply && !fReply->isFinished() )
    {
        fHasError = true;
        QString path = fReply->url().toDisplayString( QUrl::RemovePassword | QUrl::RemoveUserInfo | QUrl::RemoveFilename | QUrl::RemovePath |QUrl::RemoveQuery |QUrl::StripTrailingSlash );
        fErrorString = tr( "Error connecting to: %1<br>Connection timed out after %2ms." ).arg( path ).arg( getTimeOutDelay() );
        fReply->abort();
    }
    Q_EMIT sigKillLoop();
}

bool SDownloadInfo::load( QXmlStreamReader & reader )
{
    fPlatform = reader.attributes().value( "platform" ).toString();
    if ( fPlatform.isEmpty() )
        reader.raiseError( QString( "download element missing platform attribute" ) );
    fApplicationName = reader.attributes().value( "ApplicationName" ).toString();
    if ( fPlatform.isEmpty() )
        reader.raiseError( QString( "download element missing ApplicationName attribute" ) );
    if ( !reader.hasError() )
        fSize = NQtUtils::getInt( reader.attributes().value( "size" ), reader );
    if ( !reader.hasError() )
    {
        QStringRef tmp = reader.attributes().value( "url" );
        fUrl = QUrl( tmp.toString() );
        if ( !fUrl.isValid() )
            reader.raiseError( QString( "url is not valid '%1'" ).arg( tmp.toString() ) );
    }
    if ( !reader.hasError() )
    {
        auto tmp = reader.attributes().value( "md5sum" );
        if ( tmp.isEmpty() )
            reader.raiseError( QString( "download element missing md5sum attribute" ) );
        else
            fMD5 = QString::fromLatin1( NUtils::formatMd5( tmp.toLocal8Bit(), true ) );
    }
    return !reader.hasError();
}

QString SDownloadInfo::getSize() const
{
    QString retVal;
    quint64 tmp = fSize;
    quint64 prev = fSize;
    if ( tmp < 1024 )
        return QString::number( fSize );

    prev = tmp;
    tmp /= 1024;
    if ( tmp < 1024 )
        return QString( "%1" ).arg( 1.0*prev/1024.0, 5 ) + "kB";
 
    prev = tmp;
    tmp /= 1024;
    if ( tmp < 1024 )
        return QString( "%1" ).arg( 1.0*prev/1024.0, 0, 'g', 5 ) + "MB";

    prev = tmp;
    tmp /= 1024;
    if ( tmp < 1024 )
        return QString( "%1" ).arg( 1.0*prev/1024.0, 0, 'g', 5 ) + "GB";

    prev = tmp;
    tmp /= 1024;
    return QString( "%1" ).arg( 1.0*prev/1024.0, 0, 'g', 5 )  + "TB";
}

bool SVersion::load( QXmlStreamReader & reader )
{
    fMajor = NQtUtils::getInt( reader.attributes().value( "major" ), reader );
    if ( !reader.hasError() )
        fMinor = NQtUtils::getInt( reader.attributes().value( "minor" ), reader );
    if ( !reader.hasError() )
        fBuild = NQtUtils::getInt( reader.attributes().value( "build" ), reader );
    return !reader.hasError();
}

void CDownloadLatestVersion::loadResults( const QString & xml )
{
    fRetVal.clear();
    fReleases.clear();
    if ( fHasError )
        return;

    QXmlStreamReader reader( xml );
    bool inRelease=false;
    while ( !reader.atEnd() && !reader.hasError() )
    {
        QXmlStreamReader::TokenType type = reader.readNext();
        if ( type == QXmlStreamReader::StartElement )
        {
            QStringRef nm = reader.name();
            if ( nm == "Release" )
            {
                SReleaseInfo info;
                if ( !info.fVersion.load( reader ) )
                    continue;
                info.fReleaseDate = NQtUtils::getDateTime( reader.attributes().value( "released" ), reader, false ).date();
                info.fPatchRelease = NQtUtils::getBool( reader.attributes().value( "PatchRelease" ), false );
                info.fDownloadable = NQtUtils::getBool( reader.attributes().value( "Downloadable" ), false );
                info.fInMaintenance = NQtUtils::getBool( reader.attributes().value( "InMaintenance" ), false );
                if ( reader.hasError() )
                    continue;
                fReleases.push_back( info );
                inRelease = true;
            }
            else if ( nm == "ReleaseInfo" )
            {
                if ( !inRelease || fReleases.isEmpty() )
                {
                    reader.raiseError( tr( "ReleaseInfo element must be part of a release" ) );
                    continue;
                }
                QStringRef releaseInfo = reader.attributes().value( "Data" );
                fReleases.back().fReleaseInfo = releaseInfo.toString();
            }
            else if ( nm == "Download" )
            {
                if ( !inRelease || fReleases.isEmpty() )
                {
                    reader.raiseError( tr( "Download element must be part of a release" ) );
                    continue;
                }
                SDownloadInfo info;
                if ( !info.load( reader ) )
                    continue;
                fReleases.back().fDownloads.push_back( info );
            }
        }
        else if ( type == QXmlStreamReader::EndElement )
        {
            QStringRef nm = reader.name();

            if ( nm == "Release" )
                inRelease = false;
        }
    }
    if ( fReleases.empty() )
    {
        fRetVal = "You are running the latest version of the release.";
        return;
    }
    for( int ii = 0; fVerbose && ii < fReleases.size(); ++ii )
    {
        if ( ii != 0 )
        {
            fRetVal += "======================================\n";
        }
        fRetVal += "Version: " + fReleases[ ii ].fVersion.getVersion() + "\n";
        if ( fReleases[ ii ].fPatchRelease )
            fRetVal += "\tPatch Release\n";
        fRetVal += fReleases[ ii ].fReleaseInfo + "\n";
        if ( !fReleases[ ii ].fInMaintenance )
            fRetVal += QString( "\t****Your maintenance contract expired before this release.  Please contact %1 sales at %2.***\n" ).arg( QString::fromStdString( NVersion::getCompanyName( false ) ) ).arg( NVersion::getCompanyDomain() );
        else if ( fReleases[ ii ].fDownloadable )
        {
            for( int jj = 0; jj < fReleases[ ii ].fDownloads.size(); ++jj )
            {
                fRetVal += "\tDownload: " + fReleases[ ii ].fDownloads[ jj ].fPlatform + " - " + fReleases[ ii ].fDownloads[ jj ].fUrl.toString() + " (" + fReleases[ ii ].fDownloads[ jj ].getSize() + ")\n";
            }
        }
    }

    bool found = false;
    bool outOfDateShown = false;
    for( int ii = 0; !fVerbose && !found && ii < fReleases.size(); ++ii )
    {
        if ( ii != 0 )
        {
            fRetVal += "======================================\n";
        }

        fRetVal += "Version: ";
        fRetVal += fReleases[ ii ].fVersion.getVersion() + "\n";
        fRetVal += "\tRelease Date: " + fReleases[ ii ].fReleaseDate.toString() + "\n";

        found = ( fReleases[ ii ].fInMaintenance && fReleases[ ii ].fDownloadable );

        if ( !fReleases[ ii ].fInMaintenance )
        {
            if ( !outOfDateShown )
                fRetVal += QString( "\t****Your maintenance contract expired before this release.  Please contact %1 sales at %2.***\n" ).arg( QString::fromStdString( NVersion::getCompanyName( false ) ) ).arg( NVersion::getCompanyDomain() );
            outOfDateShown = true;
        }

        for( int jj = 0; jj < fReleases[ ii ].fDownloads.size(); ++jj )
        {
            fRetVal += "\tDownload: " + fReleases[ ii ].fDownloads[ jj ].fPlatform + " - " + fReleases[ ii ].fDownloads[ jj ].fUrl.toString() + " (" + fReleases[ ii ].fDownloads[ jj ].getSize() + ")\n";
        }
    }
}

QString CDownloadLatestVersion::latestVersion() const
{
    if ( fHasError )
        return fErrorString;
    return fRetVal;
}

void CDownloadLatestVersion::setVerbose( bool verbose )
{
    fVerbose = verbose;
}

// major.minor.buildnumber-MONTH-YEAR-OS
QString CDownloadLatestVersion::getOverrideOS()
{
    QString forceVersion = qgetenv( "BPS_FORCE_VERSION" );
    if( forceVersion.isEmpty() )
        return QString();

    QStringList lst = forceVersion.split( "-" );
    if ( lst.size() < 3 )
        return QString();
    lst = lst.mid( 3, lst.size() );
    return lst.join( "-" );
}

bool CDownloadLatestVersion::getVersionOverride( int & major, int & minor, int & buildNumber, int & maintMonth, int & maintYear )
{
    QString forceVersion = qgetenv( "BPS_FORCE_VERSION" );
    if ( forceVersion.isEmpty() || forceVersion == "FORCE" ) 
    {
        if( forceVersion == "FORCE" )
        {
            maintYear = 2048;
            maintMonth = 5;
            buildNumber = 5708;
            return true;
        }
        return false;
    }

    // major.minor.buildnumber-MONTH-YEAR-OS
    bool aOK = true;
    QStringList lst= forceVersion.split( "-" );
    if ( lst.size() < 3 )
        return false;

    QString versionString = lst.front();
    QStringList versionList = versionString.split( "." );
    if ( versionList.size() != 3 )
        return false;

    major = versionList.front().toInt( &aOK );
    if ( !aOK )
        return false;
    versionList.pop_front();
    minor = versionList.front().toInt( &aOK );
    if ( !aOK )
        return false;
    versionList.pop_front();
    buildNumber = versionList.front().toInt( &aOK );
    if ( !aOK )
        return false;

    lst.pop_front();
    maintMonth = lst.front().toInt( &aOK );
    if ( !aOK )
        return false;
    lst.pop_front();
    maintYear = lst.front().toInt( &aOK );
    if ( !aOK )
        return false;

    return true;
}


QString SVersion::getVersion() const
{
    return QString( "%1.%2.%3" ).arg( fMajor ).arg( fMinor ).arg( fBuild );
}

QString SReleaseInfo::getKey() const
{
    QString retVal = fVersion.getVersion();
    if ( fDownloads.size() )
    {
        retVal += "-" + fDownloads[ 0 ].fPlatform;
        retVal += "-" + fDownloads[ 0 ].fApplicationName;
    }
    return retVal;
}

QList< SReleaseInfo > SReleaseInfo::splitDownloads() const
{
    QList< SReleaseInfo > retVal;
    if ( fDownloads.empty() )
    {
        retVal.push_back( *this );
    }
    else
    {
        for( int ii = 0; ii < fDownloads.size(); ++ii )
        {
            SReleaseInfo curr;
            curr.fVersion = fVersion;
            curr.fReleaseDate = fReleaseDate;
            curr.fPatchRelease = fPatchRelease;
            curr.fDownloadable = fDownloadable;
            curr.fInMaintenance = fInMaintenance;
            curr.fReleaseInfo = fReleaseInfo;
            curr.fDownloads << fDownloads[ ii ];
            retVal << curr;
        }
    }
    return retVal;
}

}


