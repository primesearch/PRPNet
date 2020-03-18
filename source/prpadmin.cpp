// prpadmin.cpp
//
// Mark Rodenkirch (rogue@wi.rr.com)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defs.h"

#ifdef WIN32
  #include <io.h>
  #include <direct.h>
#endif

#ifdef UNIX
   #include <signal.h>
#endif

#include "Log.h"
#include "ClientSocket.h"

#define  RECORDS_PER_CONFIRM  100

// global variables
char       g_EmailID[200];
char       g_ServerName[200];
uint32_t   g_PortID;
uint32_t   g_DebugLevel;
char       g_Password[100];
uint32_t   g_Quit;
Log       *g_Log;
ClientSocket *g_Socket;

// procedure declarations
void     AdminMenu(void);
bool     OpenAdminSocket(void);
void     CloseAdminSocket(void);
char    *PromptForValue(const char *prompt, uint32_t verifyFile);
bool     VerifyCommand(const char *theCommand);
void     SendABCFile(const char *abcFileName);
void     SendFactorFile(const char *factorFileName);
void     SendRanges(int32_t ranges);
void     ExpireWorkunit(const char *workunit);
void     SetQuitting(int a);
void     PipeInterrupt(int a);
void     ProcessCommandLine(int count, char *arg[]);
void     Usage(char *exeName);
bool     ConfirmBatch(int countSent, bool allDone);

void      AdminMenu(void)
{
   int   menuitem = -1;
   char  response[500], *theMessage;
   int32_t ranges;

   while (true)
   {
      printf("\nPRPNet Admin menu\n");
      printf("==========\n");
      printf(" 1. Enter new server address (%s)\n", g_ServerName);
      printf(" 2. Enter new server port # (%d)\n", g_PortID);
      printf(" 3. Enter admin password\n");
      if (strlen(g_Password))
      {
         printf(" 4. Add new candidates from ABC file\n");
         printf(" 5. Remove candidates based upon factors file\n");
         printf(" 6. Add search ranges\n");
         printf("10. Expire work unit\n");
         printf("91. Recompute user and team statistics\n");
         printf("92. Recompute server statistics\n");
      }
      printf("99. Quit.\n");

      printf("Enter option from menu: ");
      fgets(response, sizeof(response), stdin);
      menuitem = atol(response);

      if (!strlen(g_Password) && menuitem > 3 && menuitem != 99)
         menuitem = 999;

      switch (menuitem)
      {
         case 99:
            return;

         case 1:
            strcpy(g_ServerName, PromptForValue("Enter server address: ", false));
            g_Password[0] = 0;
            break;

         case 2:
            g_PortID = atol(PromptForValue("Enter port number: ", false));
            g_Password[0] = 0;
            break;

         case 3:
            strcpy(g_Password, PromptForValue("Enter administrator password: ", false));
            break;

         case 4:
            strcpy(response, PromptForValue("Enter ABC file to be imported: ", true));

            if (strlen(response) && OpenAdminSocket())
            {
               SendABCFile(response);
               CloseAdminSocket();
            }
            break;

         case 5:
            strcpy(response, PromptForValue("Enter factor file to be applied: ", true));

            if (strlen(response) && OpenAdminSocket())
            {
               SendFactorFile(response);
               CloseAdminSocket();
            }
            break;


         case 6:
            strcpy(response, PromptForValue("Enter number of ranges to add: ", false));
            ranges = atol(response);

            if (strlen(response) && ranges > 0 && OpenAdminSocket())
            {
               SendRanges(ranges);
               CloseAdminSocket();
            }
            break;

         case 10:
            strcpy(response, PromptForValue("Enter candidate (or low end of wwww range) to expire: ", false));

            if (strlen(response) && OpenAdminSocket())
            {
               ExpireWorkunit(response);
               CloseAdminSocket();
            }
            break;

         case 91:
            if (OpenAdminSocket())
            {
               VerifyCommand("ADMIN_USER_STATS");
               theMessage = g_Socket->Receive(120);
               if (theMessage)
                  printf("%s\n", theMessage);
               CloseAdminSocket();
            }
            break;

         case 92:
            if (OpenAdminSocket())
            {
               VerifyCommand("ADMIN_SERVER_STATS");
               theMessage = g_Socket->Receive(120);
               if (theMessage)
                  printf("%s\n", theMessage);
               CloseAdminSocket();
            }
            break;

         default:
            printf("Invalid choice\n");
      }
   }

   CloseAdminSocket();
}

