#ifndef _WWWWServerHelper_

#define _WWWWServerHelper_

#include "defs.h"
#include "server.h"
#include "ServerHelper.h"
#include "Log.h"
#include "DBInterface.h"

class WWWWServerHelper : public ServerHelper
{
public:
   WWWWServerHelper(DBInterface *dbInterface, Log *theLog) : ServerHelper(dbInterface, theLog) {};

   int32_t  ComputeHoursRemaining(void);

   void     ExpireTests(bool canExpire, int32_t delayCount, delay_t *delays);

   void     UnhideSpecials(int32_t unhideSeconds);
};

#endif // #ifndef _WWWWServerHelper_
