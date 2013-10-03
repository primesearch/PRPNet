#include "WorkSender.h"

WorkSender::WorkSender(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                       string userID, string emailID, 
                       string machineID, string instanceID, string teamID)
{
   ip_DBInterface = dbInterface;
   ip_Socket = theSocket;
   ip_Log = globals->p_Log;
   ip_Locker = globals->p_Locker;

   is_UserID = userID;
   is_EmailID = emailID;
   is_MachineID = machineID;
   is_InstanceID = instanceID;
   is_TeamID = teamID;
   is_ProjectTitle = globals->s_ProjectTitle;

   ii_ServerType = globals->i_ServerType;
   ii_MaxWorkUnits = globals->i_MaxWorkUnits;
   ib_NeedsDoubleCheck = globals->b_NeedsDoubleCheck;
   ii_DoubleChecker = globals->i_DoubleChecker;
   ib_BriefTestLog = globals->b_BriefTestLog;
   ib_NoNewWork = globals->b_NoNewWork;
}

// Send the specified file to the client
void     WorkSender::SendGreeting(string fileName)
{
   FILE *fp;
   char  line[BUFFER_SIZE];

   ip_Socket->StartBuffering();
   ip_Socket->Send("Start Greeting");

   if ((fp = fopen(fileName.c_str(), "rt")) == NULL)
      ip_Socket->Send("%s", is_ProjectTitle.c_str());
   else
   {
      while (fgets(line, BUFFER_SIZE, fp) != NULL)
      {
         StripCRLF(line);

         if (strlen(line) > 0)
            ip_Socket->Send("%s", line);
      }

      fclose(fp);
   }

   ip_Socket->Send("End Greeting");
   ip_Socket->SendBuffer();
}