char   *PromptForValue(const char *prompt, uint32_t verifyFile)
{
   static char response[200];
   char  readBuf[500];
   FILE *fPtr;

   printf("%s", prompt);

   fgets(response, sizeof(response), stdin);

   StripCRLF(response);

   if (!verifyFile)
      return response;

   // Do a lot of pre-validation so that the client doesn't send garbage data to the server
   // The server will re-validate, but this will help reduce wasted bandwidth in case the
   // file is unusable.
   if ((fPtr = fopen(response, "r")) == NULL)
   {
      printf("File [%s] could not be opened.\n", response);
      response[0] = 0;
      return response;
   }

   if (fgets(readBuf, BUFFER_SIZE, fPtr) == NULL)
   {
      printf("File [%s] is empty.\n", response);
      response[0] = 0;
   }

   fclose(fPtr);
   return response;
}

bool     OpenAdminSocket(void)
{
   char *userID = (char *) "admin_user";
   char *machineID = (char *) "admin_machine";
   char *instanceID = (char *) "admin_instance";
   char *teamID = (char *) "admin_team";

   g_Socket = new ClientSocket(g_Log, g_ServerName, g_PortID, "admin");

   if (g_Socket->Open(g_EmailID, userID, machineID, instanceID, teamID))
      return true;

   g_Log->LogMessage("Error opening socket");
   delete g_Socket;
   g_Socket = 0;

   return false;
}

void      CloseAdminSocket(void)
{
   if (!g_Socket)
      return;

   g_Socket->Close();

   delete g_Socket;
   g_Socket = 0;
}

bool     VerifyCommand(const char *theCommand)
{
   char    *theMessage;
   bool     verified;

   // Send the command along with the password
   g_Socket->Send("%s %s", theCommand, g_Password);

   // Don't go any further if the password is not accepted
   theMessage = g_Socket->Receive();

   if (!theMessage)
      return false;

   if (memcmp(theMessage, "OK:", 3))
   {
      printf("%s", theMessage);
      verified = false;
   }
   else
      verified = true;

   return verified;
}

void  SendABCFile(const char *abcFileName)
{
   char        readBuf[BUFFER_SIZE], *theMessage;
   FILE       *fPtr;
   int32_t     countSent;
   bool        successful = true;

   if (!VerifyCommand("ADMIN_ABC"))
      return;

   fPtr = fopen(abcFileName, "r");

   if (fgets(readBuf, BUFFER_SIZE, fPtr) == NULL)
   {
      g_Socket->Send("End of File");
      fclose(fPtr);
      return;
   }

   if (memcmp(readBuf, "ABC ", 4) && memcmp(readBuf, "ABCD ", 5))
      g_Socket->Send("generic");

   StripCRLF(readBuf);
   g_Socket->Send(readBuf);

   // This tells us if the ABC can be used with the server
   // If it can't, then stop here and avoid wasting bandwidth
   theMessage = g_Socket->Receive();
   if (!theMessage)
   {
      printf("The server didn't respond\n");
      fclose(fPtr);
      return;
   }

   if (memcmp(theMessage, "OK:", 3))
   {
      printf("%s\n", theMessage);
      fclose(fPtr);
      return;
   }

   theMessage = 0;
   countSent = 0;
   g_Socket->StartBuffering();

   // Send the entire file and have the server parse it
   while (fgets(readBuf, BUFFER_SIZE, fPtr) != NULL)
   {
      if (countSent && countSent % RECORDS_PER_CONFIRM == 0)
      {
         successful = ConfirmBatch(countSent, false);
         if (!successful)
            break;
      }

      countSent++;
      StripCRLF(readBuf);
      g_Socket->Send(readBuf);
   }

   fclose(fPtr);

   if (successful)
      successful = ConfirmBatch(countSent, true);

   if (!successful)
      g_Log->LogMessage("Sent %d, some might not have been processed     ", countSent);
}

