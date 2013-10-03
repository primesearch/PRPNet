#ifndef _KeepAliveThread_

#define _KeepAliveThread_

#include "defs.h"
#include "server.h"
#include "Thread.h"
#include "Socket.h"

#define KAT_WAITING     1
#define KAT_TERMINATE   5
#define KAT_TERMINATED  9

class KeepAliveThread : public Thread
{
public:
   void  StartThread(Socket *theSocket);

private:
#ifdef WIN32
   static DWORD WINAPI EntryPoint(LPVOID threadInfo);
#endif
};

#endif // #ifndef _KeepAliveThread_

