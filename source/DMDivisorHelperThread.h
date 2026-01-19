#ifndef _DMDivisorHelperThread_

#define _DMDivisorHelperThread_

#include "defs.h"
#include "server.h"
#include "HelperThread.h"
#include "HelperSocket.h"
#include "Log.h"
#include "DBInterface.h"

class DMDivisorHelperThread : public HelperThread
{
public:

protected:
   void        AdminAddRanges(void);
   void        ProcessRequest(string theMessage);
   void        ExpireWorkunitTest(void);
};

#endif // #ifndef _DMDivisorHelperThread_