bool  ConfirmBatch(int countSent, bool allDone)
{
   char  *theMessage;
   char   expectedMessage[100];
   bool   successful = false;

   sprintf(expectedMessage, "processed %d records", countSent);

   printf(" sent %d, server has processed %d      \r", countSent, countSent - RECORDS_PER_CONFIRM);
   fflush(stdout);

   g_Socket->Send("sent %d records", countSent);
   if (allDone)
      g_Socket->Send("End of File");

   g_Socket->SendBuffer();
   g_Socket->StartBuffering();

   theMessage = g_Socket->Receive(120);
   while (theMessage != 0x00)
   {
      if (!strcmp(theMessage, expectedMessage) || !memcmp(theMessage, "processed", 9))
      {
         successful = true;
         break;
      }

      if (memcmp(theMessage, "keepalive", 9))
         g_Log->LogMessage(theMessage);

      theMessage = g_Socket->Receive(120);
   }

   if (successful && allDone)
   {
      theMessage = g_Socket->Receive(120);
      while (theMessage)
      {
         if (!memcmp(theMessage, "End of Message", 14))
            break;

         if (memcmp(theMessage, "keepalive", 9))
            g_Log->LogMessage(theMessage);

         theMessage = g_Socket->Receive(120);
      }
   }

   return successful;
}

void  SendFactorFile(const char *factorFileName)
{
   char        readBuf[BUFFER_SIZE], *theMessage;
   FILE       *fPtr;
   int32_t     endLoop = false, countSent;

   if (!VerifyCommand("ADMIN_FACTOR"))
      return;

   fPtr = fopen(factorFileName, "r");

   theMessage = 0;
   countSent = 0;
   g_Socket->StartBuffering();

   // Send the entire file and have the server parse it
   while (fgets(readBuf, BUFFER_SIZE, fPtr) != NULL)
   {
      if (countSent && countSent % RECORDS_PER_CONFIRM == 0)
      {
         printf(" sent %d, server has processed %d      \r", countSent, countSent - RECORDS_PER_CONFIRM);
         fflush(stdout);
         g_Socket->Send("sent %d records", countSent);
         g_Socket->SendBuffer();
         g_Socket->StartBuffering();

         theMessage = g_Socket->Receive(120);
         while (theMessage)
         {
            if (!memcmp(theMessage, "processed", 9))
               break;

            theMessage = g_Socket->Receive(120);
         }

         if (!theMessage)
            break;
      }

      countSent++;
      StripCRLF(readBuf);
      g_Socket->Send(readBuf);
   }

   fclose(fPtr);

   if (!theMessage)
   {
      g_Socket->Send("sent %d records", countSent);
      g_Socket->Send("End of File");
      g_Socket->SendBuffer();
   }
   else
   {
      if (countSent >= RECORDS_PER_CONFIRM)
         printf("\n");
      g_Log->LogMessage("Did not get a response from the server after 120 seconds");
      g_Log->LogMessage("Verify what the server has processed, then edit the file before sending it again.");
   }


   theMessage = g_Socket->Receive(120);
   while (theMessage && !endLoop)
   {
      if (!memcmp(theMessage, "End of Message", 14))
         endLoop = true;
      else if (!memcmp(theMessage, "processed", 9))
         printf(" Sent %d, all have been processed     \n", countSent);
      else
         if (memcmp(theMessage, "keepalive", 9))
            g_Log->LogMessage(theMessage);

      if (!endLoop)
         theMessage = g_Socket->Receive(30);
   }
}

void  SendRanges(int32_t ranges)
{
   char       *theMessage;
   int32_t     endLoop = false;

   if (!VerifyCommand("ADMIN_ADD_RANGE"))
      return;

   g_Socket->Send("ADD %d", ranges);

   theMessage = g_Socket->Receive(120);
   while (theMessage && !endLoop)
   {
      if (!memcmp(theMessage, "End of Message", 14))
         endLoop = true;
      else
         if (memcmp(theMessage, "keepalive", 9))
            g_Log->LogMessage(theMessage);

      if (!endLoop)
         theMessage = g_Socket->Receive(30);
   }
}

