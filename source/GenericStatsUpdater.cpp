#include "GenericStatsUpdater.h"
#include "SQLStatement.h"

string  GenericStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   return "generic";
}

bool  GenericStatsUpdater::UpdateCandidateGroupStats(char* candidateGroupId)
{
   SQLStatement* sqlStatement;
   bool          success;
   int32_t       countInGroup, countUntested, countTested, countDoubleChecked;
   int32_t       countInProgress, prpsAndPrimesFound;

   const char* selectSQL =
      "select count(n), " \
      "       sum(case when completedTests = 0 then 1 else 0 end), " \
      "       sum(case when completedTests > 0 then 1 else 0 end), " \
      "       sum(case when doubleChecked  = 1 then 1 else 0 end), " \
      "       sum(case when hasPendingTest = 1 then 1 else 0 end), " \
      "       sum(case when mainTestResult > 0 then 1 else 0 end) " \
      "  from Candidate " \
      " where candidateGroupId = ?";


   const char* updateSQL =
      "update CandidateGroupStats " \
      "   set countInGroup = ?, " \
      "       countUntested = ?, " \
      "       countTested = ?, " \
      "       countDoubleChecked = ?, " \
      "       countInProgress = ?, " \
      "       prpsAndPrimesFound = ? " \
      " where candidateGroupId = ?";

   // Get the easy fields to compute
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(is_GroupName, ID_LENGTH, true);
   sqlStatement->BindSelectedColumn(&countInGroup);
   sqlStatement->BindSelectedColumn(&countUntested);
   sqlStatement->BindSelectedColumn(&countTested);
   sqlStatement->BindSelectedColumn(&countDoubleChecked);
   sqlStatement->BindSelectedColumn(&countInProgress);
   sqlStatement->BindSelectedColumn(&prpsAndPrimesFound);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   sqlStatement->BindInputParameter(countInGroup);
   sqlStatement->BindInputParameter(countUntested);
   sqlStatement->BindInputParameter(countTested);
   sqlStatement->BindInputParameter(countDoubleChecked);
   sqlStatement->BindInputParameter(countInProgress);
   sqlStatement->BindInputParameter(prpsAndPrimesFound);
   sqlStatement->BindInputParameter(is_GroupName, ID_LENGTH, true);

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

bool   GenericStatsUpdater::InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                            int64_t theC, int32_t theD, double decimalLength)
{
   const char *insertSQL = "insert into Candidate " \
                           "( candidateName, decimalLength, lastUpdateTime. candidateGroupId ) " \
                           "values ( ?,?,?.? )";

   if (!ip_CandidateLoader)
   {
      ip_CandidateLoader = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      ip_CandidateLoader->BindInputParameter(candidateName, NAME_LENGTH, true);
      ip_CandidateLoader->BindInputParameter(decimalLength);
      ip_CandidateLoader->BindInputParameter((int64_t) 1);
      ip_CandidateLoader->BindInputParameter(is_GroupName, NAME_LENGTH);
   }

   ip_CandidateLoader->SetInputParameterValue(candidateName, true);
   ip_CandidateLoader->SetInputParameterValue(1);
   ip_CandidateLoader->SetInputParameterValue((int64_t) time(NULL));
   ip_CandidateLoader->SetInputParameterValue(is_GroupName);

   return ip_CandidateLoader->Execute();
}
