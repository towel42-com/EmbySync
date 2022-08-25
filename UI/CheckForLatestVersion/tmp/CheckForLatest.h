#ifndef __CHECKFORTLATEST_H
#define __CHECKFORTLATEST_H

class QString;
#include "UserRoles.h"
#include <QString>
#include <QSqlQuery>
#include <QSqlDatabase>
class QSplashScreen;
namespace NDBUtils{ class CSortableSQLModel; }

namespace NUtils
{
	bool checkForLatestVersion( bool force, QWidget * parent );
}
#endif
