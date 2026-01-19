#ifndef _DMDivisorServerHelper_

#define _DMDivisorServerHelper_

#include "defs.h"
#include "server.h"
#include "ServerHelper.h"
#include "Log.h"
#include "DBInterface.h"

class DMDivisorServerHelper : public ServerHelper
{
public:
   DMDivisorServerHelper(DBInterface* dbInterface, Log* log) : ServerHelper(dbInterface, log) {};

   DMDivisorServerHelper(DBInterface* dbInterface, globals_t* globals) : ServerHelper(dbInterface, globals) {};

   int32_t  ComputeHoursRemaining(void);

   void     ExpireTests(bool canExpire, int32_t delayCount, delay_t* delays);

   void     UnhideSpecials(int32_t unhideSeconds) {};

   void     LoadABCFile(string abcFile) {};
};

#endif // #ifndef _DMDivisorServerHelper_
