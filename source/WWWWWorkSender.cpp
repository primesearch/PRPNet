#include "WWWWWorkSender.h"
#include "SQLStatement.h"
#include "server.h"
#include "KeepAliveThread.h"

WWWWWorkSender::WWWWWorkSender(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                               string userID, string emailID, 
                               string machineID, string instanceID, string teamID)
                               : WorkSender(dbInterface, theSocket, globals,
                                            userID, emailID, machineID, instanceID, teamID)
{
   char  *readBuf;

   ii_SpecialThreshhold = globals->i_SpecialThreshhold;
   ib_HasSoftware = false;
   ib_CanDoWieferich = ib_CanDoWilson = ib_CanDoWallSunSun = ib_CanDoWolstenholme = false;
   while (true)
   {
      readBuf = ip_Socket->Receive();

      if (!readBuf)
         break;

      if (!memcmp(readBuf, "wwww", 4))
         ib_HasSoftware = true;

      if (!memcmp(readBuf, "wwwwcl", 6))
         ib_HasSoftware = true;

      if (!memcmp(readBuf, "End of Message", 14))
         break;
   }
}

void  WWWWWorkSender::ProcessMessage(string theMessage)
{
   int32_t      sendWorkUnits, sentWorkUnits;
   char         clientVersion[20];
   char         tempMessage[200];

   strcpy(tempMessage, theMessage.c_str());
   clientVersion[0] = 0;
   if (sscanf(tempMessage, "GETWORK %s %u", clientVersion, &sendWorkUnits) != 2)
      sendWorkUnits = 1;

   is_ClientVersion = clientVersion;
   if (!ib_HasSoftware)
   {
      ip_Socket->Send("ERROR:  The client must run wwww to use this server.");
      ip_Socket->Send("End of Message");
      return;
   }

   if (sendWorkUnits > ii_MaxWorkUnits)
   {
      sendWorkUnits = ii_MaxWorkUnits;
      ip_Socket->Send("INFO: Server has a limit of %u work units.", ii_MaxWorkUnits);
   }

   ip_Socket->Send("ServerConfig: %d", ii_SpecialThreshhold);

   sentWorkUnits = ib_NoNewWork ? 0 : SelectWork(sendWorkUnits);

   if (sentWorkUnits == 0)
   {
      if (ib_NoNewWork)
         ip_Socket->Send("INACTIVE: New work generation is disabled on this server.");
      else
         ip_Socket->Send("INACTIVE: No available candidates are left on this server.");
   }

   ip_Socket->Send("End of Message");

   if (sentWorkUnits > 0)
      SendGreeting("Greeting.txt");
}

int32_t  WWWWWorkSender::SelectWork(int32_t sendWorkUnits)
{
   KeepAliveThread *keepAliveThread;
   SharedMemoryItem *threadWaiter;
   SQLStatement *selectStatement;
   const char    *dcWhere = "";
   bool     encounteredError, endLoop;
   int32_t  sentWorkUnits, completedTests;
   int64_t  lowerLimit, upperLimit;
   const char *selectSQL = "select LowerLimit, UpperLimit, CompletedTests " \
                           "  from WWWWRange " \
                           " where DoubleChecked = 0 " \
                           "   and HasPendingTest = 0 " \
                           "   %s " \
                           "order by LowerLimit limit 100";

   if (!ib_NeedsDoubleCheck)
      dcWhere = " and CompletedTests = 0";

   threadWaiter = ip_Socket->GetThreadWaiter();
   threadWaiter->SetValueNoLock(KAT_WAITING);
   keepAliveThread = new KeepAliveThread();
   keepAliveThread->StartThread(ip_Socket);

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, dcWhere);
   selectStatement->BindSelectedColumn(&lowerLimit);
   selectStatement->BindSelectedColumn(&upperLimit);
   selectStatement->BindSelectedColumn(&completedTests);

   sentWorkUnits = 0;
   encounteredError = false;

   while (sentWorkUnits < sendWorkUnits && !encounteredError)
   {
      if (!selectStatement->FetchRow(false)) break;

      if (completedTests > 0 && !CheckDoubleCheck(lowerLimit, upperLimit))
      {
         ip_Log->Debug(DEBUG_WORK, "%d: Cannot double-check %" PRId64":%" PRId64"",
                       ip_Socket->GetSocketID(), lowerLimit, upperLimit);
         continue;
      }

      // Allow only one thread through here at a time
      ip_Locker->Lock();

      if (ReserveRange(lowerLimit, upperLimit))
      {
         if (completedTests == 0)
            ip_Log->Debug(DEBUG_WORK, "%d: First check for %" PRId64":%" PRId64"",
                          ip_Socket->GetSocketID(), lowerLimit, upperLimit);
         else
            ip_Log->Debug(DEBUG_WORK, "%d: Double check for %" PRId64":%" PRId64"",
                          ip_Socket->GetSocketID(), lowerLimit, upperLimit);

         if (SendWork(lowerLimit, upperLimit))
         {
            sentWorkUnits++;
            ip_DBInterface->Commit();
         }
         else
         {
            ip_DBInterface->Rollback();
            encounteredError = true;
         }
      }

      ip_Locker->Release();

      selectStatement->CloseCursor();
   }

   // Just in case a commit or rollback was not done
   ip_DBInterface->Rollback();

   delete selectStatement;

   // When we know that the keepalive thread has terminated, we can then exit
   // this loop and delete the memory associated with that thread.
   endLoop = false;
   while (!endLoop)
   {
      threadWaiter->Lock();

      if (threadWaiter->GetValueHaveLock() == KAT_TERMINATED)
      {
         endLoop = true;
         threadWaiter->Release();
      }
      else
      {
         threadWaiter->SetValueHaveLock(KAT_TERMINATE);
         threadWaiter->Release();
         Sleep(1000);
      }
   }

   delete keepAliveThread;
   return sentWorkUnits;
}

