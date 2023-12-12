#ifndef  _ServerHelper_
#define  _ServerHelper_

#include <string>
#include "defs.h"
#include "server.h"
#include "DBInterface.h"

class ServerHelper
{
public:
   inline  ServerHelper(DBInterface* dbInterface, Log* log)
   {
      ip_DBInterface = dbInterface;
      ip_Log = log;
   };

   // This one is used only when loading an ABC file upon startup.
   inline  ServerHelper(DBInterface* dbInterface, globals_t *globals) 
   {
      ip_DBInterface = dbInterface;
      ip_Log = globals->p_Log;
      ii_ServerType = globals->i_ServerType;
      ib_NeedsDoubleCheck = globals->b_NeedsDoubleCheck;
   };

   virtual ~ServerHelper() {};

   virtual int32_t   ComputeHoursRemaining(void) { return 0; };

   virtual void      ExpireTests(bool canExpire, int32_t delayCount, delay_t *delays) {};

   virtual void      UnhideSpecials(int32_t unhideSeconds) {};

   virtual void      LoadABCFile(string abcFile) {};

protected:
   DBInterface  *ip_DBInterface;
   Log          *ip_Log;
   int32_t       ii_ServerType;
   bool          ib_NeedsDoubleCheck;
};

#endif
