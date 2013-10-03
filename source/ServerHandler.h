#ifndef _ServerHandler_

#define _ServerHandler_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "client.h"
#include "ClientSocket.h"
#include "TestingProgramFactory.h"
#include "Worker.h"

class ServerHandler
{
public:
   ServerHandler(globals_t *globals, string serverConfiguration);

   ~ServerHandler();

   string   GetWorkSuffix() { return is_WorkSuffix; };

   bool     CanDoAnotherTest(void);

   bool     HaveInProgressTest(void);

   bool     GetWork(void);

   void     ReturnWork(uint32_t quitOption);

   void     IncreaseTotalTime(double totalTime) { id_TotalTime += totalTime; };

   double   GetTotalTime(void) { return id_TotalTime; };

   double   GetWorkPercent(void) { return id_WorkPercent; };
   void     SetWorkPercent(double workPercent) { id_WorkPercent = workPercent; };

   int32_t  GetCurrentWorkUnits(void) { return ii_CurrentWorkUnits; };

   string   GetServerConfiguation(void)   {return is_ServerConfiguration; };

   bool     ProcessWorkUnit(int32_t &specialsFound, bool inProgressOnly);

protected:
   ClientSocket *ip_Socket;
   TestingProgramFactory *ip_TestingProgramFactory;
   Log     *ip_Log;
   Worker  *ip_Worker;

   string   is_EmailID;
   string   is_UserID;
   string   is_MachineID;
   string   is_InstanceID;
   string   is_TeamID;

   string   is_WorkSuffix;
   string   is_ServerName;
   int32_t  ii_PortID;
   string   is_ServerVersion;
   string   is_SaveWorkFileName;
   int32_t  ii_ServerType;
   string   is_ServerConfiguration;

   int32_t  ii_MaxWorkUnits;
   double   id_WorkPercent;
   double   id_TotalTime;
   int32_t  ii_CurrentWorkUnits;
   int32_t  ii_CompletedWorkUnits;

   bool     GetWork(Socket *theSocket);
   void     ReturnWork(ClientSocket *theSocket, uint32_t returnOption);
   Worker  *AllocateWorker(int32_t serverType);

   void     Load(void);
   void     Save(void);
};

#endif // #ifndef _ServerHandler_

