#include "PrimeServerHelper.h"
#include "SQLStatement.h"

int32_t   PrimeServerHelper::ComputeHoursRemaining(void)
{
   SQLStatement *sqlStatement;
   int32_t  countTested, countUntested, hoursLeft;
   int64_t  startTime, nowTime, endTime;
   double   percent;
   bool     success;

   const char *sumSelect = "select sum(CountTested), sum(countUntested) " \
                           "  from CandidateGroupStats";
   const char *timeSelect = "select min(TestID) " \
                            "  from CandidateTest";

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

void  PrimeServerHelper::ExpireTests(bool canExpire, int32_t delayCount, delay_t *delays)
{
   int64_t  currentTime, testAge, testID;
   int32_t  ii, expireTest;
   SQLStatement *selectStatement;
   SQLStatement *updateStatement;
   SQLStatement *deleteStatement;
   double   decimalLength;
   char     candidateName[NAME_LENGTH+1];
   char     userID[ID_LENGTH+1], emailID[ID_LENGTH+1];
   char     machineID[ID_LENGTH+1], instanceID[ID_LENGTH+1];
   const char *selectSQL = "select Candidate.CandidateName, Candidate.DecimalLength, " \
                           "       CandidateTest.TestID, " \
                           "       CandidateTest.UserID, CandidateTest.EmailID, " \
                           "       CandidateTest.MachineID, " \
                           "       CandidateTest.InstanceID " \
                           "  from Candidate, CandidateTest " \
                           " where Candidate.CandidateName = CandidateTest.CandidateName " \
                           "   and CandidateTest.IsCompleted = 0";
   const char *deleteSQL = "delete from CandidateTest " \
                           " where CandidateName = ? " \
                           "   and TestId = ?";
   const char *updateSQL = "update Candidate " \
                           "   set HasPendingTest = 0 " \
                           " where CandidateName = ?";

   if (!canExpire)
      return;

   currentTime = time(NULL);

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   selectStatement->BindSelectedColumn(&decimalLength);
   selectStatement->BindSelectedColumn(&testID);
   selectStatement->BindSelectedColumn(userID, ID_LENGTH);
   selectStatement->BindSelectedColumn(emailID, ID_LENGTH);
   selectStatement->BindSelectedColumn(machineID, ID_LENGTH);
   selectStatement->BindSelectedColumn(instanceID, ID_LENGTH);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(candidateName, NAME_LENGTH, false);

   deleteStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   deleteStatement->BindInputParameter(candidateName, NAME_LENGTH, false);
   deleteStatement->BindInputParameter(testID);

   while (selectStatement->FetchRow(false))
   {
      testAge = currentTime - testID;

      expireTest = false;
      for (ii=0; ii<delayCount; ii++)
         if (decimalLength < delays[ii].maxLength &&
             testAge > delays[ii].expireDelay)
            expireTest = true;

      if (expireTest)
      {
         updateStatement->SetInputParameterValue(candidateName, true);
         deleteStatement->SetInputParameterValue(candidateName, true);
         deleteStatement->SetInputParameterValue(testID);

         if (updateStatement->Execute() && deleteStatement->Execute())
         {
            ip_Log->LogMessage("Test of %s for email %s / machine %s / instance %s has expired.",
                                          candidateName, emailID, machineID, instanceID);
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

void  PrimeServerHelper::UnhideSpecials(int32_t unhideSeconds)
{
   SQLStatement *selectStatement;
   SQLStatement *updateStatement;
   int64_t  cutoff;
   int32_t  unhideCount;
   char     candidateName[NAME_LENGTH+1];
   const char *selectSQL = "select CandidateName " \
                           "  from UserPrimes " \
                           " where ShowOnWebPage = 0 " \
                           "   and DateReported < ?";
   const char *updateSQL = "update UserPrimes set ShowOnWebPage = 1 " \
                           " where CandidateName = ?";

   cutoff = (int64_t) time(NULL);
   cutoff -= unhideSeconds;

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(cutoff);
   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(candidateName, NAME_LENGTH, false);

   unhideCount = 0;
   while (selectStatement->FetchRow(false))
   {
      updateStatement->SetInputParameterValue(candidateName, true);

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
