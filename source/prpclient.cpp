// prpclient.cpp
//
// Mark Rodenkirch (rogue@wi.rr.com)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include "defs.h"

#ifdef WIN32
   #include <io.h>
   #include <direct.h>
#endif

#ifdef UNIX
   #include <signal.h>
#endif

#include "Log.h"
#include "Socket.h"
#include "ServerHandler.h"
#include "TestingProgramFactory.h"
#include "client.h"

#define  RC_NOWORK   999999999

#define  SO_PROMPT                           0     // prompt for value
#define  SO_REPORT_ABORT_WORK                1     // report, abandon, get more work
#define  SO_REPORT_ABORT_QUIT                2     // report, abandon, shut down
#define  SO_REPORT_COMPLETED_CONTINUE        3     // report completed, then continue
#define  SO_COMPLETE_INPROGRESS_ABORT_WORK   4     // complete inprogress, abandon, get more work
#define  SO_COMPLETE_INPROGRESS_ABORT_QUIT   5     // complete inprogress, abandon, shut down
#define  SO_COMPLETE_ALL_REPORT_QUIT         6     // complete all, report, shut down
#define  SO_COMPLETE_ALL_QUIT                7     // complete all, shut down
#define  SO_CONTINUE                         9     // continue

globals_t   *gp_Globals;

ServerHandler **gp_ServerHandler;
int32_t         gi_ServerHandlerCount;

time_t   gt_StartTime;
bool     gb_IsQuitting;
int32_t  gi_ErrorTimeout;
double   gd_TotalTime;
int32_t  gi_TotalWorkUnits;
int32_t  gi_TotalSpecials;
bool     gb_IsRunningTest;
int32_t  gi_StartOption;
int32_t  gi_StopOption;
int32_t  gi_StopASAPOption;

#ifdef WIN32
#if defined (_MSC_VER) && defined (MEMLEAK)
   _CrtMemState mem_dbg1;
   HANDLE hLogFile;
#endif
   BOOL WINAPI HandlerRoutine(DWORD /*dwCtrlType*/);
   #define LIST_COMMAND      "dir /B"
   #define FILE_DELIMITER    '\\'
#else
   #define LIST_COMMAND      "ls"
   #define FILE_DELIMITER    '/'
#endif

// procedure declarations
uint32_t FindNextServerForWork(void);
uint32_t FindAnyAvailableServerForWork(void);
uint32_t FindAnyIncompleteWork(void);
bool     DoWork(uint32_t serverIdx, bool inProgressOnly);
void     ProcessINIFile(string configFile);
void     ReprocessINIFile(string configFile);
void     ProcessCommandLine(int count, char *arg[]);
void     ValidateConfiguration(void);
void     AllocateWorkClasses(string configFile);
void     SetQuitting(int sig);
void     RemoveTempFiles(string filter);
bool     CheckForIncompleteWork(void);
int32_t  PromptForStartOption(ServerHandler *serverHandler);
void     CleanUpAndExit(void);
void     TerminateWithError(string fmt, ...);

// Since we could not connect to our first choice for work, find any
// server that we can connect to
uint32_t FindAnyIncompleteWork()
{
   int32_t  serverIdx;
   bool     haveInProgressTest, canDoAnotherTest;
   ServerHandler *serverHandler;

   gp_Globals->p_Log->Debug(DEBUG_WORK, "in FindAnyIncompleteWork");

   // If we couldn't connect to any servers then look for any PRP tests
   // that have not been returned to their respective server
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
   {
      serverHandler = gp_ServerHandler[serverIdx];

      haveInProgressTest = serverHandler->HaveInProgressTest(); 
      canDoAnotherTest = serverHandler->CanDoAnotherTest();

      gp_Globals->p_Log->Debug(DEBUG_WORK, "Work: %s, HaveInProgressTest = %s, CanDoAnotherTest = %s",
                               serverHandler->GetWorkSuffix().c_str(),
                               (haveInProgressTest ? "true" : "false"),
                               (canDoAnotherTest ? "true" : "false"));

      if (haveInProgressTest || canDoAnotherTest)
         return serverIdx;
   }

   return RC_NOWORK;
}

