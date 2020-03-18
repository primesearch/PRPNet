#include "WWWWHelperThread.h"

#include "SQLStatement.h"
#include "StatsUpdaterFactory.h"
#include "StatsUpdater.h"
#include "WWWWWorkSender.h"
#include "WWWWWorkReceiver.h"

void  WWWWHelperThread::ProcessRequest(string theMessage)
{
   char     tempMessage[200];
   WWWWWorkSender *workSender;
   WWWWWorkReceiver *workReceiver;
   
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

   if (!memcmp(tempMessage, "ADMIN_ADD_RANGE ", 16))
      if (VerifyAdminPassword(tempMessage + 16))
         AdminAddRange();

   if (!memcmp(tempMessage, "EXPIRE_WORKUNIT ", 16))
      if (VerifyAdminPassword(tempMessage + 16))
         ExpireWorkunitTest();

   if (!memcmp(tempMessage, "GETWORK", 7))
   {
      workSender = new WWWWWorkSender(ip_DBInterface, ip_Socket, ip_Globals,
                                      is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
      workSender->ProcessMessage(tempMessage);
      delete workSender;
   }

   if (!memcmp(tempMessage, "RETURNWORK", 10))
   {
      workReceiver = new WWWWWorkReceiver(ip_DBInterface, ip_Socket, ip_Globals,
                                          is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
      workReceiver->ProcessMessage(tempMessage);
      delete workReceiver;
   }
}

void      WWWWHelperThread::AdminAddRange(void)
{
   StatsUpdaterFactory *suf;
   StatsUpdater *su;
   SQLStatement *selectStatement;
   SQLStatement *insertStatement;
   const char *selectSQL = "select $null_func$(max(UpperLimit), 0) from WWWWRange ";
   const char *insertSQL = "insert into WWWWRange (LowerLimit, UpperLimit) values (?, ?)";
   char    *readBuf;
   int32_t  ii, ranges;
   int64_t  lowerLimit = 0, upperLimit = 0, rangeSize;
   int64_t  lowestLimit;
   int64_t  million = 1000000, billion = 1000000000;

   readBuf = ip_Socket->Receive();

   if (!readBuf) return;

   if (sscanf(readBuf, "ADD %d", &ranges) != 1)
   {
      ip_Socket->Send("Command not in the correct format", readBuf);
      return;
   }

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindSelectedColumn(&lowestLimit);
   if (!selectStatement->FetchRow(false))
   {
      ip_Socket->Send("Server encounted an error");
      delete selectStatement;
      return;
   }

   delete selectStatement;

   if (lowestLimit == 0)
   {
      // Specify starting points for each search
      // Wieferich starts at 3e15
      // Wilson starts at 1e10
      // Wall-Sun-Sun starts at 7e14
      // Wolstenholme starts at 5e8
      if (ii_ServerType == ST_WIEFERICH)    lowestLimit = 4 * billion * million;
      if (ii_ServerType == ST_WILSON)       lowestLimit = 10 * billion;
      if (ii_ServerType == ST_WALLSUNSUN)   lowestLimit = 970 * million * million;
      if (ii_ServerType == ST_WOLSTENHOLME) lowestLimit = 500 * million;
   }

   // Specify range size for each search
   // Wieferich -> 1e11
   // Wilson -> 1e6
   // Wall-Sun-Sun -> 1e10
   // Wolstenholme -> 1e7
   if (ii_ServerType == ST_WIEFERICH)    rangeSize = 100 * billion;
   if (ii_ServerType == ST_WILSON)       rangeSize = 1 * million;
   if (ii_ServerType == ST_WALLSUNSUN)   rangeSize = 10 * billion;
   if (ii_ServerType == ST_WOLSTENHOLME) rangeSize = 10 * million;

   insertStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   insertStatement->BindInputParameter(lowerLimit);
   insertStatement->BindInputParameter(upperLimit);

   lowerLimit = lowestLimit;
   for (ii=0; ii<ranges; ii++)
   {
      upperLimit = lowerLimit + rangeSize;

      insertStatement->SetInputParameterValue(lowerLimit, true);
      insertStatement->SetInputParameterValue(upperLimit);

      if (!insertStatement->Execute())
      {
         ip_DBInterface->Rollback();
         ip_Socket->Send("Server encounted an error upon insert");
         delete insertStatement;
         return;
      }

      // Send keepalive every 256 inserts
      if (!(ii & 0xff))
         ip_Socket->Send("keepalive");

      lowerLimit += rangeSize;
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

   ip_Socket->Send("Added %d new ranges from %" PRId64" to %" PRId64" ", ranges, lowestLimit, upperLimit);
   ip_Socket->Send("End of Message");

   ip_Log->LogMessage("ADMIN:  %d new ranges added through the admin process", ranges);
}

void      WWWWHelperThread::ExpireWorkunitTest(void)
{
   char *theMessage;
   int64_t  lowerLimit, rows;
   bool  success;
   SQLStatement *sqlStatement;
   const char *deleteSQL = "delete from WWWWRangeTest " \
                           " where LowerLimit = ? " \
                           "   and SearchingProgram is null";
   const char *updateSQL = "update WWWWRange " \
                           "   set HasPendingTest = 0 " \
                           " where LowerLimit = ?";

   theMessage = ip_Socket->Receive();
   if (!theMessage) return;

   lowerLimit = atoll(theMessage);

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   sqlStatement->BindInputParameter(lowerLimit);

   success = sqlStatement->Execute();
   rows = sqlStatement->GetRowsAffected();

   delete sqlStatement;

   if (success && rows)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(lowerLimit);
      success = sqlStatement->Execute();
      delete sqlStatement;
   }

   if (success)
   {
      if (rows)
      {
         ip_Log->LogMessage("ADMIN:  Test for lower limit %s was expired", theMessage+7);
         ip_Socket->Send("Test for lower limit %s was expired", theMessage+7);
         ip_DBInterface->Commit();
      }
      else
      {
         ip_Socket->Send("Test for lower limit %s was not found", theMessage+7);
         ip_DBInterface->Rollback();
      }
   }
   else
   {
      ip_Socket->Send("Failed to expire test for lower limit %s", theMessage+7);
      ip_DBInterface->Rollback();
   }

   ip_Socket->Send("End of Message");
}
