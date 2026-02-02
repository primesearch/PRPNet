#include "DMDivisorHelperThread.h"

#include "SQLStatement.h"
#include "StatsUpdaterFactory.h"
#include "StatsUpdater.h"
#include "DMDivisorWorkSender.h"
#include "DMDivisorWorkReceiver.h"

void  DMDivisorHelperThread::ProcessRequest(string theMessage)
{
   char     tempMessage[200];
   DMDivisorWorkSender* workSender;
   DMDivisorWorkReceiver* workReceiver;

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

   if (!memcmp(tempMessage, "ADD_DM_RANGE ", 13))
      if (VerifyAdminPassword(tempMessage + 13))
         AdminAddRanges();

   if (!memcmp(tempMessage, "EXPIRE_WORKUNIT ", 16))
      if (VerifyAdminPassword(tempMessage + 16))
         ExpireWorkunitTest();

   if (!memcmp(tempMessage, "GETWORK", 7))
   {
      workSender = new DMDivisorWorkSender(ip_DBInterface, ip_Socket, ip_Globals,
         is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
      workSender->ProcessMessage(tempMessage);
      delete workSender;
   }

if (!memcmp(tempMessage, "RETURNWORK", 10))
{
   workReceiver = new DMDivisorWorkReceiver(ip_DBInterface, ip_Socket, ip_Globals,
      is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
   workReceiver->ProcessMessage(tempMessage);
   delete workReceiver;
}
}

void      DMDivisorHelperThread::AdminAddRanges(void)
{
   StatsUpdaterFactory* suf;
   StatsUpdater* su;
   SQLStatement* insertStatement;
   const char* insertSQL = "insert into DMDRange (n, kMin, kMax) values (?, ?, ?)";
   char* readBuf;
   uint32_t ranges, rangesLeft;
   int32_t  n;
   int64_t  kMin, kRangeMin = 0, kRangeMax = 0, rangeSize;

   readBuf = ip_Socket->Receive();

   if (!readBuf) return;

   if (sscanf(readBuf, "ADD_DM_RANGE %u %" PRIu64" %u %" PRIu64"", &n, &kMin, &ranges, &rangeSize) != 4)
   {
      ip_Socket->Send("Command not in the correct format", readBuf);
      return;
   }
   insertStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   insertStatement->BindInputParameter(n, false);
   insertStatement->BindInputParameter(kRangeMin, false);
   insertStatement->BindInputParameter(kRangeMax, false);

   kRangeMin = kMin;

   rangesLeft = ranges;
   while (rangesLeft > 0)
   {
      int64_t kRangeMax = kRangeMin + rangeSize;

      if (kRangeMin == 1)
         kRangeMax--;

      insertStatement->SetInputParameterValue(n, true);
      insertStatement->SetInputParameterValue(kRangeMin);
      insertStatement->SetInputParameterValue(kRangeMax);

      if (!insertStatement->Execute())
      {
         ip_DBInterface->Rollback();
         ip_Socket->Send("Server encounted an error upon insert");
         delete insertStatement;
         return;
      }

      // Send keepalive every 256 inserts
      if (!(rangesLeft & 0xff))
         ip_Socket->Send("keepalive");

      kRangeMin = kRangeMax;
      rangesLeft--;
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

   ip_Socket->Send("Added %d new ranges for %d from %" PRIu64" to %" PRIu64" ", ranges, n, kMin, kRangeMax);
   ip_Socket->Send("End of Message");

   ip_Log->LogMessage("ADMIN:  %d new ranges added through the admin process", ranges);
}

void      DMDivisorHelperThread::ExpireWorkunitTest(void)
{
   char* theMessage;
   int32_t n, rows;
   int64_t kMin;
   bool  success;
   SQLStatement* sqlStatement;
   const char* deleteSQL = "delete from DMDRangeTest " \
      " where n = ? and kMin = ?" \
      "   and SearchingProgram is null";
   const char* updateSQL = "update DMDRange " \
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