// Find the next server that we want to get work from based upon
// the percentage of work to be done for that server
uint32_t FindNextServerForWork(void)
{
   int32_t  serverIdx;
   ServerHandler *serverHandler;

   gp_Globals->p_Log->Debug(DEBUG_WORK, "in FindNextServerForWork: total time for client=%.0lf seconds", gd_TotalTime);

   // If we find a server that needs work done, but hasn't had any, then get work for it.
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
   {
      serverHandler = gp_ServerHandler[serverIdx];

      if (gd_TotalTime == 0. && serverHandler->GetWorkPercent() > 0.)
      {
         gp_Globals->p_Log->Debug(DEBUG_WORK, "Work: %s, no work done yet, target pct work done=%.0lf",
                      serverHandler->GetWorkSuffix().c_str(), serverHandler->GetWorkPercent());

         if (serverHandler->GetWork())
            return serverIdx;
      }
   }

   // Now try to balance work across all servers
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
   {
      serverHandler = gp_ServerHandler[serverIdx];

      if (serverHandler->GetCurrentWorkUnits() > 0)
      {
         gp_Globals->p_Log->Debug(DEBUG_WORK, "Work: %s, skipping due to all work done",
                                  serverHandler->GetWorkSuffix().c_str());
         continue;
      }

      if (serverHandler->GetWorkPercent() <= 0.)
      {
         gp_Globals->p_Log->Debug(DEBUG_WORK, "Work: %s, skipping due to configured work percent of 0",
                                  serverHandler->GetWorkSuffix().c_str());
         continue;
      }

      gp_Globals->p_Log->Debug(DEBUG_WORK, "Work: %s, time=%.0lf, pct time=%0lf, work pct=%.0lf",
                   serverHandler->GetWorkSuffix().c_str(), serverHandler->GetTotalTime(),
                   (serverHandler->GetTotalTime() * 100.)/gd_TotalTime, serverHandler->GetWorkPercent());

      if ((serverHandler->GetTotalTime() * 100.)/gd_TotalTime > serverHandler->GetWorkPercent())
         continue;

      if (serverHandler->GetWork())
         return serverIdx;
   }

   return RC_NOWORK;
}

// Since we could not connect to our first choice for work, find any
// server that we can connect to
uint32_t FindAnyAvailableServerForWork()
{
   int32_t  serverIdx;
   ServerHandler *serverHandler;

   gp_Globals->p_Log->Debug(DEBUG_WORK, "in FindAnyAvailableServerForWork");

   // Start with servers that we want work from (work percent > 0)
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
   {
      serverHandler = gp_ServerHandler[serverIdx];

      gp_Globals->p_Log->Debug(DEBUG_WORK, "First loop: Work: %s, CurrentWorkUnits = %d, GetWorkPercent = %.0lf",
                   serverHandler->GetWorkSuffix().c_str(), serverHandler->GetCurrentWorkUnits(),
                   serverHandler->GetWorkPercent());

      // Only select if there is no work
      if (serverHandler->GetCurrentWorkUnits() > 0) continue;

      if (serverHandler->GetWorkPercent() <= 0.) continue;

      if (serverHandler->GetWork())
         return serverIdx;
   }

   // Then try servers with work percent = 0
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
   {
      serverHandler = gp_ServerHandler[serverIdx];

      gp_Globals->p_Log->Debug(DEBUG_WORK, "Second loop: Work: %s, CurrentWorkUnits = %d, GetWorkPercent = %.0lf",
                   serverHandler->GetWorkSuffix().c_str(), serverHandler->GetCurrentWorkUnits(),
                   serverHandler->GetWorkPercent());

      // Only select if there is no work
      if (serverHandler->GetCurrentWorkUnits() > 0) continue;

      if (serverHandler->GetWorkPercent() > 0.) continue;

      if (serverHandler->GetWork())
         return serverIdx;
   }

   return RC_NOWORK;
}

// Take the given work and do it
bool     DoWork(int32_t serverIdx, bool inProgressOnly)
{
   bool     didTest = false;
   int32_t  specialsFound;
   uint32_t elapsed, timeh, timem, times;
   double   timeStarted, timeDone;

   while (true)
   {
      // If quitting and don't want to complete work units, then break out of loop
      if (gb_IsQuitting &&
          gi_StopOption != SO_COMPLETE_ALL_REPORT_QUIT &&
          gi_StopOption != SO_COMPLETE_ALL_QUIT &&
          gi_StopOption != SO_COMPLETE_INPROGRESS_ABORT_QUIT)
         break;

      if (gb_IsQuitting && gi_StopOption == SO_COMPLETE_INPROGRESS_ABORT_QUIT)
         inProgressOnly = true;

         // If there are no more PRP tests to perform, then break out of loop
      if (!gp_ServerHandler[serverIdx]->CanDoAnotherTest())
         break;

      timeStarted = (double) time(NULL);

      gb_IsRunningTest = true;
      didTest = gp_ServerHandler[serverIdx]->ProcessWorkUnit(specialsFound, inProgressOnly);
      gb_IsRunningTest = false;

      timeDone = (double) time(NULL);

      // If no test was done, that means that the process was interrupted
      if (!didTest)
      {
         if (!gb_IsQuitting)
            TerminateWithError("Huh?  The test did not complete, yet you didn't terminate it.");
      }
      else
         gi_TotalWorkUnits++;

      gi_TotalSpecials += specialsFound;

      gd_TotalTime += (timeDone - timeStarted);

      // Read for any updates
      ReprocessINIFile("prpclient.ini");

      // If stopping ASAP and don't want to do any more tests, then break out of loop
      if (gi_StopASAPOption == SO_REPORT_ABORT_QUIT || gi_StopASAPOption == SO_REPORT_COMPLETED_CONTINUE)
         break;

      // If only doing inprogress tests, then break out of loop
      if (inProgressOnly)
         break;
   }

   elapsed = (int32_t) difftime(time(NULL), gt_StartTime);
   timeh = elapsed/3600;
   timem = (elapsed - timeh*3600) / 60;
   times = elapsed - timeh*3600 - timem*60;
   gp_Globals->p_Log->LogMessage("Total Time:%3d:%02d:%02d  Total Work Units: %d  Special Results Found: %d",
                      timeh, timem, times, gi_TotalWorkUnits, gi_TotalSpecials);

   return didTest;
}

