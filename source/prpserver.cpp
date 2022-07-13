// prpserver.cpp
//
// Mark Rodenkirch (rogue@wi.rr.com)

#include <stdio.h>
#include <time.h>
#include <ctype.h>

#ifdef WIN32
   #include <windows.h>
   BOOL WINAPI HandlerRoutine(DWORD /*dwCtrlType*/);
#else
   #include <signal.h>
#endif

#include "server.h"
#include "defs.h"
#include "ServerThread.h"
#include "DBInterface.h"
#include "SQLStatement.h"
#include "StatsUpdaterFactory.h"
#include "ServerHelperFactory.h"
#include "StatsUpdater.h"
#include "Mail.h"
#include "MailFactory.h"

globals_t  *gp_Globals;
Mail       *gp_Mail;

void     PipeInterrupt(int a);
void     SetQuitting(int sig);
void     ProcessINIFile(string configFile, string &smtpServer);
void     ReprocessINIFile(string configFile);
void     ProcessDelayFile(string delayFile);
bool     ValidateConfiguration(string smtpServer);
void     SendPrimeEmail(DBInterface *dbInterface);
void     SendWWWWEmail(DBInterface *dbInterface);
void     SyncGroupStats(DBInterface *dbInterface, bool doubleCheck);

int   main(int argc, char *argv[])
{
   char     abcFile[50], name[50];
   string   s_Mail;
   bool     lowWorkCheckDone = false;
   int32_t  showUsage;
   int32_t  ii, quit, counter, unhideSeconds;
   time_t   theTime;
   struct tm *theTm;
   ServerThread *serverThread = 0;
   DBInterface  *dbInterface = 0, *timerDBInterface = 0;
   ServerHelper *serverHelper = 0;

#if defined (_MSC_VER) && defined (MEMLEAK)
   _CrtMemState mem_dbg1, mem_dbg2, mem_dbg3;
   HANDLE hLogFile;
   int deleteIt;

   hLogFile = CreateFile("prpserver_memleak.txt", GENERIC_WRITE,
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
   gp_Globals->i_ServerType = 0;
   gp_Globals->i_PortID = 0;
   gp_Globals->b_NeedsDoubleCheck = false;
   gp_Globals->b_OneKPerInstance = false;
   gp_Globals->i_DoubleChecker = 0;
   gp_Globals->i_MaxWorkUnits = 1;
   gp_Globals->i_MaxClients = 10;
   gp_Globals->i_DestIDCount = 0;
   gp_Globals->s_HTMLTitle = "";
   gp_Globals->s_SortLink = "";
   gp_Globals->s_CSSLink = "";
   gp_Globals->s_ProjectTitle = "";
   gp_Globals->s_AdminPassword = "";
   gp_Globals->s_ProjectName = "";
   gp_Globals->p_Delay = 0;
   gp_Globals->p_Log = 0;
   gp_Globals->s_SortSequence = "";
   gp_Globals->b_UseLLROverPFGW = false;
   gp_Globals->b_LocalTimeHTML = true;
   gp_Globals->b_BriefTestLog = false;
   gp_Globals->b_ServerStatsSummaryOnly = false;
   gp_Globals->s_EmailID = "";
   gp_Globals->s_EmailPassword = "";
   gp_Globals->i_UnhidePrimeHours = 0;
   gp_Globals->i_SpecialThreshhold = 100;
   gp_Globals->i_NotifyLowWork = 0;
   gp_Globals->p_Quitting = new SharedMemoryItem("quit");
   gp_Globals->p_ClientCounter = new SharedMemoryItem("client_counter");
   gp_Globals->b_NoExpire = false;
   gp_Globals->b_NoNewWork = false;

   s_Mail.clear();
   gp_Mail = 0;

   showUsage = false;

   gp_Globals->p_Log = new Log(0, "prpserver.log", DEBUG_NONE, true);

   // Get default values from the configuration file
   ProcessINIFile("prpserver.ini", s_Mail);

   for (ii=1; ii<argc; ii++)
   {
      if (!memcmp(argv[ii], "-d", 2))
         gp_Globals->p_Log->SetDebugLevel(DEBUG_ALL);
      else if (!memcmp(argv[ii], "-l", 2))
         strcpy(abcFile, &argv[ii][2]);
      else
         showUsage = true;
   }

   if (showUsage)
   {
      printf("Mark Rodenkirch's PRPNet Server, Version %s\n", PRPNET_VERSION);
      printf("   -d                 Turn on all debugging\n");
      printf("   -h                 Show this menu\n");
      printf("   -l<filename>       Load ABC file <filename> into database\n");
      printf("   -u                 Upgrade from 2.x to 3.x\n");
      exit(0);
   }

   dbInterface = new DBInterface(gp_Globals->p_Log, "database.ini");
   if (!dbInterface->Connect(2))
      exit(0);

   ProcessDelayFile("prpserver.delay");

   if (!ValidateConfiguration(s_Mail))
   {
      delete gp_Globals->p_Log;
      return -1;
   }

#ifdef WIN32
   char     processName[200];

   WSADATA  wsaData;
   WSAStartup(MAKEWORD(1,1), &wsaData);

   SetConsoleCtrlHandler(HandlerRoutine, TRUE);

   // prevent multiple copies from running
   sprintf(processName, "PRPNetServer_Port%d", gp_Globals->i_PortID);
   void *mutex = CreateMutex(NULL, FALSE, processName);
   if (!mutex || ERROR_ALREADY_EXISTS == GetLastError())
   {
      printf("Unable to run multiple copies of the PRPNet server on port %d.\n", gp_Globals->i_PortID);
      exit(-1);
   }
#endif

#ifdef UNIX
   // Ignore SIGHUP, as to not die on logout.
   // We log to file in most cases anyway.
   signal(SIGHUP, SIG_IGN);
   signal(SIGTERM, SetQuitting);
   signal(SIGINT, SetQuitting);
   signal(SIGPIPE, SIG_IGN);

   int lockFile;
   lockFile = open("server.lock", O_WRONLY | O_CREAT | O_EXCL, 0600);
   if (lockFile <= 0)
   {
      printf("Only one copy of the PRPNet server can be run from this folder at a time.\n");
      exit(-1);
   }
#endif

   // If unhideprimehours is set to 0, then show primes immediately
   gp_Globals->b_ShowOnWebPage = !gp_Globals->i_UnhidePrimeHours;

   sprintf(name, "prpnet_port_%d", gp_Globals->i_PortID);
   gp_Globals->p_Locker = new SharedMemoryItem(name);

   serverThread = new ServerThread();
   serverThread->StartThread(gp_Globals);

   gp_Globals->p_Log->LogMessage("PRPNet Server application v%s started.", PRPNET_VERSION);
   gp_Globals->p_Log->LogMessage("Please contact Mark Rodenkirch at rogue@wi.rr.com for support");
   gp_Globals->p_Log->LogMessage("Waiting for connections on port %d", gp_Globals->i_PortID);

   timerDBInterface = new DBInterface(gp_Globals->p_Log, "database.ini");
   if (gp_Mail) gp_Mail->SetDBInterface(timerDBInterface);
   
   serverHelper = ServerHelperFactory::GetServerHelper(gp_Globals, timerDBInterface);

   // A minimum 10 minute wait
   unhideSeconds = gp_Globals->i_UnhidePrimeHours * 3600 + 600;
   counter = 0;
   quit = false;
   while (!quit)
   {
      counter++;
      Sleep(1000);

      time(&theTime);
      theTm = localtime(&theTime);
      if (theTm->tm_hour == 0) {
         if (!lowWorkCheckDone) {
            if (gp_Mail) {
               timerDBInterface->Connect(3);

               int32_t daysLeft = serverHelper->ComputeHoursRemaining() / 24;

               if (daysLeft < gp_Globals->i_NotifyLowWork)
                  gp_Mail->MailLowWorkNotification(daysLeft);

               lowWorkCheckDone = true;

               timerDBInterface->Disconnect();
            }
         }
      }
      else
         lowWorkCheckDone = false;
      
      // Read ini file every 256 seconds to reset the debug level
      if (!(counter & 0xff))
         ReprocessINIFile("prpserver.ini");

      // Send e-mail every 1024 seconds (if new primes/PRPs have been reported)
      if (!(counter & 0x3fff))
      {
         timerDBInterface->Connect(3);

         if (gp_Mail) gp_Mail->MailSpecialResults();

         serverHelper->ExpireTests(!gp_Globals->b_NoExpire, gp_Globals->i_DelayCount, gp_Globals->p_Delay);

         SyncGroupStats(timerDBInterface, gp_Globals->b_NeedsDoubleCheck);

         timerDBInterface->Disconnect();
      }

      if (!(counter % unhideSeconds))
      {
         timerDBInterface->Connect(3);

         serverHelper->UnhideSpecials(unhideSeconds);

         timerDBInterface->Disconnect();
      }

      // If the user is trying to terminate the process, then
      // wait until all clients have completed what they are
      // doing before shutting down the server.
      if (gp_Globals->p_Quitting->GetValueNoLock() != 0)
      {
         int32_t threadsLeft = (int32_t) gp_Globals->p_ClientCounter->GetValueNoLock();

         if (threadsLeft == 0)
            quit = true;
         else
         {
            if (!(counter & 0x0f))
               gp_Globals->p_Log->LogMessage("%d threads still executing", threadsLeft);
         }
      }
   }

   delete serverHelper;
   delete timerDBInterface;
   delete dbInterface;

   gp_Globals->p_Log->LogMessage("Server is shutting down");

   if (gp_Mail) delete gp_Mail;
   if (serverThread) delete serverThread;
   if (gp_Globals->p_ServerSocket) delete gp_Globals->p_ServerSocket;
   delete gp_Globals->p_Delay;
   delete gp_Globals->p_Log;
   delete gp_Globals->p_Quitting;
   delete gp_Globals->p_ClientCounter;
   delete gp_Globals->p_Locker;
   delete gp_Globals;

#if defined (_MSC_VER) && defined (MEMLEAK)
   _CrtMemCheckpoint( &mem_dbg2 );
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
      unlink("prpserver_memleak.txt");
#endif

#ifdef WIN32
   CloseHandle(mutex);
#else
   close(lockFile);
   unlink("server.lock");
#endif

   return 0;
}

// Parse the configuration file to get the default values
void  ProcessINIFile(string configFile, string &smtpServer)
{
   FILE *fp;
   char  line[241];

   if ((fp = fopen(configFile.c_str(), "r")) == NULL)
   {
      printf("Configuration file '%s' was not found", configFile.c_str());
      return;
   }

   while (fgets(line, 240, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "htmltitle=", 10))
         gp_Globals->s_HTMLTitle = line+10;
      else if (!memcmp(line, "projecttitle=", 13))
         gp_Globals->s_ProjectTitle = line+13;
      else if (!memcmp(line, "sortlink=", 9))
         gp_Globals->s_SortLink = line+9;
      else if (!memcmp(line, "csslink=", 8))
         gp_Globals->s_CSSLink = line+8;
      else if (!memcmp(line, "email=", 6))
         gp_Globals->s_EmailID = line+6;
      else if (!memcmp(line, "emailpassword=", 14))
         gp_Globals->s_EmailPassword = line+14;
      else if (!memcmp(line, "port=", 5))
         gp_Globals->i_PortID = atol(line+5);
      else if (!memcmp(line, "debuglevel=", 11))
         gp_Globals->p_Log->SetDebugLevel(atol(line+11));
      else if (!memcmp(line, "echotest=", 9))
         gp_Globals->p_Log->SetEchoTest((atol(line+9) == 0 ? false : true));
      else if (!memcmp(line, "brieftestlog=", 13))
         gp_Globals->b_BriefTestLog = (atol(line+13) == 0 ? false : true);
      else if (!memcmp(line, "doublecheck=", 12))
         gp_Globals->b_NeedsDoubleCheck = (atol(line+12) == 0 ? false : true);
      else if (!memcmp(line, "doublechecker=", 14))
         gp_Globals->i_DoubleChecker = atol(line+14);
      else if (!memcmp(line, "servertype=", 11))
         gp_Globals->i_ServerType = atol(line+11);
      else if (!memcmp(line, "loglimit=", 9))
         gp_Globals->p_Log->SetLogLimit(atol(line+9));
      else if (!memcmp(line, "localtimelog=", 13))
         gp_Globals->p_Log->SetUseLocalTime((atol(line+13) == 0 ? false : true));
      else if (!memcmp(line, "localtimelog=2", 14))
         gp_Globals->p_Log->AppendTimeZone(false);
      else if (!memcmp(line, "localtimehtml=", 14))
         gp_Globals->b_LocalTimeHTML = (atol(line+14) == 0 ? false : true);
      else if (!memcmp(line, "serverstatssummaryonly=", 23))
         gp_Globals->b_ServerStatsSummaryOnly = (atol(line+23) == 0 ? false : true);
      else if (!memcmp(line, "maxworkunits=", 13))
         gp_Globals->i_MaxWorkUnits = atol(line+13);
      else if (!memcmp(line, "maxclients=", 11))
         gp_Globals->i_MaxClients = atol(line+11);
      else if (!memcmp(line, "unhideprimehours=", 17))
         gp_Globals->i_UnhidePrimeHours = atol(line+17);
      else if (!memcmp(line, "usellroverpfgw=", 15))
         gp_Globals->b_UseLLROverPFGW = (atol(line+15) == 0 ? false : true);
      else if (!memcmp(line, "onekperinstance=", 16))
         gp_Globals->b_OneKPerInstance = (atol(line+16) == 0 ? false : true);
      else if (!memcmp(line, "adminpassword=", 14))
         gp_Globals->s_AdminPassword = line+14;
      else if (!memcmp(line, "projectname=", 12))
         gp_Globals->s_ProjectName = line+12;
      else if (!memcmp(line, "smtpserver=", 11))
         smtpServer = line+11;
      else if (!memcmp(line, "sortoption=", 11))
         gp_Globals->s_SortSequence = &line[11];
      else if (!memcmp(line, "specialthreshhold=", 18))
         gp_Globals->i_SpecialThreshhold = atoi(&line[18]);
      else if (!memcmp(line, "noexpire=", 9))
         gp_Globals->b_NoExpire = (atol(line+9) == 0) ? false : true;
      else if (!memcmp(line, "nonewwork=", 10))
         gp_Globals->b_NoNewWork = (atol(line+10) == 0) ? false : true;
      else if (!memcmp(line, "notifylowwork=", 14))
         gp_Globals->i_NotifyLowWork = atol(line+14);
      else if (!memcmp(line, "destid=", 7))
      {
         if (strlen(&line[7]) > 0  && gp_Globals->i_DestIDCount < 10)
         {
            gp_Globals->s_DestID[gp_Globals->i_DestIDCount] = line+7;
            gp_Globals->i_DestIDCount++;
         }
      }
   }

   fclose(fp);

   if (gp_Globals->i_ServerType == ST_GENERIC)
      gp_Globals->s_SortSequence = "a,l,m";

   if (gp_Globals->i_ServerType == ST_GFN)
      gp_Globals->s_SortSequence = "b,n";
}

