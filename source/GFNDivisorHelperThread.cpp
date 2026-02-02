#include "GFNDivisorHelperThread.h"

#include "SQLStatement.h"
#include "StatsUpdaterFactory.h"
#include "StatsUpdater.h"
#include "GFNDivisorWorkSender.h"
#include "GFNDivisorWorkReceiver.h"

void  GFNDivisorHelperThread::ProcessRequest(string theMessage)
{
   char     tempMessage[200];
   GFNDivisorWorkSender* workSender;
   GFNDivisorWorkReceiver* workReceiver;

   strcpy(tempMessage, theMessage.c_str());
   if (!memcmp(tempMessage, "ADMIN_STATS ", 12))
      if (VerifyAdminPassword(tempMessage + 12))
         AdminStatsUpdate(false);

   if (!memcmp(tempMessage, "ADMIN_SERVER_STATS ", 19))
      if (VerifyAdminPassword(tempMessage + 19))
         AdminStatsUpdate(false);

   if (!memcmp(tempMessage, "ADMIN_USER_STATS ", 17))
      if (VerifyAdminPassword(tempMessage + 17))
         AdminStatsUpdate(true);

   if (!memcmp(tempMessage, "ADD_GFN_RANGE ", 14))
      if (VerifyAdminPassword(tempMessage + 14))
         AdminAddRanges();

   if (!memcmp(tempMessage, "EXPIRE_WORKUNIT ", 16))
      if (VerifyAdminPassword(tempMessage + 16))
         ExpireWorkunitTest();

   if (!memcmp(tempMessage, "GETWORK", 7))
   {
      workSender = new GFNDivisorWorkSender(ip_DBInterface, ip_Socket, ip_Globals,
         is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
      workSender->ProcessMessage(tempMessage);
      delete workSender;
   }

   if (!memcmp(tempMessage, "RETURNWORK", 10))
   {
      workReceiver = new GFNDivisorWorkReceiver(ip_DBInterface, ip_Socket, ip_Globals,
         is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
      workReceiver->ProcessMessage(tempMessage);
      delete workReceiver;
   }
}

void      GFNDivisorHelperThread::AdminAddRanges(void)
{
   StatsUpdaterFactory* suf;
   StatsUpdater* su;
   SQLStatement* insertStatement;
   const char* insertSQL = "insert into GFNDRange (n, kMin, kMax) values (?, ?, ?)";
   char* readBuf;
   uint32_t ranges;
   int32_t  n = 0, nMin, nMax;
   int64_t  kMin, kMax;

   readBuf = ip_Socket->Receive();

   if (!readBuf) return;

   if (sscanf(readBuf, "ADD_GFN_RANGE %u %u %" PRIu64" %" PRIu64"", &nMin, &nMax, &kMin, &kMax) != 4)
   {
      ip_Socket->Send("Command not in the correct format", readBuf);
      return;
   }
   insertStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   insertStatement->BindInputParameter(n, false);
   insertStatement->BindInputParameter(kMin, false);
   insertStatement->BindInputParameter(kMax, false);

   ranges = 0;
   for (n=nMin; n<=nMax; n++)
   {
      insertStatement->SetInputParameterValue(n, true);
      insertStatement->SetInputParameterValue(kMin);
      insertStatement->SetInputParameterValue(kMax);

      if (!insertStatement->Execute())
      {
         ip_DBInterface->Rollback();
         ip_Socket->Send("Server encounted an error upon insert");
         delete insertStatement;
         return;
      }

      // Send keepalive every 256 inserts
      if (!(++ranges & 0xff))
         ip_Socket->Send("keepalive");
   }

   delete insertStatement;

   suf = new StatsUpdaterFactory();
   su = suf->GetInstance(ip_DBInterface, ip_Log, ii_ServerType, ip_Globals->b_NeedsDoubleCheck);

   if (!su->RollupGroupStats(true))
      ip_DBInterface->Rollback();
   else
      ip_DBInterface->Commit();

   delete su;
   delete suf;

   ip_Socket->Send("Added %d new ranges for n from %d to %d and k from %" PRIu64" to %" PRIu64" ", 
      ranges, nMin, nMax, kMin, kMax);
   ip_Socket->Send("End of Message");

   ip_Log->LogMessage("ADMIN:  %d new ranges added through the admin process", ranges);
}

void      GFNDivisorHelperThread::ExpireWorkunitTest(void)
{
   char* theMessage;
   int32_t n, rows;
   int64_t kMin;
   bool  success;
   SQLStatement* sqlStatement;
   const char* deleteSQL = "delete from GFNDRangeTest " \
      " where n = ? and kMin = ?" \
      "   and SearchingProgram is null";
   const char* updateSQL = "update GFNDRange " \
      "   set rangeStatus = 0 " \
      " where n = ? and kMin = ?";

   theMessage = ip_Socket->Receive();
   if (!theMessage) return;

   if (sscanf(theMessage, "%d %" PRId64"", &n, &kMin) != 2)
   {
      ip_Socket->Send("Could not parse message %s", theMessage + 7);
      ip_Socket->Send("End of Message");
      return;
   }

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   sqlStatement->BindInputParameter(n);
   sqlStatement->BindInputParameter(kMin);

   success = sqlStatement->Execute();
   rows = sqlStatement->GetRowsAffected();

   delete sqlStatement;

   if (success && rows)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(n);
      sqlStatement->BindInputParameter(kMin);
      success = sqlStatement->Execute();
      delete sqlStatement;
   }

   if (success)
   {
      if (rows)
      {
         ip_Log->LogMessage("ADMIN:  Test for n=%d and kMin=%" PRId64" was expired", n, kMin);
         ip_Socket->Send("Test for n=%d and kMin=%" PRId64" was expired", n, kMin);
         ip_DBInterface->Commit();
      }
      else
      {
         ip_Socket->Send("Test for n=%d and kMin=%" PRId64" was not found", n, kMin);
         ip_DBInterface->Rollback();
      }
   }
   else
   {
      ip_Socket->Send("Failed to expire test for lower limit %s", theMessage + 7);
      ip_DBInterface->Rollback();
   }

   ip_Socket->Send("End of Message");
}