#ifdef WIN32
BOOL WINAPI HandlerRoutine(DWORD /*dwCtrlType*/)
{
   if (!gb_IsRunningTest)
   {
      SetQuitting(1);
      return FALSE;
   }

   return TRUE;
}
#endif

void SetQuitting(int sig)
{
   int   theChar = 0;
   int   serverIdx, workToDo;

   if (gb_IsQuitting)
      TerminateWithError("CTRL-C already hit, so I'm going to terminate now.");

   ReprocessINIFile("prpclient.ini");

#ifdef WIN32
   if (sig == 1 && gi_StopOption == SO_PROMPT)
#else
   if (sig == SIGINT && gi_StopOption == SO_PROMPT)
#endif
   {
      workToDo = 0;
      for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
         workToDo += gp_ServerHandler[serverIdx]->GetCurrentWorkUnits();

      if (workToDo)
      {
         gi_StopOption = SO_PROMPT;

         fprintf(stderr, "You have requested the client to terminate.  What should it do with current work units?\n");
         fprintf(stderr, "  2 = Return completed work units and abandon the rest\n");
         fprintf(stderr, "  3 = Return completed work units and keep the rest\n");
         fprintf(stderr, "  5 = Complete inprogress work units, abandon the rest, then return them\n");
         fprintf(stderr, "  6 = Complete all work units and return them\n");
         fprintf(stderr, "  7 = Complete all work units\n");
         fprintf(stderr, "  9 = Do nothing and shut down");

         while (gi_StopOption != SO_REPORT_ABORT_QUIT &&
                gi_StopOption != SO_REPORT_COMPLETED_CONTINUE &&
                gi_StopOption != SO_COMPLETE_INPROGRESS_ABORT_QUIT &&
                gi_StopOption != SO_COMPLETE_ALL_REPORT_QUIT &&
                gi_StopOption != SO_COMPLETE_ALL_QUIT &&
                gi_StopOption != SO_CONTINUE)
         {
            if (theChar != '\n' && theChar != '\r')
               fprintf(stderr, "\nChoose option: ");
            theChar = getchar();

            if (isdigit(theChar))
               gi_StopOption = theChar - '0';
         }
      }
   }

   if (gi_StopOption == SO_COMPLETE_INPROGRESS_ABORT_QUIT)
      gp_Globals->p_Log->LogMessage("Client will shut down after completing inprogress work units");

   if (gi_StopOption == SO_COMPLETE_ALL_REPORT_QUIT ||
       gi_StopOption == SO_COMPLETE_ALL_QUIT)
      gp_Globals->p_Log->LogMessage("Client will shut down after completing all work units");

   gi_StopASAPOption = SO_PROMPT;
   gb_IsQuitting = true;
}

int main(int argc, char *argv[])
{
   int32_t  serverIdx;
   int32_t  workToDo;
   int32_t  returnOption;
   int32_t  seconds;

#if defined (_MSC_VER) && defined (MEMLEAK)
   hLogFile = CreateFile("prpclient_memleak.txt", GENERIC_WRITE,
      FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);

   _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE);
   _CrtSetReportFile( _CRT_WARN, hLogFile);
   _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_ERROR, hLogFile );
   _CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
   _CrtSetReportFile( _CRT_ASSERT, hLogFile );

   // Store a memory checkpoint in the s1 memory-state structure
   _CrtMemCheckpoint( &mem_dbg1 );

   // Get the current state of the flag
   int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
   // Turn Off (AND) - DO NOT Enable debug heap allocations and use of memory block type identifiers such as _CLIENT_BLOCK
   //tmpFlag &= ~_CRTDBG_ALLOC_MEM_DF;
   tmpFlag |= _CRTDBG_ALLOC_MEM_DF;

   // Turn Off (AND) - prevent _CrtCheckMemory from being called at every allocation request.  _CurCheckMemory must be called explicitly
   tmpFlag &= ~_CRTDBG_CHECK_ALWAYS_DF;
   //tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;

   // Turn Off (AND) - Do NOT include __CRT_BLOCK types in leak detection and memory state differences
   tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;
   //tmpFlag |= _CRTDBG_CHECK_CRT_DF;

   // Turn Off (AND) - DO NOT Keep freed memory blocks in the heap’s linked list and mark them as freed
   tmpFlag &= ~_CRTDBG_DELAY_FREE_MEM_DF;

   // Turn Off (AND) - Do NOT perform leak check at end of program run.
   //tmpFlag &= ~_CRTDBG_LEAK_CHECK_DF;
   tmpFlag |= _CRTDBG_LEAK_CHECK_DF;

   // Set the new state for the flag
   _CrtSetDbgFlag( tmpFlag );
