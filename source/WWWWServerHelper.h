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
   WWWWServerHelper(DBInterface* dbInterface, Log *log) : ServerHelper(dbInterface, log) {};

   WWWWServerHelper(DBInterface *dbInterface, globals_t *globals) : ServerHelper(dbInterface, globals) {};

   int32_t  ComputeHoursRemaining(void);

   void     ExpireTests(bool canExpire, int32_t delayCount, delay_t *delays);

   void     UnhideSpecials(int32_t unhideSeconds);

   void     LoadABCFile(string abcFile);
};

#endif // #ifndef _WWWWServerHelper_
