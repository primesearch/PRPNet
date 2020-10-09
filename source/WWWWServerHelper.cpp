#include "WWWWServerHelper.h"
#include "SQLStatement.h"

int32_t   WWWWServerHelper::ComputeHoursRemaining(void)
{
   SQLStatement *sqlStatement;
   int32_t  countTested, countUntested, hoursLeft;
   int64_t  startTime, nowTime, endTime;
   double   percent;
   bool     success;

   const char *sumSelect = "select sum(CountTested), sum(CountUntested) " \
                           "  from WWWWGroupStats ";
   const char *timeSelect = "select min(TestID) " \
                            "  from WWWWRangeTest";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, sumSelect);
   sqlStatement->BindSelectedColumn(&countTested);
   sqlStatement->BindSelectedColumn(&countUntested);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return 0;
   if (!countTested || !countUntested) return 0;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, timeSelect);
   sqlStatement->BindSelectedColumn(&startTime);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return 0;

   nowTime = time(NULL);

   percent = (double) countTested / (double) (countTested + countUntested);
   endTime = (int64_t) ((double) (nowTime - startTime) / percent);
   hoursLeft = (int32_t) ((endTime - nowTime + startTime) / 3600);

   return hoursLeft;
}

void  WWWWServerHelper::ExpireTests(bool canExpire, int32_t delayCount, delay_t *delays)
{
   int64_t  currentTime, testAge, testID;
   int64_t  lowerLimit, upperLimit;
   SQLStatement *selectStatement;
   SQLStatement *updateStatement;
   SQLStatement *deleteStatement;
   char     userID[ID_LENGTH+1], emailID[ID_LENGTH+1];
   char     machineID[ID_LENGTH+1], instanceID[ID_LENGTH+1];
   const char *selectSQL = "select WWWWRange.LowerLimit, WWWWRange.UpperLimit, " \
                           "       WWWWRangeTest.TestID, " \
                           "       WWWWRangeTest.UserID, WWWWRangeTest.EmailID, " \
                           "       WWWWRangeTest.MachineID, " \
                           "       WWWWRangeTest.InstanceID " \
                           "  from WWWWRange, WWWWRangeTest " \
                           " where WWWWRange.LowerLimit = WWWWRangeTest.LowerLimit " \
                           "   and WWWWRange.UpperLimit = WWWWRangeTest.UpperLimit " \
                           "   and WWWWRange.HasPendingTest = 1 " \
                           "   and WWWWRangeTest.SearchingProgram is null";
   const char *deleteSQL = "delete from WWWWRangeTest " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ? " \
                           "   and TestId = ?";
   const char *updateSQL = "update WWWWRange " \
                           "   set HasPendingTest = 0 " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ?";

   if (!canExpire)
      return;

   currentTime = time(NULL);

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindSelectedColumn(&lowerLimit);
   selectStatement->BindSelectedColumn(&upperLimit);
   selectStatement->BindSelectedColumn(&testID);
   selectStatement->BindSelectedColumn(userID, ID_LENGTH);
   selectStatement->BindSelectedColumn(emailID, ID_LENGTH);
   selectStatement->BindSelectedColumn(machineID, ID_LENGTH);
   selectStatement->BindSelectedColumn(instanceID, ID_LENGTH);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(lowerLimit);
   updateStatement->BindInputParameter(upperLimit);

   deleteStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   deleteStatement->BindInputParameter(lowerLimit);
   deleteStatement->BindInputParameter(upperLimit);
   deleteStatement->BindInputParameter(testID);

   while (selectStatement->FetchRow(false))
   {
      testAge = currentTime - testID;

      if (testAge > delays[0].expireDelay)
      {
         updateStatement->SetInputParameterValue(lowerLimit, true);
         updateStatement->SetInputParameterValue(upperLimit);
         deleteStatement->SetInputParameterValue(lowerLimit, true);
         deleteStatement->SetInputParameterValue(upperLimit);
         deleteStatement->SetInputParameterValue(testID);

         if (updateStatement->Execute() && deleteStatement->Execute())
         {
            ip_Log->LogMessage("Test of %" PRId64" for email %s / machine %s / instance %s has expired.",
                                          lowerLimit, emailID, machineID, instanceID);
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

void  WWWWServerHelper::UnhideSpecials(int32_t unhideSeconds)
{
   SQLStatement *selectStatement;
   SQLStatement *updateStatement;
   int64_t  cutoff;
   int32_t  unhideCount;
   int64_t  thePrime;
   const char *selectSQL = "select Prime " \
                           "  from UserWWWWs " \
                           " where ShowOnWebPage = 0 " \
                           "   and DateReported < ?";
   const char *updateSQL = "update UserWWWWs set ShowOnWebPage = 1 " \
                           " where Prime = ?";

   cutoff = (int64_t) time(NULL);
   cutoff -= unhideSeconds;

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(cutoff);
   selectStatement->BindSelectedColumn(&thePrime);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(thePrime);

   unhideCount = 0;
   while (selectStatement->FetchRow(false))
   {
      updateStatement->SetInputParameterValue(thePrime, true);

      if (updateStatement->Execute())
      {
         ip_DBInterface->Commit();
         unhideCount++;
      }
      else
         ip_DBInterface->Rollback();
   }

   delete updateStatement;
   delete selectStatement;

   if (unhideCount)
      ip_Log->LogMessage("%d prime%s will no longer be hidden on webpage",
                                    unhideCount, ((unhideCount>1) ? "s" : ""));
}