void     ReprocessINIFile(string configFile)
{
   FILE    *fp;
   char     line[2001];

   if ((fp = fopen(configFile.c_str(), "r")) == NULL) return;

   // Extract the email ID and count the number of servers on this pass
   while (fgets(line, 2000, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "debuglevel=", 11))
         gp_Globals->p_Log->SetDebugLevel(atol(line+11));
   }

   fclose(fp);
}

// Parse the delay file
void  ProcessDelayFile(string delayFile)
{
   FILE    *fp;
   char     line[241], dcScope, expScope;
   int32_t  ii, dcDelay, expDelay;

   if ((fp = fopen(delayFile.c_str(), "r")) == NULL)
   {
      printf("Delay file '%s' was not found.  Will set defaults of 1 hour", delayFile.c_str());

      gp_Globals->i_DelayCount = 1;
      gp_Globals->p_Delay = new delay_t [gp_Globals->i_DelayCount];
      gp_Globals->p_Delay[0].maxLength = 999999999;
      gp_Globals->p_Delay[0].doubleCheckDelay = 3600;
      gp_Globals->p_Delay[0].expireDelay = 3600;
      return;
   }

   gp_Globals->i_DelayCount = 0;

   while (fgets(line, 240, fp) != NULL)
   {
      if (!memcmp(line, "delay=", 6))
         gp_Globals->i_DelayCount++;
   }

   fclose(fp);

   gp_Globals->i_DelayCount++;
   gp_Globals->p_Delay = new delay_t [gp_Globals->i_DelayCount];
   for (ii=0; ii<gp_Globals->i_DelayCount; ii++)
   {
      gp_Globals->p_Delay[ii].maxLength = 0;
      gp_Globals->p_Delay[ii].doubleCheckDelay = 0;
      gp_Globals->p_Delay[ii].expireDelay = 0;
   }

   fp = fopen(delayFile.c_str(), "r");

   ii = 0;
   while (fgets(line, 240, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "delay=", 6))
      {
         if (sscanf(line, "delay=%lf:%u%c:%u%c", &gp_Globals->p_Delay[ii].maxLength, &dcDelay, &dcScope, &expDelay, &expScope) == 5)
         {
            if (dcScope != 'm' && dcScope != 'h' && dcScope != 'd')
               printf("Scope should be 'm', 'h', or 'd'\n");
            else if (expScope != 'm' && expScope != 'h' && expScope != 'd')
               printf("Scope should be 'm', 'h', or 'd'\n");
            else
            {
               gp_Globals->p_Delay[ii].doubleCheckDelay = 60 * dcDelay * (dcScope == 'm' ? 1 : (dcScope == 'h' ? 60 : (60*24)));
               gp_Globals->p_Delay[ii].expireDelay = 60 * expDelay * (expScope == 'm' ? 1 : (expScope == 'h' ? 60 : (60*24)));
               ii++;

               if (gp_Globals->i_ServerType == ST_WIEFERICH ||
                   gp_Globals->i_ServerType == ST_WILSON ||
                   gp_Globals->i_ServerType == ST_WALLSUNSUN ||
                   gp_Globals->i_ServerType == ST_WOLSTENHOLME) break;
            }

            if (ii == 0)
               gp_Globals->p_Delay[ii].minLength = 0;
            else
               gp_Globals->p_Delay[ii].minLength = gp_Globals->p_Delay[ii-1].maxLength;
         }
         else
            printf("Unable to parse line %s in file %s\n", line, delayFile.c_str());
      }
   }

   gp_Globals->p_Delay[ii].maxLength = 999999999;
   gp_Globals->p_Delay[ii].doubleCheckDelay = gp_Globals->p_Delay[ii-1].doubleCheckDelay;
   gp_Globals->p_Delay[ii].expireDelay = gp_Globals->p_Delay[ii-1].expireDelay;

   fclose(fp);
}

