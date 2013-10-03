#include "KeepAliveThread.h"

#ifndef WIN32
static void *KeepAliveThreadEntryPoint(void *threadInfo);
#endif

void  KeepAliveThread::StartThread(Socket *theSocket)
{
#ifdef WIN32
   // Ignore the thread handle return since the parent process won't suspend
   // or terminate the thread.
   CreateThread(0,                        // security attributes (0=use parents)
                0,                        // stack size (0=use parents)
                EntryPoint,
                theSocket,
                0,                        // execute immediately
                0);                       // don't care about the thread ID
#else
   pthread_create(&ih_thread, NULL, &KeepAliveThreadEntryPoint, theSocket);
   pthread_detach(ih_thread);
#endif
}

#ifdef WIN32
DWORD WINAPI KeepAliveThread::EntryPoint(LPVOID threadInfo)
#else
static void *KeepAliveThreadEntryPoint(void *threadInfo)
#endif
{
   int32_t  endLoop, counter;
   Socket  *theSocket;
   SharedMemoryItem *threadWaiter;

   theSocket = (Socket *) threadInfo;
   threadWaiter = theSocket->GetThreadWaiter();

   counter = 0;
   endLoop = false;
   while (!endLoop)
   {
      // Wait one second.  If the server is still waiting for MySQL to respond, then
      // send "keepalive" to the client so that it knows that the server is still alive.
      Sleep(1000);

      threadWaiter->Lock();
      counter++;

      if (threadWaiter->GetValueHaveLock() == KAT_WAITING)
      {
         // Do this once every 4 seconds
         if (!(counter & 3))
            theSocket->Send("keepalive");
      }
      else
         endLoop = true;

      threadWaiter->Release();
   }

   threadWaiter->SetValueNoLock(KAT_TERMINATED);

#ifdef WIN32
   return 0;
#else
   pthread_exit(0);
#endif
}
