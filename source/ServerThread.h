#ifndef _ServerThread_

#define _ServerThread_

#include "defs.h"
#include "server.h"
#include "Thread.h"

class ServerThread : public Thread
{
public:
   void  StartThread(globals_t *globals);

private:

};

#endif // #ifndef _ServerThread_

