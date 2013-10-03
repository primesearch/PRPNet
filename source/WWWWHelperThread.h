#ifndef _WWWWHelperThread_

#define _WWWWHelperThread_

#include "defs.h"
#include "server.h"
#include "HelperThread.h"
#include "HelperSocket.h"
#include "Log.h"
#include "DBInterface.h"

class WWWWHelperThread : public HelperThread
{
public:

protected:
   void        AdminAddRange(void);
   void        ProcessRequest(string theMessage);
   void        ExpireWorkunitTest(void);
};

#endif // #ifndef _WWWWHelperThread_

