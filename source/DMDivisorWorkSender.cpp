#include "DMDivisorWorkSender.h"
#include "SQLStatement.h"
#include "server.h"
#include "KeepAliveThread.h"

DMDivisorWorkSender::DMDivisorWorkSender(DBInterface* dbInterface, Socket* theSocket, globals_t* globals,
   string userID, string emailID,
   string machineID, string instanceID, string teamID)
   : WorkSender(dbInterface, theSocket, globals,
      userID, emailID, machineID, instanceID, teamID)
{
   char* readBuf;

   ib_HasSoftware = false;
   while (true)
   {
      readBuf = ip_Socket->Receive();

      if (!readBuf)
         break;

      if (!memcmp(readBuf, "dmdsieve", 8))
         ib_HasSoftware = true;

      if (!memcmp(readBuf, "End of Message", 14))
         break;
   }
}

void  DMDivisorWorkSender::ProcessMessage(string theMessage)
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

int32_t  DMDivisorWorkSender::SelectWork(int32_t sendWorkUnits)
{
   KeepAliveThread* keepAliveThread;
   SharedMemoryItem* threadWaiter;
   SQLStatement* selectStatement;
   const char* dcWhere = "";
   bool     encounteredError, endLoop;
   int32_t  n, sentWorkUnits;
   int64_t  kMin, kMax;
   const char* selectSQL = "select n, kMin, kMax " \
      "  from DMDRange " \
      " where rangeStatus = 0 " \
      "order by n, kMin limit 100";

   threadWaiter = ip_Socket->GetThreadWaiter();
   threadWaiter->SetValueNoLock(KAT_WAITING);
   keepAliveThread = new KeepAliveThread();
   keepAliveThread->StartThread(ip_Socket);

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, dcWhere);
   selectStatement->BindSelectedColumn(&n);
   selectStatement->BindSelectedColumn(&kMin);
   selectStatement->BindSelectedColumn(&kMax);

   sentWorkUnits = 0;
   encounteredError = false;

   while (sentWorkUnits < sendWorkUnits && !encounteredError)
   {
      if (!selectStatement->FetchRow(false)) break;

      // Allow only one thread through here at a time
      ip_Locker->Lock();

      if (ReserveRange(n, kMin, kMax))
      {
         ip_Log->Debug(DEBUG_WORK, "%d: Sending for n=%u %" PRIu64" <= k <= %" PRIu64"",
            ip_Socket->GetSocketID(), n, kMin, kMax);

         if (SendWork(n, kMin, kMax))
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

bool     DMDivisorWorkSender::ReserveRange(int32_t n, int64_t kMin, int64_t kMax)
{
   SQLStatement* sqlStatement;
   bool     didUpdate;
   const char* updateSQL = "update DMDRange " \
      "   set rangeStatus = 1 " \
      " where n = ? " \
      "   and kMin = ? " \
      "   and kMax = ? " \
      "   and rangeStatus = 0";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   sqlStatement->BindInputParameter(n);
   sqlStatement->BindInputParameter(kMin);
   sqlStatement->BindInputParameter(kMax);
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

bool     DMDivisorWorkSender::SendWork(int32_t n, int64_t kMin, int64_t kMax)
{
   int64_t  lastUpdateTime;
   bool     sent;
   SQLStatement* sqlStatement;
   SharedMemoryItem* threadWaiter;
   const char* insertSQL = "insert into DMDRangeTest " \
      "( n, kMin, kMax, TestID, EmailID, MachineID, InstanceID, UserID ) " \
      "values ( ?,?,?,?,?,?,?,? )";

   lastUpdateTime = (int64_t)time(NULL);

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   sqlStatement->BindInputParameter(n);
   sqlStatement->BindInputParameter(kMin);
   sqlStatement->BindInputParameter(kMax);
   sqlStatement->BindInputParameter(lastUpdateTime);
   sqlStatement->BindInputParameter(is_EmailID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_MachineID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_InstanceID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_UserID, ID_LENGTH);

   if (!sqlStatement->Execute())
   {
      delete sqlStatement;
      return false;
   }

   delete sqlStatement;

   threadWaiter = ip_Socket->GetThreadWaiter();
   threadWaiter->Lock();

   sent = ip_Socket->Send("WorkUnit: %u %" PRIu64" %" PRIu64" %" PRIu64"",
      n, kMin, kMax, lastUpdateTime);

   threadWaiter->Release();

   if (!sent)
      return false;

   if (ib_BriefTestLog)
      ip_Log->TestMessage("%d: n=%u %" PRIu64" <= k <= %" PRIu64" sent to %s/%s/%s/%s", ip_Socket->GetSocketID(),
         ip_Socket->GetSocketID(), n, kMin, kMax,
         is_EmailID.c_str(), is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
   else
      ip_Log->TestMessage("%d: n=%u %" PRIu64" <= k <= %" PRIu64" sent to Email: %s  User: %s  Machine: %s  Instance: %s",
         ip_Socket->GetSocketID(), n, kMin, kMax,
         is_EmailID.c_str(), is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());

   return true;
}
