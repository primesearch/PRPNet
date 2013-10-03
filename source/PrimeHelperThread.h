#ifndef _PrimeHelperThread_

#define _PrimeHelperThread_

#include "defs.h"
#include "server.h"
#include "HelperThread.h"
#include "HelperSocket.h"
#include "Log.h"
#include "DBInterface.h"

class PrimeHelperThread : public HelperThread
{
public:

protected:
   void        AdminABCFile(void);
   void        AdminFactorFile(void);
   void        ProcessRequest(string theMessage);
   void        ExpireWorkunitTest(void);
};

#endif // #ifndef _PrimeHelperThread_
