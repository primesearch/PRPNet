#include "HelperThread.h"

#ifdef WIN32
   static DWORD WINAPI HelperThreadEntryPoint(LPVOID threadInfo);
#else
   static void *HelperThreadEntryPoint(void *threadInfo);
#endif

#include "HTMLGeneratorFactory.h"
#include "PrimeHTMLGenerator.h"
#include "StatsUpdaterFactory.h"
#include "StatsUpdater.h"

void  HelperThread::StartThread(globals_t *theGlobals, HelperSocket *theSocket)
{
   ip_Globals = theGlobals;

   ip_Log = theGlobals->p_Log;
   ii_ServerType = theGlobals->i_ServerType;

   is_HTMLTitle = theGlobals->s_HTMLTitle;
   is_ProjectTitle = theGlobals->s_ProjectTitle;
   is_SortLink = theGlobals->s_SortLink;
   is_CSSLink = theGlobals->s_CSSLink;
   is_AdminPassword = theGlobals->s_AdminPassword;

   ip_Socket = theSocket;

   ip_DBInterface = new DBInterface(ip_Log, "database.ini");

   ip_ClientCounter = theGlobals->p_ClientCounter;

   ip_ClientCounter->IncrementValue();

#ifdef WIN32
   // Ignore the thread handle return since the parent process won't suspend
   // or terminate the thread.
   CreateThread(0,                        // security attributes (0=use parents)
                0,                        // stack size (0=use parents)
                HelperThreadEntryPoint,
                this,
                0,                        // execute immediately
                0);                       // don't care about the thread ID
#else
   pthread_create(&ih_thread, NULL, &HelperThreadEntryPoint, this);
   pthread_detach(ih_thread);
#endif
}

#ifdef WIN32
DWORD WINAPI HelperThreadEntryPoint(LPVOID threadInfo)
#else
static void *HelperThreadEntryPoint(void *threadInfo)
#endif
{
   HelperThread *helper;

   helper = (HelperThread *) threadInfo;

   helper->HandleClient();

   helper->ReleaseResources();

   delete helper;

#ifdef WIN32
   return 0;
#else
   pthread_exit(0);
#endif
}

void  HelperThread::ReleaseResources(void)
{
   ip_ClientCounter->DecrementValue();

   if (ip_DBInterface->IsConnected())
      ip_DBInterface->Disconnect();

   ip_Socket->Close();

   delete ip_DBInterface;
   delete ip_Socket;
}

void  HelperThread::HandleClient(void)
{
   char    *theMessage;
   char     clientVersion[50];
   char     userID[ID_LENGTH+1];
   char     emailID[ID_LENGTH+1];
   char     machineID[ID_LENGTH+1];
   char     instanceID[ID_LENGTH+1];
   char     teamID[ID_LENGTH+1];

   is_UserID = "";
   is_TeamID = "";
   is_EmailID = "";
   is_MachineID = "";
   is_InstanceID = "";

   theMessage = ip_Socket->Receive(20);

   if (!theMessage)
      return;

   if (!memcmp(theMessage, "GET ", 4))
   {
      if (ip_DBInterface->Connect(ip_Socket->GetSocketID()))
         SendHTML(theMessage);
      return;
   }

   if (!theMessage)
   {
      ip_Log->LogMessage("Expected 'FROM', but message was '%s'.  The connection was dropped.", theMessage);
      return;
   }

   if (strlen(theMessage) > 200)
   {
      ip_Log->LogMessage("The FROM message is too long to be parsed.  The connected was dropped.");
      return;
   }

   // Support for 5.2 and later (first supported by 4.2, but 5.2 client is first to use it.
   if (!memcmp(theMessage, "FROM 5.", 7))
   {
      if (sscanf(theMessage, "FROM %s %s %s %s %s %s", clientVersion, emailID, machineID, userID, teamID, instanceID) != 6)
      {
         if (sscanf(theMessage, "FROM %s %s %s %s %s", clientVersion, emailID, machineID, userID, teamID) != 5)
         {
            ip_Log->LogMessage("The FROM message [%s] did not specify required information.  The connection was dropped.", theMessage);
            return;
         }
         strcpy(instanceID, machineID);
      }
   }
   else
   {
      if (sscanf(theMessage, "FROM %s %s %s %s %s", emailID, machineID, userID, clientVersion, teamID) != 5)
      {
         ip_Log->LogMessage("The FROM message [%s] did not specify required information.  The connection was dropped.", theMessage);
         return;
      }
      strcpy(instanceID, machineID);
   }

   if (memcmp(clientVersion, "4.3", 3) < 0)
   {
      ip_Log->LogMessage("The client is too old.  The connection was dropped.", theMessage);
      return;
   }

   if (!ip_DBInterface->Connect(ip_Socket->GetSocketID()))
   {
      ip_Log->LogMessage("Not connected to database.  Rejecting client connection.");
      return;
   }

   if (memcmp(clientVersion, "5.0", 3) < 0)
      ip_Socket->Send("Connected to server %s", PRPNET_VERSION);
   else
      ip_Socket->Send("Connected to server %s %d", PRPNET_VERSION, ii_ServerType);

   theMessage = ip_Socket->Receive(20);
   if (!theMessage)
      return;

   // Clear out the team name placeholder
   if (!strcmp(teamID, NO_TEAM))
      teamID[0] = 0;

   is_UserID = userID;
   is_TeamID = teamID;
   is_EmailID = emailID;
   is_MachineID = machineID;
   is_InstanceID = instanceID;

   ProcessRequest(theMessage);
}

