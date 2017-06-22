#include <signal.h>
#include <ctype.h>

#include "ServerHandler.h"
#include "PrimeWorker.h"
#include "WWWWWorker.h"

ServerHandler::ServerHandler(globals_t *globals, string serverConfiguration)
{
   char    *ptr;
   char     tempConfiguration[BUFFER_SIZE];

   ip_Log = globals->p_Log;
   ip_TestingProgramFactory = globals->p_TestingProgramFactory;

   is_EmailID = globals->s_EmailID;
   is_UserID = globals->s_UserID;
   is_MachineID = globals->s_MachineID;
   is_InstanceID = globals->s_InstanceID;
   is_TeamID = globals->s_TeamID;
   is_ServerConfiguration = serverConfiguration;

   strcpy(tempConfiguration, serverConfiguration.c_str());
   ptr = strtok(tempConfiguration, ":");

   if (ptr) { is_WorkSuffix = ptr;                 ptr = strtok(NULL, ":"); }
   if (ptr) { sscanf(ptr, "%lf", &id_WorkPercent); ptr = strtok(NULL, ":"); }
   if (ptr) { sscanf(ptr, "%d", &ii_MaxWorkUnits); ptr = strtok(NULL, ":"); }
   if (ptr) { is_ServerName = ptr;                 ptr = strtok(NULL, ":"); }

   if (ptr)
      sscanf(ptr, "%d", &ii_PortID); 
   else
   {
      printf("server configuration [%s] is invalid.  Exiting\n", tempConfiguration);
      exit(0);
   }

   if (id_WorkPercent == 0.0) ii_MaxWorkUnits = 1;
   id_TotalTime = 0.0;
   ii_CompletedWorkUnits = ii_CurrentWorkUnits = 0;
   ip_Worker = NULL;
   ii_ServerType = 0;
   is_ServerVersion = "";

   is_SaveWorkFileName = "work_" + is_WorkSuffix + ".save";
   ip_Socket = new ClientSocket(ip_Log, is_ServerName.c_str(), ii_PortID, is_WorkSuffix.c_str());

   Load();
}

ServerHandler::~ServerHandler()
{
   if (ip_Worker)
   {
      Save();

      delete ip_Worker;
   }

   delete ip_Socket;
}

bool  ServerHandler::GetWork(void)
{
   char *theGreeting = 0;

   if (ii_CurrentWorkUnits)
      return true;

   if (!ip_Socket->Open(is_EmailID, is_UserID, is_MachineID, is_InstanceID, is_TeamID))
      return false;

   // Now that the socket is open, instantiate the worker class
   ii_ServerType = ip_Socket->GetServerType();
   is_ServerVersion = ip_Socket->GetServerVersion();

   ip_Worker = AllocateWorker(ii_ServerType);

   ip_Worker->GetWork();

   ii_CurrentWorkUnits = ip_Worker->GetCurrentWorkUnits();
   ii_CompletedWorkUnits = ip_Worker->GetCompletedWorkUnits();

   theGreeting = ip_Socket->Receive(1);
   if (theGreeting && !memcmp(theGreeting, "Start Greeting", 14))
   {
      theGreeting = ip_Socket->Receive(1);

      while (theGreeting && memcmp(theGreeting, "End Greeting", 12))
      {
         printf("%s\n", theGreeting);
         theGreeting = ip_Socket->Receive(1);
      }
   }

   ip_Socket->Close();

   Save();

   return (ii_CurrentWorkUnits > 0);
}

void  ServerHandler::ReturnWork(uint32_t quitOption)
{
   ip_Log->Debug(DEBUG_WORK, "%s: Returning work.  currentworkunits=%d, completedworkunits=%d, quitOption=%d",
                 is_WorkSuffix.c_str(), ii_CurrentWorkUnits, ii_CompletedWorkUnits, quitOption);

   // If nothing to send back to the server, just return
   if (!ii_CurrentWorkUnits)
      return;

   // Return if nothing has been completed
   if (quitOption == RO_COMPLETED && !ii_CompletedWorkUnits)
      return;

   // Return if anything has not been completed
   if (quitOption == RO_IFDONE && (CanDoAnotherTest() || HaveInProgressTest()))
      return;

   if (ip_Socket->Open(is_EmailID, is_UserID, is_MachineID, is_InstanceID, is_TeamID))
   {
      ip_Worker->ReturnWork(quitOption);

      ii_CurrentWorkUnits = ip_Worker->GetCurrentWorkUnits();
      ii_CompletedWorkUnits = ip_Worker->GetCompletedWorkUnits();

      ip_Socket->Close();
   }

   Save();
}

