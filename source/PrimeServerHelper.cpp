#include "PrimeServerHelper.h"
#include "SQLStatement.h"
#include "ABCParser.h"
#include "LengthCalculator.h"
#include "StatsUpdaterFactory.h"
#include "PrimeStatsUpdater.h"

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

   if (!success) return -1;
   if (countUntested == 0 && countTested == 0) return -2;
   if (countTested == 0) return -3;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, timeSelect);
   sqlStatement->BindSelectedColumn(&startTime);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return -1;

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

void      PrimeServerHelper::LoadABCFile(string abcFile)
{
   SQLStatement* selectStatement;
   LengthCalculator* lengthCalculator;
   StatsUpdaterFactory* suf;
   PrimeStatsUpdater* su;
   int32_t    totalEntries, newEntries, badEntries, dupEntries, failedInserts;
   int32_t    countFound;
   int64_t    theK, theC;
   int32_t    theB, theN, theD;
   double     decimalLength;
   string     candidateName;
   const char* selectSQL = "select count(*) from Candidate " \
      " where CandidateName = ?";

   ip_DBInterface->Connect(3);

   ABCParser* abcParser = new ABCParser(ii_ServerType, abcFile);

   if (!abcParser->IsValidFormat())
   {
      delete abcParser;
      ip_Log->LogMessage("ABC file has unknown format or is not supported for the server type", abcFile.c_str());
      return;
   }

   suf = new StatsUpdaterFactory();
   su = (PrimeStatsUpdater *) suf->GetInstance(ip_DBInterface, ip_Log, ii_ServerType, ib_NeedsDoubleCheck);
   delete suf;

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(candidateName, NAME_LENGTH, false);
   selectStatement->BindSelectedColumn(&countFound);

   totalEntries = newEntries = badEntries = dupEntries = failedInserts = 0;
   lengthCalculator = new LengthCalculator(ii_ServerType, ip_DBInterface, ip_Log);

   while (true)
   {
      rowtype_t rowType = abcParser->GetNextCandidate(candidateName, theK, theB, theN, theC, theD);

      if (rowType == RT_IGNORE)
         continue;

      if (rowType == RT_EOF)
         break;

      totalEntries++;

      if (theC != 1 && theC != -1 && (ii_ServerType == ST_SOPHIEGERMAIN || ii_ServerType == ST_TWIN || ii_ServerType == ST_TWINANDSOPHIE))
         badEntries++;
      else
      {
         selectStatement->SetInputParameterValue(candidateName, true);

         if (selectStatement->FetchRow(true) && countFound == 0)
         {
            if (ii_ServerType == ST_GENERIC)
               decimalLength = 0.0;
            else
               decimalLength = lengthCalculator->CalculateDecimalLength(theK, theB, theN, theD);

            if (!su->InsertCandidate(candidateName, theK, theB, theN, theC, theD, decimalLength))
               failedInserts++;
            else
               newEntries++;

            if (newEntries % 100 == 0)
               ip_DBInterface->Commit();
         }
         else
         {
            dupEntries++;
            ip_Log->LogMessage("ERR: %s is a duplicate and has been rejected", candidateName.c_str());
         }

         if (totalEntries % 10000 == 0)
         {
            printf(" Processed %d entries from the input file\r", totalEntries);
            fflush(stdout);
         }
      }
   }

   ip_DBInterface->Commit();

   delete selectStatement;
   delete abcParser;

   if (ii_ServerType == ST_GENERIC || ii_ServerType == ST_FACTORIAL ||
      ii_ServerType == ST_MULTIFACTORIAL || ii_ServerType == ST_PRIMORIAL)
      lengthCalculator->CalculateDecimalLengths(0);

   delete lengthCalculator;

   ip_Log->LogMessage("Total candidates received = %6d", totalEntries);
   ip_Log->LogMessage("           Failed Inserts = %6d", failedInserts);
   ip_Log->LogMessage("     New candidates added = %6d", newEntries);
   ip_Log->LogMessage("      Duplicates rejected = %6d", dupEntries);
   ip_Log->LogMessage("Bad entries from ABC file = %6d", badEntries);
   ip_Log->LogMessage("End of Message");

   if (!su->RollupGroupStats(true))
      ip_DBInterface->Rollback();
   else
      ip_DBInterface->Commit();

   delete su;

   ip_DBInterface->Disconnect();

   ip_Log->LogMessage("%d new candidates added through the ABC file", newEntries);
}