void  HelperThread::SendHTML(string theMessage)
{
   char                  thePage[200];
   char                 *buffer;
   char                  tempMessage[1000];
   HTMLGenerator        *htmlGenerator;

   if (theMessage.length() > sizeof(tempMessage)-1)
      return;

   strcpy(tempMessage, theMessage.c_str());

   if (strstr(tempMessage, ".htm"))
      sscanf(tempMessage, "GET /%s ", thePage);
   else if (strstr(tempMessage, ".ico"))
      sscanf(tempMessage, "GET /%s ", thePage);
   else
      thePage[0] = 0;

   buffer = tempMessage;
   while (buffer)
   {
      buffer = ip_Socket->Receive();

      if (!buffer)
         break;

      if (!memcmp(buffer, "Connection", 10))
         break;
   }

   htmlGenerator = HTMLGeneratorFactory::GetHTMLGenerator(ip_Globals);

   htmlGenerator->SetDBInterface(ip_DBInterface);
   htmlGenerator->SetLog(ip_Log);
   htmlGenerator->SetSocket(ip_Socket);

   htmlGenerator->Send(thePage);

   delete htmlGenerator;
}

bool  HelperThread::VerifyAdminPassword(string password)
{
   if (is_AdminPassword == password)
   {
      ip_Socket->Send("OK: Password Verified");
      ip_Log->LogMessage("%s at %s is connecting as admin", is_EmailID.c_str(), ip_Socket->GetFromAddress().c_str());
      return true;
   }
   else
   {
      ip_Socket->Send("ERR: The entered password is not valid.  No access will be granted.");
      ip_Log->LogMessage("%s at %s attempted to connect as admin, but was rejected", is_EmailID.c_str(), ip_Socket->GetFromAddress().c_str());
      return false;
   }
}

void      HelperThread::AdminStatsUpdate(bool userStats)
{
   StatsUpdaterFactory *suf;
   StatsUpdater *su;
   bool  success;

   suf = new StatsUpdaterFactory();
   su = suf->GetInstance(ip_DBInterface, ip_Log, ii_ServerType, ip_Globals->b_NeedsDoubleCheck);

   if (userStats)
      success = su->RollupAllStats();
   else
      success = su->RollupGroupStats(true);

   if (success)
      ip_DBInterface->Commit();
   else
      ip_DBInterface->Rollback();

   delete su;
   delete suf;

   if (success)
      ip_Log->LogMessage("ADMIN:  Group stats have been resynced");

   if (success)
      ip_Socket->Send("%s stats have been resynced", (userStats ? "User and Team" : "Server"));
   else
      ip_Socket->Send("An error occurred on the server when trying to re-sync statistics");
}