bool     WWWWWorkSender::CheckDoubleCheck(int64_t lowerLimit, int64_t upperLimit)
{
   bool     canDoubleCheck;
   int32_t  count;
   SQLStatement *sqlStatement;
   char     emailID[ID_LENGTH+1], machineID[ID_LENGTH+1];
   const char *selectSQL = "select count(*) from WWWWRangeTest " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ? " \
                           "   and EmailID = ? " \
                           "   and MachineID = ?";

   canDoubleCheck = false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);
   sqlStatement->BindInputParameter(emailID, ID_LENGTH, false);
   sqlStatement->BindInputParameter(machineID, ID_LENGTH, false);
   sqlStatement->BindSelectedColumn(&count);

   switch (ii_DoubleChecker)
   {
      case DC_DIFFBOTH:
         strcpy(emailID, is_EmailID.c_str());
         strcpy(machineID, is_MachineID.c_str());
         break;

      case DC_DIFFEMAIL:
         strcpy(emailID, is_EmailID.c_str());
         strcpy(machineID, " ");

         break;
      case DC_DIFFMACHINE:
         strcpy(emailID, " ");
         strcpy(machineID, is_MachineID.c_str());
         break;


      default:
         canDoubleCheck = true;
  }

   sqlStatement->SetInputParameterValue(lowerLimit, true);
   sqlStatement->SetInputParameterValue(upperLimit);
   sqlStatement->SetInputParameterValue(emailID);
   sqlStatement->SetInputParameterValue(machineID);

   if (!canDoubleCheck)
   {
      if (sqlStatement->FetchRow(true) && count == 0)
         canDoubleCheck = true;
   }

   delete sqlStatement;

   return canDoubleCheck;
}

bool     WWWWWorkSender::ReserveRange(int64_t lowerLimit, int64_t upperLimit)
{
   SQLStatement *sqlStatement;
   bool     didUpdate;
   const char *updateSQL = "update WWWWRange " \
                           "   set HasPendingTest = 1 " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ? " \
                           "   and HasPendingTest = 0";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);

   didUpdate = sqlStatement->Execute();

   // The update only fails if there is an SQL error.  If no rows are updated, then
   // it will say that it was successful, but won't have updated any rows.  Verify
   // that the row was updated before replying that this was successful.
   if (sqlStatement->GetRowsAffected() == 0)
      didUpdate = false;

   if (!didUpdate)
      ip_DBInterface->Rollback();

   delete sqlStatement;

   return didUpdate;
}

bool     WWWWWorkSender::SendWork(int64_t lowerLimit, int64_t upperLimit)
{
   int64_t  lastUpdateTime;
   bool     sent;
   SQLStatement *sqlStatement;
   SharedMemoryItem *threadWaiter;
   const char *insertSQL = "insert into WWWWRangeTest " \
                           "( LowerLimit, UpperLimit, TestID, EmailID, MachineID, InstanceID, UserID, TeamID ) " \
                           "values ( ?,?,?,?,?,?,?,? )";

   lastUpdateTime = (int64_t) time(NULL);

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);
   sqlStatement->BindInputParameter(lastUpdateTime);
   sqlStatement->BindInputParameter(is_EmailID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_MachineID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_InstanceID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_UserID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_TeamID, ID_LENGTH);

   if (!sqlStatement->Execute())
   {
      delete sqlStatement;
      return false;
   }

   delete sqlStatement;

   threadWaiter = ip_Socket->GetThreadWaiter();
   threadWaiter->Lock();

   sent = ip_Socket->Send("WorkUnit: %" PRId64" %" PRId64" %" PRId64"",
                          lowerLimit, upperLimit, lastUpdateTime);

   threadWaiter->Release();

   if (!sent)
      return false;
   
   if (ib_BriefTestLog)
      ip_Log->TestMessage("%d: %" PRId64":%" PRId64" sent to %s/%s/%s/%s", ip_Socket->GetSocketID(),
                          ip_Socket->GetSocketID(), lowerLimit, upperLimit,
                          is_EmailID.c_str(), is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
   else
      ip_Log->TestMessage("%d: %" PRId64":%" PRId64" sent to Email: %s  User: %s  Machine: %s  Instance: %s",
                          ip_Socket->GetSocketID(), lowerLimit, upperLimit,
                          is_EmailID.c_str(), is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());

   return true;
}
