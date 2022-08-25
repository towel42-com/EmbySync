#ifndef __SHARED_CHECK_FOR_LATEST_VERSION_H
#define __SHARED_CHECK_FOR_LATEST_VERSION_H

#include <string>
namespace NCheckForLatestVersion
{
    std::string checkForLatestVersion( int month, int year, int major, int minor, int buildNumber, bool hdlc, bool forHTML, bool verbose );
}


#endif
