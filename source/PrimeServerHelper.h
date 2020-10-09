#ifndef _PrimeServerHelper_

#define _PrimeServerHelper_

#include "defs.h"
#include "server.h"
#include "ServerHelper.h"
#include "Log.h"
#include "DBInterface.h"

class PrimeServerHelper : public ServerHelper
{
public:
   PrimeServerHelper(DBInterface *dbInterface, Log *theLog) : ServerHelper(dbInterface, theLog) {};

   int32_t  ComputeHoursRemaining(void);

   void     ExpireTests(bool canExpire, int32_t delayCount, delay_t *delays);

   void     UnhideSpecials(int32_t unhideSeconds);
};

#endif // #ifndef _PrimeServerHelper_
