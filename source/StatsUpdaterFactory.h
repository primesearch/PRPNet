#ifndef _StatsUpdaterFactory_

#define _StatsUpdaterFactory_

#include "defs.h"
#include "StatsUpdater.h"
#include "DBInterface.h"
#include "Log.h"

class StatsUpdaterFactory
{
public:
   StatsUpdaterFactory(void) {};

   StatsUpdater   *GetInstance(DBInterface *dbInterface, Log *theLog, int32_t serverType, bool needsDoubleCheck);
};

#endif // #ifndef _StatsUpdaterFactory_
