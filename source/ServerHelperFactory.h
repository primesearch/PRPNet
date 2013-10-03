#ifndef _ServerHelperFactory_

#define _ServerHelperFactory_

#include "defs.h"
#include "ServerHelper.h"
#include "DBInterface.h"

class ServerHelperFactory
{
public:
   static ServerHelper   *GetServerHelper(globals_t *globals, DBInterface *dbInterface);
};

#endif // #ifndef _ServerHelperFactory_