bool     ValidateConfiguration(string smtpServer)
{
   char   *ptr;
   char    sortOption[100];
   bool    first;
   int     length;
   MailFactory *mailFactory;

   if (!gp_Globals->s_HTMLTitle.length())
   {
      printf("The HTML title must be configured.\n");
      return false;
   }

   if (!gp_Globals->s_ProjectTitle.length())
   {
      printf("The project title must be configured.\n");
      return false;
   }

   if (!gp_Globals->s_EmailID.length())
   {
      printf("An email address must be configured.\n");
      return false;
   }

   // The email ID must have a '@', but not a ':'
   if (gp_Globals->s_EmailID.find("@") == string::npos ||
       gp_Globals->s_EmailID.find(":") != string::npos)
   {
      printf("Email address supplied (%s) is not valid.\n", gp_Globals->s_EmailID.c_str());
      return false;
   }

   if (gp_Globals->i_ServerType != ST_SIERPINSKIRIESEL &&
       gp_Globals->i_ServerType != ST_CULLENWOODALL    &&
       gp_Globals->i_ServerType != ST_FIXEDBKC         &&
       gp_Globals->i_ServerType != ST_FIXEDBNC         &&
       gp_Globals->i_ServerType != ST_PRIMORIAL        &&
       gp_Globals->i_ServerType != ST_FACTORIAL        &&
       gp_Globals->i_ServerType != ST_MULTIFACTORIAL   &&
       gp_Globals->i_ServerType != ST_TWIN             &&
       gp_Globals->i_ServerType != ST_GFN              &&
       gp_Globals->i_ServerType != ST_XYYX             && 
       gp_Globals->i_ServerType != ST_SOPHIEGERMAIN    &&
       gp_Globals->i_ServerType != ST_WAGSTAFF         && 
       gp_Globals->i_ServerType != ST_CYCLOTOMIC       &&
       gp_Globals->i_ServerType != ST_CAROLKYNEA       &&
       gp_Globals->i_ServerType != ST_GENERIC          &&
       gp_Globals->i_ServerType != ST_WIEFERICH        &&
       gp_Globals->i_ServerType != ST_WILSON           &&
       gp_Globals->i_ServerType != ST_WALLSUNSUN       &&
       gp_Globals->i_ServerType != ST_WOLSTENHOLME)
   {
      printf("servertype is invalid.\n");
      return false;
   }

   if (gp_Globals->i_MaxWorkUnits < 1)
   {
      printf("maxworkunits is invalid.  Will set it to 1.\n");
      gp_Globals->i_MaxWorkUnits = 1;
   }

   if (gp_Globals->b_NeedsDoubleCheck != 0 && gp_Globals->b_NeedsDoubleCheck != 1)
   {
      printf("doublecheck is invalid.  Will set it to 0.\n");
      gp_Globals->b_NeedsDoubleCheck = 0;
   }

   if ( gp_Globals->i_ServerType != ST_SIERPINSKIRIESEL && gp_Globals->b_OneKPerInstance)
   {
      printf("onekperinstance can only be 1 for Sierpinski/Riesel servers.\n");
      return false;
   }

   length = (int32_t) gp_Globals->s_SortSequence.length();
   if (gp_Globals->i_ServerType == ST_WIEFERICH    ||
       gp_Globals->i_ServerType == ST_WILSON       ||
       gp_Globals->i_ServerType == ST_WALLSUNSUN   ||
       gp_Globals->i_ServerType == ST_WOLSTENHOLME)
   {
      gp_Globals->s_SortSequence = "LowerPrime";
   }
   else if (!length || length > 15 || !(length & 0x01))
   {
      printf("sortoption is invalid.  Will set it to l,a (by length then age).\n");
      gp_Globals->s_SortSequence = "DecimalLength,LastUpdateTime";
   }
   else
   {
      strcpy(sortOption, gp_Globals->s_SortSequence.c_str());
      gp_Globals->s_SortSequence = "";
      ptr = sortOption;
      first = true;
      while (*ptr)
      {
         if (!first) gp_Globals->s_SortSequence += ",";
         first = false;

         switch (toupper(*ptr))
         {
            case 'M':
               gp_Globals->s_SortSequence += "CandidateName";
               break;
            case 'L':
               gp_Globals->s_SortSequence += "DecimalLength";
               break;
            case 'A':
               gp_Globals->s_SortSequence += "LastUpdateTime";
               break;
            case 'B':
               gp_Globals->s_SortSequence += "abs(b)";
               break;
            case 'K':
               gp_Globals->s_SortSequence += "k";
               break;
            case 'N':
               gp_Globals->s_SortSequence += "n";
               break;
            case 'C':
               gp_Globals->s_SortSequence += "abs(c)";
               break;
            default:
               printf("sortoption %c is invalid. It will be ignored.\n", *ptr);
         }
         ptr++;
         if (!*ptr)
            break;

         if (*ptr != ',')
            printf("sortoption delimiter %c is invalid. It will be ignored.\n", *ptr);
         ptr++;
      }
   }

   if (gp_Globals->i_DoubleChecker != DC_DIFFBOTH && gp_Globals->i_DoubleChecker != DC_DIFFEMAIL &&
       gp_Globals->i_DoubleChecker != DC_DIFFMACHINE && gp_Globals->i_DoubleChecker != DC_ANY)
   {
      printf("doublechecker is invalid.  Will set it to 1 (different user and machine).\n");
      gp_Globals->i_DoubleChecker = DC_DIFFBOTH;
   }

   if (smtpServer.length() > 0)
   {
      char tempServer[200];
      strcpy(tempServer, smtpServer.c_str());
      ptr = strchr(tempServer, ':');
      if (!ptr)
         printf("The SMTP Server %s is not configured correctly.  Found factors will not be emailed to %s.\n",
                smtpServer.c_str(), gp_Globals->s_EmailID.c_str());
      else
      {
         *ptr = 0x00;
         ptr++;
         if (strlen(tempServer) > 0)
         {
            mailFactory = new MailFactory();
            gp_Mail = mailFactory->GetInstance(gp_Globals, tempServer, atol(ptr));
            delete mailFactory;
         }
      }
   }

   fflush(stdout);
   return true;
}