#endif

   gp_Globals = new globals_t;

   time(&gt_StartTime);

   // Set default values for the global variables
   gp_Globals->s_EmailID = "";
   gp_Globals->s_UserID = "";
   gp_Globals->s_MachineID = "";
   gp_Globals->s_InstanceID = "";
   gp_Globals->s_TeamID = "";

   gi_ServerHandlerCount = 0;
   gi_ErrorTimeout = 20;
   gi_TotalWorkUnits = 0;
   gi_TotalSpecials = 0;
   gb_IsRunningTest = false;
   gi_StartOption = SO_PROMPT;
   gi_StopOption = SO_PROMPT;
   gi_StopASAPOption = SO_PROMPT;

   gp_Globals->p_Log = new Log(0, "prpclient.log", DEBUG_NONE, true);

    // Get default values from the configuration file
   ProcessINIFile("prpclient.ini");

   ProcessCommandLine(argc, argv);

   ValidateConfiguration();

#ifdef WIN32
   char     processName[200];

   WSADATA wsaData;
   WSAStartup(MAKEWORD(1,1), &wsaData);

   SetConsoleCtrlHandler(HandlerRoutine, TRUE);

   // prevent multiple copies from running
   sprintf(processName, "PRPNetClient_Id%s", gp_Globals->s_InstanceID.c_str());
   void *mutex = CreateMutex(NULL, FALSE, processName);
   if (!mutex || ERROR_ALREADY_EXISTS == GetLastError())
      TerminateWithError("Unable to run multiple instances of the PRPNet client with id %s.", gp_Globals->s_InstanceID.c_str());
#endif

#ifdef UNIX
   // Ignore SIGHUP, as to not die on logout.
   // We log to file in most cases anyway.
   signal(SIGHUP, SIG_IGN);
   signal(SIGINT, SetQuitting);
   signal(SIGQUIT, SetQuitting);
   signal(SIGTERM, SetQuitting);
   signal(SIGPIPE, SIG_IGN);

   int lockFile;
   lockFile = open("client.lock", O_WRONLY | O_CREAT | O_EXCL, 0600);
   if (lockFile <= 0)
      TerminateWithError("Only one copy of the PRPNet client can be run from this folder at a time.");
#endif

   AllocateWorkClasses("prpclient.ini");

   // Do not allow these values to be zero
   if (gi_ErrorTimeout < 1)
      gi_ErrorTimeout = 1;

   gp_Globals->p_Log->LogMessage("PRPNet Client application v%s started", PRPNET_VERSION);
   gp_Globals->p_Log->LogMessage("User name %s at email address is %s",
                                 gp_Globals->s_UserID.c_str(), gp_Globals->s_EmailID.c_str());

   if (CheckForIncompleteWork())
   {
#ifdef WIN32
      CloseHandle(mutex);
#else
      close(lockFile);
      unlink("client.lock");
#endif

      CleanUpAndExit();
   }

   // If all work completed, return here otherwise client will sleep right away
   // if there is no work to be done.
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
      gp_ServerHandler[serverIdx]->ReturnWork(RO_IFDONE);

   // The main loop...
   // 1. Find some work to do (might connect to a server to get it)
   // 2. Do the work
   // 3. Return the work details

   while (!gb_IsQuitting)
   {
      // Finish any PRP test that we have started
      serverIdx = FindAnyIncompleteWork();

      // Find a server based upon how it is divied up amongst servers
      if (serverIdx == RC_NOWORK && !gb_IsQuitting)
         serverIdx = FindNextServerForWork();

      // If we could not get work from the desired server, find any available server
      // that we want to get work from
      if (serverIdx == RC_NOWORK && !gb_IsQuitting)
         serverIdx = FindAnyAvailableServerForWork();

      // If we cannot connect to any servers, then find any work that has not been
      // completed, i.e., a factor has not been found and  doing P+1 or PRP factoring
      if (serverIdx == RC_NOWORK && !gb_IsQuitting)
      {
         gp_Globals->p_Log->LogMessage("Could not connect to any servers and no work is pending.  Pausing %d minute%s",
                            gi_ErrorTimeout, ((gi_ErrorTimeout == 1) ? "" : "s"));

         for (seconds=0; seconds<=gi_ErrorTimeout*60; seconds++)
         {
            if (gb_IsQuitting)
                break;

            Sleep(1000);
         }
      }
      else
         DoWork(serverIdx, false);

      // If we need to stop ASAP, then do so now
      if (gb_IsQuitting || gi_StopASAPOption != SO_PROMPT)
          break;

      workToDo = 0;
      // Return all completed work.
      for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
      {
         gp_ServerHandler[serverIdx]->ReturnWork(RO_ALL);

         // If we have nothing left to report, then re-allocate the work classes
         workToDo += gp_ServerHandler[serverIdx]->GetCurrentWorkUnits();
      }

      // If everything has been reported, then see if they have changed work allocation
      if (workToDo == 0)
         AllocateWorkClasses("prpclient.ini");
   }

   // Now that we are out of the loop, what do we need to do with any existing workunits
   returnOption = RO_UNDEFINED;
   if (gi_StopOption == SO_REPORT_COMPLETED_CONTINUE)
      returnOption = RO_COMPLETED;
   else if (gi_StopOption == SO_REPORT_ABORT_QUIT ||
            gi_StopOption == SO_COMPLETE_INPROGRESS_ABORT_QUIT ||
            gi_StopOption == SO_COMPLETE_ALL_REPORT_QUIT)
      returnOption = RO_ALL;

   if (gi_StopASAPOption == SO_REPORT_COMPLETED_CONTINUE)
      returnOption = RO_COMPLETED;
   else if (gi_StopASAPOption == SO_REPORT_ABORT_QUIT ||
            gi_StopASAPOption == SO_COMPLETE_ALL_REPORT_QUIT)
      returnOption = RO_ALL;

   if (returnOption != RO_UNDEFINED)
      for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
         gp_ServerHandler[serverIdx]->ReturnWork(returnOption);

   workToDo = 0;
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
      workToDo += gp_ServerHandler[serverIdx]->GetCurrentWorkUnits();

   // Remove all temp files since there is nothing left to do
   if (!workToDo)
   {
      RemoveTempFiles("z*");
      RemoveTempFiles("*.pfr");
      RemoveTempFiles("phrot.ckpt");
      RemoveTempFiles("genefer.ckpt");
      RemoveTempFiles("cyclo.ckpt");
      RemoveTempFiles("work_*");
      RemoveTempFiles("llr.ini");
      RemoveTempFiles("phrot.ini");
      RemoveTempFiles("pfgw.ini");
      RemoveTempFiles("wwww.ckpt");
   }

