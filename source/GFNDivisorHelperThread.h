#ifndef _GFNDivisorHelperThread_

#define _GFNDivisorHelperThread_

#include "defs.h"
#include "server.h"
#include "HelperThread.h"
#include "HelperSocket.h"
#include "Log.h"
#include "DBInterface.h"

class GFNDivisorHelperThread : public HelperThread
{
public:

protected:
   void        AdminAddRanges(void);
   void        ProcessRequest(string theMessage);
   void        ExpireWorkunitTest(void);
};

#endif // #ifndef _GFNDivisorHelperThread_

