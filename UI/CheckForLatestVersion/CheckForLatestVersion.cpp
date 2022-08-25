#include "CheckForLatestVersion.h"

#include "DownloadLatestVersion.h"

namespace NCheckForLatestVersion
{
    std::string checkForLatestVersion( int month, int year, int major, int minor, int buildNumber, bool hdlc, bool forHTML, bool verbose )
    {
        CDownloadLatestVersion dlv;
        CDownloadLatestVersion::getVersionOverride( major, minor, buildNumber, month, year );
        dlv.setMaintenanceDate( month, year );
        dlv.setCurrentVersion( major, minor, buildNumber );
        dlv.setVerbose( verbose );
        dlv.downloadLatestVersion( hdlc, forHTML );
        return dlv.latestVersion().toStdString();
    }
}