#ifdef WIN32
   CloseHandle(mutex);
#else
   close(lockFile);
   unlink("client.lock");
#endif

   CleanUpAndExit();
}

// Parse the configuration file to get the default values
void  ProcessINIFile(string configFile)
{
   FILE   *fp;
   char    line[2001];

   if ((fp = fopen(configFile.c_str(), "r")) == NULL)
      TerminateWithError("Configuration file '%s' could not be opened for reading.", configFile.c_str());

   // Extract the email ID and count the number of servers on this pass
   while (fgets(line, 2000, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "userid=", 7))
         gp_Globals->s_UserID = line+7;
      else if (!memcmp(line, "email=", 6))
         gp_Globals->s_EmailID =  line+6;
      else if (!memcmp(line, "machineid=", 10))
         gp_Globals->s_MachineID = line+10;
      else if (!memcmp(line, "instanceid=", 11))
         gp_Globals->s_InstanceID = line+11;
      else if (!memcmp(line, "teamid=", 7))
         gp_Globals->s_TeamID = line+7;
      else if (!memcmp(line, "loglimit=", 9))
         gp_Globals->p_Log->SetLogLimit(atol(line+9) * 1024 * 1024);
      else if (!memcmp(line, "debuglevel=", 11))
         gp_Globals->p_Log->SetDebugLevel(atol(line+11));
      else if (!memcmp(line, "echotest=", 9))
         gp_Globals->p_Log->SetEchoTest((atol(line+9)==1 ? true : false));
      else if (!memcmp(line, "errortimeout=", 13))
         gi_ErrorTimeout = atol(line+13);
      else if (!memcmp(line, "startoption=", 12))
         gi_StartOption = atol(line+12);
      else if (!memcmp(line, "stopoption=", 11))
         gi_StopOption = atol(line+11);
      else if (!memcmp(line, "stopasapoption=", 15))
         gi_StopASAPOption = atol(line+15);
   }

   fclose(fp);

   gp_Globals->p_TestingProgramFactory = new TestingProgramFactory(gp_Globals->p_Log, configFile);

   if (!gp_Globals->p_TestingProgramFactory->ValidatePrograms())
   {
      Sleep(10000);
      exit(-1);
   }
}

// Parse the command line to get any overrides of the default values
void  ProcessCommandLine(int count, char *arg[])
{
   int   i = 1;

   while (i < count)
   {
      if (!strcmp(arg[i], "-debug"))
         gp_Globals->p_Log->SetDebugLevel(DEBUG_ALL);

      i++;
   }
}

