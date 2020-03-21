#include "ServerThread.h"
#include "ServerSocket.h"
#include "HelperSocket.h"
#include "HelperThread.h"
#include "PrimeHelperThread.h"
#include "WWWWHelperThread.h"

#ifdef WIN32
   static DWORD WINAPI ServerThreadEntryPoint(LPVOID threadInfo);
#else
   static void  *ServerThreadEntryPoint(void *threadInfo);
#endif

void  ServerThread::StartThread(globals_t *globals)
{
   globals->p_ServerSocket = new ServerSocket(globals->p_Log, globals->i_PortID);

#ifdef WIN32
   // Ignore the thread handle return since the parent process won't suspend
   // or terminate the thread.
   CreateThread(0,                        // security attributes (0=use parents)
                0,                        // stack size (0=use parents)
                ServerThreadEntryPoint,
                globals,
                0,                        // execute immediately
                0);                       // don't care about the thread ID
#else
   pthread_create(&ih_thread, NULL, &ServerThreadEntryPoint, globals);
   pthread_detach(ih_thread);
#endif
}

#ifdef WIN32
DWORD WINAPI ServerThreadEntryPoint(LPVOID threadInfo)
#else
static void  *ServerThreadEntryPoint(void *threadInfo)
#endif
{
   globals_t    *globals;
   ServerSocket *serverSocket;
   HelperSocket *helperSocket;
   sock_t        clientSocketID;
   int32_t       quitting, clientsConnected;
   string        clientAddress;
   char          socketDescription[50];
   HelperThread *helperThread;

   globals = (globals_t *) threadInfo;
   serverSocket = globals->p_ServerSocket;

   serverSocket->Open();

   while (true)
   {
      if (!serverSocket->AcceptMessage(clientSocketID, clientAddress))
         continue;

      quitting = (int32_t) globals->p_Quitting->GetValueNoLock();
      clientsConnected = (int32_t) globals->p_ClientCounter->GetValueNoLock();

      // If quitting, then break out of the loop
      if (quitting > 0)
         break;

      // If this is a client, generate a thread to handle it
      if (clientSocketID)
      {
         sprintf(socketDescription, "%d", (int32_t) clientSocketID);
         helperSocket = new HelperSocket(globals->p_Log, clientSocketID, clientAddress, socketDescription);

         if (clientsConnected >= globals->i_MaxClients)
         {
            helperSocket->Send("ERROR:  Server cannot handle more connections");
            globals->p_Log->LogMessage("Server has reached max connections of %d.  Connection from %s rejected",
                                       globals->i_MaxClients, clientAddress.c_str());
            delete helperSocket;
         }
         else
         {
            if (globals->i_ServerType == ST_WIEFERICH ||
                globals->i_ServerType == ST_WILSON ||
                globals->i_ServerType == ST_WALLSUNSUN ||
                globals->i_ServerType == ST_WOLSTENHOLME)
               helperThread = new WWWWHelperThread();
            else
               helperThread = new PrimeHelperThread();
            
            helperThread->StartThread(globals, helperSocket);
         }
      }
      else
         Sleep(500);
   }

#ifdef WIN32
   return 0;
#else
   pthread_exit(0);
#endif
}