// Load from the save file
void  ServerHandler::Load(void)
{
   FILE          *fp;
   ClientSocket  *theSocket;
   Worker        *theWorker;
   char           line[2000];
   int32_t        portID = 0, serverType = 0;
   string         serverName;
   struct stat    buf;

   // If there is not save file, then do nothing
   if (stat(is_SaveWorkFileName.c_str(), &buf) == -1)
      return;

   if ((fp = fopen(is_SaveWorkFileName.c_str(), "r")) == NULL)
   {
      printf("Could not open file [%s] for reading\n", is_SaveWorkFileName.c_str());
      exit(-1);
   }

   while (fgets(line, BUFFER_SIZE, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "ServerName=", 11))
         serverName = line+11;
      else if (!memcmp(line, "ServerType=", 11))
         serverType = atol(line+11);
      else if (!memcmp(line, "PortID=", 7))
         portID = atol(line+7);
   }

   fclose(fp);

   theWorker = AllocateWorker(serverType);
   theWorker->Load(is_SaveWorkFileName);

   if (is_ServerName == serverName && ii_PortID == portID)
      ip_Worker = theWorker;
   else
   {
      ip_Log->LogMessage("%s: The server (%s) and port (%ld) in save file do not match the input configuration.",
                         is_WorkSuffix.c_str(), serverName.c_str(), portID, is_SaveWorkFileName.c_str());
      ip_Log->LogMessage("%s: server (%s) and port (%ld).  Attempting to notify correct server of work done.",
                         is_WorkSuffix.c_str(), is_ServerName.c_str(), ii_PortID);

      theSocket = new ClientSocket(ip_Log, serverName, portID, is_WorkSuffix);

      if (theSocket->Open(is_EmailID, is_UserID, is_MachineID, is_InstanceID, is_TeamID))
      {
         theWorker->ReturnWork(RO_ALL, theSocket);

         ii_CurrentWorkUnits = ip_Worker->GetCurrentWorkUnits();
         ii_CompletedWorkUnits = ip_Worker->GetCompletedWorkUnits();

         theSocket->Close();
      }

      if (theWorker->GetCurrentWorkUnits() > 0)
      {
         ip_Log->LogMessage("   The client was unable to report the workunits to the original");
         ip_Log->LogMessage("   server.  You can fix this problem by deleting the file %s", is_SaveWorkFileName.c_str());
         ip_Log->LogMessage("   or by setting the correct server and port for this work suffix.");
         ip_Log->LogMessage("   By correcting the server and port, the client will save the workunits");
         ip_Log->LogMessage("   until it can connect to the server and report them.  To allow the");
         ip_Log->LogMessage("   client to work with other servers, add a server with a different work suffix.");
         exit(0);
      }

      delete theSocket;
      delete theWorker;
   }

   ii_CurrentWorkUnits = ip_Worker->GetCurrentWorkUnits();
   ii_CompletedWorkUnits = ip_Worker->GetCompletedWorkUnits();
}

// Save the current status
void  ServerHandler::Save(void)
{
   FILE       *fp;
   int         tryCount;

   if (!ii_CurrentWorkUnits)
   {
      unlink(is_SaveWorkFileName.c_str());
      return;
   }

   tryCount = 1;
   while ((fp = fopen(is_SaveWorkFileName.c_str(), "wt")) == NULL)
   {
      // Try up to five times to open the file before bailing
      if (tryCount < 5)
      {
         Sleep(500);
         tryCount++;
      }
      else
      {
         printf("Could not open file [%s] for writing.  Exiting program", is_SaveWorkFileName.c_str());
         exit(-1);
      }
   }

   fprintf(fp, "ServerName=%s\n", is_ServerName.c_str());
   fprintf(fp, "PortID=%d\n", ii_PortID);

   ip_Worker->Save(fp);

   fclose(fp);
}

// We don't need a factory class for this simple procedure
Worker  *ServerHandler::AllocateWorker(int32_t serverType)
{
   switch (serverType)
   {
      case ST_WIEFERICH:
      case ST_WILSON:
      case ST_WALLSUNSUN:
      case ST_WOLSTENHOLME:
         return new WWWWWorker(ip_Log, ip_TestingProgramFactory, ip_Socket, ii_MaxWorkUnits, is_WorkSuffix);

      case 0:
      case ST_SIERPINSKIRIESEL:
      case ST_CULLENWOODALL:
      case ST_FIXEDBKC:
      case ST_FIXEDBNC:
      case ST_PRIMORIAL:
      case ST_FACTORIAL:
      case ST_MULTIFACTORIAL:
      case ST_TWIN:
      case ST_GFN:
      case ST_XYYX:
      case ST_SOPHIEGERMAIN:
      case ST_GENERIC:
      case ST_CYCLOTOMIC:
      case ST_CAROLKYNEA:
      case ST_WAGSTAFF:
         return new PrimeWorker(ip_Log, ip_TestingProgramFactory, ip_Socket, ii_MaxWorkUnits, is_WorkSuffix);

      default:
         printf("Servertype [%d] is unknown.  Exiting\n", serverType);
         exit(0);
   }
}

bool  ServerHandler::ProcessWorkUnit(int32_t &specialsFound, bool inProgressOnly)
{
   double   seconds;
   bool     completed;

   completed = ip_Worker->ProcessWorkUnit(specialsFound, inProgressOnly, seconds);

   id_TotalTime += seconds;

   ii_CurrentWorkUnits = ip_Worker->GetCurrentWorkUnits();
   ii_CompletedWorkUnits = ip_Worker->GetCompletedWorkUnits();

   Save();

   return completed;
}

bool  ServerHandler::CanDoAnotherTest(void)
{
   if (ip_Worker)
      return ip_Worker->CanDoAnotherTest();

   return false;
}

bool  ServerHandler::HaveInProgressTest(void)
{
   if (ip_Worker)
      return ip_Worker->HaveInProgressTest();

   return false;
}