void  ReprocessINIFile(string configFile)
{
   FILE    *fp;
   char     line[2001];
   int32_t  stopOption = 9;
   int32_t  stopASAPOption = 9;

   if (gb_IsQuitting) return;

   if ((fp = fopen(configFile.c_str(), "r")) == NULL) return;

   // Extract the email ID and count the number of servers on this pass
   while (fgets(line, 2000, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "debuglevel=", 11))
         gp_Globals->p_Log->SetDebugLevel(atol(line+11));
      else if (!memcmp(line, "stopoption=", 11))
         stopOption = atol(line+11);
      else if (!memcmp(line, "stopasapoption=", 15))
         stopASAPOption = atol(line+15);
   }

   fclose(fp);

   if (stopOption != SO_PROMPT && 
       stopOption != SO_REPORT_ABORT_QUIT &&
       stopOption != SO_REPORT_COMPLETED_CONTINUE &&
       stopOption != SO_COMPLETE_INPROGRESS_ABORT_QUIT &&
       stopOption != SO_COMPLETE_ALL_REPORT_QUIT &&
       stopOption != SO_COMPLETE_ALL_QUIT &&
       stopOption != SO_CONTINUE)
   {
      printf("stopoption was updated and is now invalid.  Please fix for it to take affect\n");
   }
   else
      gi_StopOption = stopOption;

   if (stopASAPOption != SO_PROMPT && 
       stopASAPOption != SO_REPORT_ABORT_QUIT &&
       stopASAPOption != SO_REPORT_COMPLETED_CONTINUE &&
       stopASAPOption != SO_COMPLETE_ALL_REPORT_QUIT &&
       stopASAPOption != SO_COMPLETE_ALL_QUIT)
   {
      printf("stopasapoption was updated and is now invalid.  Please fix for it to take affect\n");
   }
   else
      gi_StopASAPOption = stopASAPOption;

   // Trigger shutdown after completing work
   if (gi_StopASAPOption == SO_PROMPT && 
       ( gi_StartOption == SO_COMPLETE_ALL_REPORT_QUIT ||
         gi_StartOption == SO_COMPLETE_ALL_QUIT ))
      gi_StopASAPOption = gi_StartOption;
}

void   ValidateConfiguration()
{
   if (!gp_Globals->s_EmailID.length())
      TerminateWithError("An email address must be configured.");

   if (gp_Globals->s_EmailID.find("@") == string::npos ||
       gp_Globals->s_EmailID.find(":") != string::npos ||
       gp_Globals->s_EmailID.find(" ") != string::npos)
      TerminateWithError("Email address supplied (%s) is not valid as it has an embedded ':' or ' '.",
                         gp_Globals->s_EmailID.c_str());

   if (!gp_Globals->s_UserID.length())
   {
      printf("No user ID specified, will use e-mail ID\n");
      gp_Globals->s_UserID = gp_Globals->s_EmailID;
   }

   if (!gp_Globals->s_MachineID.length())
      TerminateWithError("No machine ID supplied.");

   if (!gp_Globals->s_InstanceID.length())
   {
      printf("No instance ID specified, will use machine ID\n");
      gp_Globals->s_InstanceID = gp_Globals->s_MachineID;
   }

   if (!gp_Globals->s_TeamID.length())
      gp_Globals->s_TeamID = NO_TEAM;

   if (gp_Globals->s_MachineID.find("@") != string::npos ||
       gp_Globals->s_MachineID.find(":") != string::npos ||
       gp_Globals->s_MachineID.find(" ") != string::npos)
      TerminateWithError("Machine ID supplied (%s) is not valid as it has an embedded '@', ':' or ' '.", 
                         gp_Globals->s_MachineID.c_str());

   if (gp_Globals->s_InstanceID.find("@") != string::npos ||
       gp_Globals->s_InstanceID.find(":") != string::npos ||
       gp_Globals->s_InstanceID.find(" ") != string::npos)
      TerminateWithError("Instance ID supplied (%s) is not valid as it has an embedded '@', ':' or ' '.", 
                         gp_Globals->s_InstanceID.c_str());

   if (gp_Globals->s_TeamID.find(" ") != string::npos)
      TerminateWithError("Team ID supplied (%s) is not valid as it has an embedded space.", 
                         gp_Globals->s_TeamID.c_str());

   if (gi_StartOption != SO_PROMPT && gi_StartOption != SO_CONTINUE && 
       gi_StartOption != SO_REPORT_ABORT_WORK &&
       gi_StartOption != SO_REPORT_ABORT_QUIT  &&
       gi_StartOption != SO_REPORT_COMPLETED_CONTINUE  &&
       gi_StartOption != SO_COMPLETE_INPROGRESS_ABORT_WORK &&
       gi_StartOption != SO_COMPLETE_INPROGRESS_ABORT_QUIT &&
       gi_StartOption != SO_COMPLETE_ALL_REPORT_QUIT &&
       gi_StartOption != SO_COMPLETE_ALL_QUIT)
   {
      gi_StartOption = SO_PROMPT;
      printf("startoption is invalid, setting it to 0 (prompt).\n");
   }

   if (gi_StopOption != SO_PROMPT && 
       gi_StopOption != SO_REPORT_ABORT_QUIT  &&
       gi_StopOption != SO_REPORT_COMPLETED_CONTINUE  &&
       gi_StopOption != SO_COMPLETE_INPROGRESS_ABORT_QUIT &&
       gi_StopOption != SO_COMPLETE_ALL_REPORT_QUIT &&
       gi_StopOption != SO_COMPLETE_ALL_QUIT &&
       gi_StopOption != SO_CONTINUE)
   {
      gi_StopOption = SO_PROMPT;
      printf("stopoption is invalid, setting it to 0 (prompt).\n");
   }

   if (gi_StopASAPOption != SO_PROMPT)
      TerminateWithError("stopasapoption is invalid.  Reset to 0, then restart the client.");

   // Trigger shutdown after completing work
   if (gi_StartOption == SO_COMPLETE_ALL_REPORT_QUIT || gi_StartOption == SO_COMPLETE_ALL_QUIT)
      gi_StopASAPOption = gi_StartOption;

   if (gi_StopASAPOption == SO_COMPLETE_ALL_REPORT_QUIT || gi_StopASAPOption == SO_COMPLETE_ALL_QUIT)
      gp_Globals->p_Log->LogMessage("Client will shut down after completing all work units");
}