void  ExpireWorkunit(const char *workunit)
{
   char       *theMessage;
   int32_t     endLoop = false;

   if (!VerifyCommand("EXPIRE_WORKUNIT"))
      return;

   g_Socket->Send("EXPIRE %s", workunit);

   theMessage = g_Socket->Receive(120);
   while (theMessage && !endLoop)
   {
      if (!memcmp(theMessage, "End of Message", 14))
         endLoop = true;
      else
         if (memcmp(theMessage, "keepalive", 9))
            g_Log->LogMessage(theMessage);

      if (!endLoop)
         theMessage = g_Socket->Receive(30);
   }
}

void  PipeInterrupt(int a)
{
   g_Log->LogMessage("Pipe Interrupt with code %ld.  Processing will continue", a);
}

void SetQuitting(int a)
{
   g_Log->LogMessage("Accepted force quit.  Waiting to close sockets before exiting");
   g_Quit = a = 1;
}

int main(int argc, char *argv[])
{
   char     logFile[200];

#if defined (_MSC_VER) && defined (MEMLEAK)
	_CrtMemState mem_dbg1, mem_dbg2, mem_dbg3;
   HANDLE hLogFile;
   int    deleteIt;

   hLogFile = CreateFile("prpadmin_memleak.txt", GENERIC_WRITE, 
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

   sprintf(logFile, "prpadmin.log");

   // Set default values for the global variables
   strcpy(g_EmailID, "PRPNet_Admin_user@");
   strcpy(g_ServerName, "localhost");
   g_PortID = 7100;
   g_DebugLevel = 0;
   g_Quit = 0;
   g_Password[0] = 0;

   // Get override values from the command line
   ProcessCommandLine(argc, argv);

   g_Log = new Log(10000000, logFile, g_DebugLevel, true);

   if (strlen(g_EmailID) == 0)
   {
      g_Log->LogMessage("An email address must be configured with -e.");
      exit(-1);
   }

   g_Log->LogMessage("PRPnet Admin application v%s started.", PRPNET_VERSION);
   g_Log->LogMessage("Configured to connect to %s:%ld", g_ServerName, g_PortID);

#ifdef UNIX
   signal(SIGHUP, SIG_IGN);
   signal(SIGINT, SetQuitting);
   signal(SIGQUIT, SetQuitting);
   signal(SIGTERM, SetQuitting);
   signal(SIGPIPE, PipeInterrupt);
#else
   // This has to be done before anything is done with sockets
   WSADATA  wsaData;
   WSAStartup(MAKEWORD(1,1), &wsaData);
#endif

   AdminMenu();

   CloseAdminSocket();

   delete g_Log;

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
      unlink("prpadmin_memleak.txt");
#endif

   return 0;
}

// Parse the command line to get any overrides of the default values
void  ProcessCommandLine(int count, char *arg[])
{
   int   i = 1;

   while (i < count)
   {
      if (!strcmp(arg[i],"-p"))
      {
         g_PortID = atol(arg[i+1]);
         i++;
      }
      else if (!strcmp(arg[i],"-s"))
      {
         strcpy(g_ServerName, arg[i+1]);
         i++;
      }
      else if (!strcmp(arg[i],"-e"))
      {
         strcpy(g_EmailID, arg[i+1]);
         i++;
      }
      else if (!strcmp(arg[i],"-a"))
      {
         strcpy(g_Password, arg[i+1]);
         i++;
      }
      else if (!strcmp(arg[i], "-debug"))
         g_DebugLevel = 1;
      else
      {
         Usage(arg[0]);
         exit(0);
      }

      i++;
   }
}

void  Usage(char *exeName)
{
   printf("Usage: %s [-s addr] [-p port#] [-a pwd] [-e email]\n", exeName);
   printf("    -s addr    Specify address of server to connect to.\n");
   printf("    -p port #  Specify port number to connect to.\n");
   printf("    -a pwd     Specify server admin password.\n");
   printf("    -e email   Specify email address to provide to server.\n");
}


