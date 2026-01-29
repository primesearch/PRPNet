#include "GFNDivisorServerHelper.h"
#include "SQLStatement.h"

int32_t   GFNDivisorServerHelper::ComputeHoursRemaining(void)
{
   int64_t  minTime, nowTime, completedRanges, totalRanges;
   const char* selectSQL1 = "select min(lastUpdateTime), count(*) " \
      "  from GFNDRange ";
   const char* selectSQL2 = "select count(*) " \
      "  from GFNDRange " \
      " where GFNDRange.rangeStatus = 2";

   SQLStatement* selectStatement1 = new SQLStatement(ip_Log, ip_DBInterface, selectSQL1);
   selectStatement1->BindSelectedColumn(&minTime);
   selectStatement1->BindSelectedColumn(&totalRanges);

   SQLStatement* selectStatement2 = new SQLStatement(ip_Log, ip_DBInterface, selectSQL2);
   selectStatement2->BindSelectedColumn(&completedRanges);

   int64_t hours = 1000000;

   if (selectStatement1->FetchRow(true) && selectStatement2->FetchRow(true))
   {
      if (totalRanges == 0)
         return 0;

      if (completedRanges > 0)
      {
         nowTime = time(NULL);
         hours = ((nowTime - minTime) / completedRanges);
         hours *= (totalRanges - completedRanges);
         hours /= 3600;
      }
   }

   delete selectStatement1;
   delete selectStatement2;

   return (int32_t) hours;
}

void  GFNDivisorServerHelper::ExpireTests(bool canExpire, int32_t delayCount, delay_t* delays)
{
   int32_t  n;
   int64_t  currentTime, testAge, testID;
   int64_t  kMin, kMax;
   SQLStatement* selectStatement;
   SQLStatement* updateStatement;
   SQLStatement* deleteStatement;
   char     userID[ID_LENGTH + 1], emailID[ID_LENGTH + 1];
   char     machineID[ID_LENGTH + 1], instanceID[ID_LENGTH + 1];
   const char* selectSQL = "select GFNDRange.n, GFNDRange.kMin, GFNDRange.kMax, " \
      "       GFNDRangeTest.TestID, " \
      "       GFNDRangeTest.UserID, GFNDRangeTest.EmailID, " \
      "       GFNDRangeTest.MachineID, " \
      "       GFNDRangeTest.InstanceID " \
      "  from GFNDRange, GFNDRangeTest " \
      " where GFNDRange.n = GFNDRangeTest.n " \
      "   and GFNDRange.kMin = GFNDRangeTest.kMin " \
      "   and GFNDRange.kMax = GFNDRangeTest.kMax " \
      "   and GFNDRange.rangeStatus = 1 ";
   const char* deleteSQL = "delete from GFNDRangeTest " \
      " where n = ? " \
      "   and kMin = ? " \
      "   and kMax = ? " \
      "   and testId = ?";
   const char* updateSQL = "update GFNDRange " \
      "   set rangeStatus = 0 " \
      " where n = ? " \
      "   and kMin = ? " \
      "   and kMax = ? ";

   if (!canExpire)
      return;

   currentTime = time(NULL);

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindSelectedColumn(&n);
   selectStatement->BindSelectedColumn(&kMin);
   selectStatement->BindSelectedColumn(&kMax);
   selectStatement->BindSelectedColumn(&testID);
   selectStatement->BindSelectedColumn(userID, ID_LENGTH);
   selectStatement->BindSelectedColumn(emailID, ID_LENGTH);
   selectStatement->BindSelectedColumn(machineID, ID_LENGTH);
   selectStatement->BindSelectedColumn(instanceID, ID_LENGTH);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(n);
   updateStatement->BindInputParameter(kMin);
   updateStatement->BindInputParameter(kMax);

   deleteStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   deleteStatement->BindInputParameter(n);
   deleteStatement->BindInputParameter(kMin);
   deleteStatement->BindInputParameter(kMax);
   deleteStatement->BindInputParameter(testID);

   while (selectStatement->FetchRow(false))
   {
      testAge = currentTime - testID;

      if (testAge > delays[0].expireDelay)
      {
         updateStatement->SetInputParameterValue(n, true);
         updateStatement->SetInputParameterValue(kMin);
         updateStatement->SetInputParameterValue(kMax);

         deleteStatement->SetInputParameterValue(n, true);
         deleteStatement->SetInputParameterValue(kMin);
         deleteStatement->SetInputParameterValue(kMax);
         deleteStatement->SetInputParameterValue(testID);

         if (updateStatement->Execute() && deleteStatement->Execute())
         {
            ip_Log->LogMessage("Test %u for %" PRIu64" <= k <= %" PRIu64" for email% s / machine % s / instance % s has expired.",
               n, kMin, kMax, emailID, machineID, instanceID);
            ip_DBInterface->Commit();
         }
         else
            ip_DBInterface->Rollback();
      }
   }

   delete deleteStatement;
   delete updateStatement;
   delete selectStatement;
}