void  AllocateWorkClasses(string configFile)
{
   FILE    *fp;
   char     line[2001];
   int32_t  serverIdx;
   bool     changed;
   double   totalPercent = 0.;
   double   workPercent;
   ServerHandler *serverHandler;

   if ((fp = fopen(configFile.c_str(), "r")) == NULL)
      TerminateWithError("Configuration file '%s' could not be opened for reading.", configFile.c_str());

   changed = false;
   serverIdx = 0;
   while (fgets(line, 2000, fp) != NULL)
   {
      if (!memcmp(line, "server=", 7))
      {
         StripCRLF(line);

         if (serverIdx >= gi_ServerHandlerCount)
            changed = true;
         else if (gp_ServerHandler[serverIdx]->GetServerConfiguation() != line+7)
            changed = true;
         serverIdx++;
      }
   }

   fclose(fp);

   // Since this is called only when there is no work to be returned to a server
   // we can exit if no servers are configured, even if the user changed the
   // configuration while the client is running.
   if (serverIdx == 0)
      TerminateWithError("No servers configured.");

   if (serverIdx == gi_ServerHandlerCount && !changed)
      return;

   while (gi_ServerHandlerCount)
   {
      gi_ServerHandlerCount--;
      delete gp_ServerHandler[gi_ServerHandlerCount];

      if (gi_ServerHandlerCount == 0)
         gp_Globals->p_Log->LogMessage("Due to changes in the prpclient.ini file, work classes have been reloaded");
   }

   gi_ServerHandlerCount = serverIdx;

   fp = fopen(configFile.c_str(), "r");

   gp_ServerHandler = new ServerHandler *[gi_ServerHandlerCount];

   serverIdx = 0;
   while (fgets(line, 2000, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "server=", 7))
      {
         gp_ServerHandler[serverIdx] = new ServerHandler(gp_Globals, line+7);
         totalPercent += gp_ServerHandler[serverIdx]->GetWorkPercent();
         serverIdx++;
      }
   }

   if (totalPercent != 100.)
   {
      printf("Total percent of servers does not equal 100.  Normalizing\n");

      for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
      {
         serverHandler = gp_ServerHandler[serverIdx];
         if (totalPercent == 0.0)
            serverHandler->SetWorkPercent(100.0 / gi_ServerHandlerCount);
         else
         {
            workPercent = serverHandler->GetWorkPercent();
            workPercent = (100.0 * workPercent) / totalPercent;
            serverHandler->SetWorkPercent(workPercent);
         }
      }
   }
}

void  RemoveTempFiles(string filter)
{
   char     command[200], fileName[500];
   int32_t  rc;
   FILE    *fPtr;

   sprintf(command, "%s %s > file_list.out", LIST_COMMAND, filter.c_str());

   system(command);

   fPtr = fopen("file_list.out", "r");

   if (!fPtr)
      return;

   while (fgets(fileName, sizeof(fileName), fPtr) != NULL)
   {
      StripCRLF(fileName);

      // Strip off the directory portion of the file name
      for (rc=strlen(fileName); rc>0; rc--)
        if (fileName[rc] == FILE_DELIMITER)
        {
           fileName[rc] = 0;
           strcpy(fileName, &fileName[rc+1]);
           rc = 0;
        }

      unlink(fileName);
   }

   fclose(fPtr);
   unlink("file_list.out");
}

