#ifndef  _ServerHelper_
#define  _ServerHelper_

#include <string>
#include "defs.h"
#include "server.h"
#include "DBInterface.h"

class ServerHelper
{
public:
   inline  ServerHelper(DBInterface *dbInterface, Log *theLog) { ip_DBInterface = dbInterface; ip_Log = theLog; };

   virtual ~ServerHelper() {};

   virtual int32_t   ComputeHoursRemaining(void) { return 0; };

   virtual void      ExpireTests(bool canExpire, int32_t delayCount, delay_t *delays) {};

   virtual void      UnhideSpecials(int32_t unhideSeconds) {};

protected:
   DBInterface  *ip_DBInterface;
   Log          *ip_Log;
};

#endif