#ifdef WIN32
BOOL WINAPI HandlerRoutine(DWORD /*dwCtrlType*/)
{
   gp_Globals->p_Log->LogMessage("Windows control handler called.  Exiting");

   gp_Globals->p_Quitting->SetValueNoLock(1);

   return true;
}
#endif

void PipeInterrupt(int a)
{
   gp_Globals->p_Log->LogMessage("Pipe Interrupt with code %d.  Processing will continue", a);
}

void SetQuitting(int sig)
{
   if (gp_Globals->p_Quitting->GetValueNoLock() == 5)
   {
      printf("Couldn't lock memory.  Cannot quit.\n");
      return;
   }

#ifndef WIN32
   const char *sigText = "";

   if (sig == SIGINT)
      sigText = "SIGINT - Interrupt (Ctrl-C)";
   if (sig == SIGQUIT)
      sigText = "SIGQUIT";
   if (sig == SIGHUP)
      sigText = "SIGHUP - Timeout or Disconnect";
   if (sig == SIGTERM)
      sigText = "SIGTERM - Software termination signal from kill";

   gp_Globals->p_Log->LogMessage("Signal recieved:  %s", sigText);
#endif

   gp_Globals->p_Quitting->IncrementValue();

#ifndef WIN32
   return;
   signal(sig, SIG_DFL);
   raise(sig);
#endif
}

void     SyncGroupStats(DBInterface *dbInterface, bool doubleCheck)
{
   StatsUpdater *statsUpdater;
   StatsUpdaterFactory *suf;

   suf = new StatsUpdaterFactory();
   statsUpdater = suf->GetInstance(dbInterface, gp_Globals->p_Log, gp_Globals->i_ServerType, gp_Globals->b_NeedsDoubleCheck);
   delete suf;

   if (statsUpdater->RollupGroupStats(false))
      dbInterface->Commit();
   else
      dbInterface->Rollback();

   delete statsUpdater;
}
