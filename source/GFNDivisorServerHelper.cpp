#include "GFNDivisorServerHelper.h"
#include "SQLStatement.h"

int32_t   GFNDivisorServerHelper::ComputeHoursRemaining(void)
{
   return 999;
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