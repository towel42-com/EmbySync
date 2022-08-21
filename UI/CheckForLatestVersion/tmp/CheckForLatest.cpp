#include "CheckForLatest.h"
#include "Utils.h"
//#ifdef _WINDOWS
//#include <winsock2.h>
//#include <windows.h>
//#include <Lmcons.h>
//#include <windows.h>
//#include <tlhelp32.h>
//#else
//#include <pwd.h>
//#include <unistd.h>
//#endif
//
#include <QUrl>
//#include <QString>
//#include <QSqlQuery>
//#include <QComboBox>
//#include <QTableView>
//#include <QTreeView>
//#include <QRegExp>
//#include <QHeaderView>
//#include <QDomDocument>
//#include <QCheckBox>
//#include <QLineEdit>
//#include <QTableView>
#include <QFileInfo>
//#include <QUuid>
//#include <QDir>
//#include <QCryptographicHash>
#include <QDesktopServices>
//#include <QSplashScreen>
#include <QMessageBox>
//
//#include "OCS/DBUtils/Credentials.h"
#include "OCS/LatestVersion/LatestVersion.h"
//
//#include "OCS/DBUtils/NoGUI/DBUtils.h"
//#include "OCS/DBUtils/SearchPage/CriteriaModel.h"
//#include "OCS/DBUtils/SortableSQLModel.h"
#include "OCS/DBUtils/Utils.h"
//#include "OCS/QtUtils/QtUtils.h"
//#include "OCS/QtUtils/NoGUI/QtUtils.h"
//#include "OCS/DBUtils/CredentialsValidator.h"
//#include "OCS/DBUtils/Login.h"
//

namespace NUtils
{
    bool checkForLatestVersion( bool force, QWidget * parent )
    {
        CLatestVersion::SVersionInfo vi;
        NUtils::getAppVersion( vi.fMajor, vi.fMinor, vi.fBuild, vi.fSuffix );
        vi.fPubDate = NDBUtils::getAppReleaseDate();
        vi.fVersion = NUtils::getAppVersion( true, false );
        vi.fURL = "http://onshorecs.net/LazarusMinistry/latestversion.php?platform=";
#ifdef _WIN32
		vi.fURL += "win32";
#else 
		vi.fURL += "osx";
#endif
	    CLatestVersion dlg( vi, parent );
        dlg.loadData();
	    if ( ( !force && !dlg.outOfDate() ) || ( dlg.hasError() ) )
		    return false;
		
		if ( dlg.isRequired() )
		{
			QMessageBox::warning( NULL, QObject::tr( "Required Download" ), QObject::tr( "This version is required.  If you do not download, you will not be able to run the tool." ) );
		}

        if ( dlg.exec() == QDialog::Accepted )
        {
            QFileInfo fi( dlg.getDownloadFile() );
            if ( fi.exists() )
            {
                QUrl url = QUrl::fromLocalFile( dlg.getDownloadFile() );
                QDesktopServices::openUrl( url );
                return true;
            }
        }
#ifndef _DEBUG
		if ( dlg.isRequired() )
			return true;
#endif
        return false;
    }
}