bool  CheckForIncompleteWork(void)
{
   int32_t  serverIdx;
   int32_t  startOption = gi_StartOption;
   bool     shutDown;

   shutDown = false;
   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
   {
      if (gp_ServerHandler[serverIdx]->HaveInProgressTest() || gp_ServerHandler[serverIdx]->CanDoAnotherTest())
      {
         // This allows each one with remaining work to be prompted separately, but
         // if any say "shut down", then the client will exit when done.
         if (gi_StartOption == SO_PROMPT)
            startOption = PromptForStartOption(gp_ServerHandler[serverIdx]);

         switch (startOption)
         {
            // If they don't want to do anything, then return it all.
            case SO_REPORT_ABORT_WORK:
               gp_ServerHandler[serverIdx]->ReturnWork(RO_ALL);
               break;

            case SO_REPORT_ABORT_QUIT:
               gp_ServerHandler[serverIdx]->ReturnWork(RO_ALL);
               shutDown = true;
               break;

            case SO_REPORT_COMPLETED_CONTINUE:
               gp_ServerHandler[serverIdx]->ReturnWork(RO_COMPLETED);
               break;

            // Try to cmplete the inprogess test.  If we are successful, then
            // we will return everything to the server.
            case SO_COMPLETE_INPROGRESS_ABORT_WORK:
               if (DoWork(serverIdx, true))
                  gp_ServerHandler[serverIdx]->ReturnWork(RO_ALL);
               break;

            case SO_COMPLETE_INPROGRESS_ABORT_QUIT:
               if (DoWork(serverIdx, true))
                  gp_ServerHandler[serverIdx]->ReturnWork(RO_ALL);
               shutDown = true;
               break;

            // Treat this as if we have started, but want to stop ASAP after all
            // tests have been completed.
            case SO_COMPLETE_ALL_REPORT_QUIT:
            case SO_COMPLETE_ALL_QUIT:
               gi_StartOption = startOption;
               gi_StopASAPOption = startOption;
               break;
         }
      }
   }

   return shutDown;
}

int32_t  PromptForStartOption(ServerHandler *serverHandler)
{
   int32_t  option = SO_PROMPT;
   char     theChar = ' ';

   fprintf(stderr, "It appears that the PRPNet client was aborted without completing\n");
   fprintf(stderr, "the workunits asigned by server %s.  What do you want to do with them?\n", serverHandler->GetWorkSuffix().c_str());
   fprintf(stderr, "  1 = Report completed and abort the rest, then get more work\n");
   fprintf(stderr, "  2 = Report completed and abort the rest, then shut down\n");
   fprintf(stderr, "  3 = Return completed, then continue\n");
   fprintf(stderr, "  4 = Complete in-progress, abort the rest, report them, then get more work\n");
   fprintf(stderr, "  5 = Complete in-progress, abort the rest, report them, then shut down\n");
   fprintf(stderr, "  6 = Complete all work units, report them, then shut down\n");
   fprintf(stderr, "  7 = Complete all work units, then shut down\n");
   fprintf(stderr, "  9 = Continue from client left off when it was shut down\n");

   while (option != SO_CONTINUE && 
          option != SO_REPORT_ABORT_WORK &&
          option != SO_REPORT_ABORT_QUIT &&
          option != SO_REPORT_COMPLETED_CONTINUE &&
          option != SO_COMPLETE_INPROGRESS_ABORT_WORK &&
          option != SO_COMPLETE_INPROGRESS_ABORT_QUIT &&
          option != SO_COMPLETE_ALL_REPORT_QUIT &&
          option != SO_COMPLETE_ALL_QUIT &&
          option != SO_CONTINUE)
   {
      if (theChar != '\n' && theChar != '\r')
         fprintf(stderr, "\nChoose option: ");
      theChar = (char) getchar();

      if (isdigit(theChar))
         option = (theChar - '0');
   }

   return option;
}

void  TerminateWithError(string fmt, ...)
{
   va_list  args;

   va_start(args, fmt);
   vfprintf(stderr, fmt.c_str(), args);
   va_end(args);

   Sleep(10000);
}

void  CleanUpAndExit(void)
{
   int32_t  serverIdx;

   gp_Globals->p_Log->LogMessage("Client shutdown complete");

   for (serverIdx=0; serverIdx<gi_ServerHandlerCount; serverIdx++)
      delete gp_ServerHandler[serverIdx];

   delete [] gp_ServerHandler;

   delete gp_Globals->p_TestingProgramFactory;

   delete gp_Globals->p_Log;

   delete gp_Globals;

#if defined (_MSC_VER) && defined (MEMLEAK)
   bool  deleteIt;
   _CrtMemState mem_dbg2, mem_dbg3;

   if ( _CrtMemDifference( &mem_dbg3, &mem_dbg1, &mem_dbg2 ) )
   {
      _RPT0(_CRT_WARN, "\nDump the changes that occurred between two memory checkpoints\n");
      _CrtMemDumpStatistics( &mem_dbg3 );
      _CrtDumpMemoryLeaks( );
      deleteIt = false;
   }
   else
      deleteIt = true;

   CloseHandle(hLogFile);

   if (deleteIt)
      unlink("prpclient_memleak.txt");
#endif

   exit(0);
}
