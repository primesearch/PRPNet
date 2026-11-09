#include "PrimeStatsUpdater.h"
#include "SQLStatement.h"

bool   PrimeStatsUpdater::SetCandidateGroupId(string candidateName, int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   string      candidateGroupId;
   const char* updateSQL = "update Candidate set candidateGroupid = ? where candidateName = ?";

   candidateGroupId = GenerateCandidateGroupId(theK, theB, theN, theC, theD);

   if (!ip_CandidateUpdater)
   {
      ip_CandidateUpdater = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      ip_CandidateUpdater->BindInputParameter(candidateGroupId, ID_LENGTH, true);
      ip_CandidateUpdater->BindInputParameter(candidateName, NAME_LENGTH, true);
   }

   ip_CandidateUpdater->SetInputParameterValue(candidateGroupId, true);
   ip_CandidateUpdater->SetInputParameterValue(candidateName);

   return ip_CandidateUpdater->Execute();
}

bool   PrimeStatsUpdater::InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD, double decimalLength)
{
   string      candidateGroupId;
   const char* insertSQL = "insert into Candidate " \
      "( CandidateName, candidateGroupId, DecimalLength, k, b, n, c, d, LastUpdateTime ) " \
      "values ( ?,?,?,?,?,?,?,?,? )";

   candidateGroupId = GenerateCandidateGroupId(theK, theB, theN, theC, theD);

   if (!ip_CandidateLoader)
   {
      ip_CandidateLoader = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      ip_CandidateLoader->BindInputParameter(candidateName, NAME_LENGTH, true);
      ip_CandidateLoader->BindInputParameter(candidateGroupId, ID_LENGTH, true);
      ip_CandidateLoader->BindInputParameter(decimalLength);
      ip_CandidateLoader->BindInputParameter(theK);
      ip_CandidateLoader->BindInputParameter(theB);
      ip_CandidateLoader->BindInputParameter(theN);
      ip_CandidateLoader->BindInputParameter(theC);
      ip_CandidateLoader->BindInputParameter(theD);
      ip_CandidateLoader->BindInputParameter((int64_t) 1);
   }

   ip_CandidateLoader->SetInputParameterValue(candidateName, true);
   ip_CandidateLoader->SetInputParameterValue(candidateGroupId);
   ip_CandidateLoader->SetInputParameterValue(decimalLength);
   ip_CandidateLoader->SetInputParameterValue(theK);
   ip_CandidateLoader->SetInputParameterValue(theB);
   ip_CandidateLoader->SetInputParameterValue(theN);
   ip_CandidateLoader->SetInputParameterValue(theC);
   ip_CandidateLoader->SetInputParameterValue(theD);
   ip_CandidateLoader->SetInputParameterValue((int64_t) time(NULL));

   return ip_CandidateLoader->Execute();
}


// Update the groups stats.  This routine presumes that all other updates
// have been committed before calling this function.
bool  PrimeStatsUpdater::RollupCandidateGroupStats(bool deleteInsert)
{
   SQLStatement* sqlStatement;
   bool           success;
   char           candidateGroupId[ID_LENGTH + 1];
   const char* deleteSQL = "delete from CandidateGroupStats";
   const char* selectSQL = "select candidateGroupId from CandidateGroupStats order by candidateGroupId";

   if (deleteInsert)
   {
      // Delete all of the group stats, then re-insert, then do the nasty update SQL.
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
      success = sqlStatement->Execute();
      delete sqlStatement;

      if (!success) return false;

      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, GetInsertSqlForCandidateGroupStats());
      success = sqlStatement->Execute();
      delete sqlStatement;

      if (!success) return false;

      ip_DBInterface->Commit();
   }

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindSelectedColumn(candidateGroupId, ID_LENGTH);

   while (sqlStatement->FetchRow(false))
   {
      if (!UpdateCandidateGroupStats(candidateGroupId))
         return false;
      else
         ip_DBInterface->Commit();
   }

   sqlStatement->CloseCursor();
   delete sqlStatement;
   return true;
}

bool  PrimeStatsUpdater::UpdateCandidateGroupStats(char* candidateGroupId)
{
   SQLStatement* sqlStatement;
   bool          success;
   int32_t       countInGroup, countUntested, countTested, countDoubleChecked;
   int32_t       countInProgress, prpsAndPrimesFound, minInGroup, maxInGroup;
   int32_t       completedThru, leadingEdge;
   char         *columnName = GetGroupingColumnForCandidateGroupStats();

   const char* selectSQL = 
      "select count(n), " \
      "       sum(case when completedTests = 0 then 1 else 0 end), " \
      "       sum(case when completedTests > 0 then 1 else 0 end), " \
      "       sum(case when doubleChecked  = 1 then 1 else 0 end), " \
      "       sum(case when hasPendingTest = 1 then 1 else 0 end), " \
      "       sum(case when mainTestResult > 0 then 1 else 0 end), " \
      "       min(%s), " \
      "       max(%s) " \
      "  from Candidate " \
      " where candidateGroupId = ?";

   const char* updateSQL = 
      "update CandidateGroupStats " \
      "   set countInGroup = ?, " \
      "       countUntested = ?, " \
      "       countTested = ?, " \
      "       countDoubleChecked = ?, " \
      "       countInProgress = ?, " \
      "       prpsAndPrimesFound = ?, " \
      "       minInGroup = ?, " \
      "       maxInGroup = ?, " \
      "       completedThru = ?, " \
      "       leadingEdge = ? " \
      " where candidateGroupId = ?";

   // Get the easy fields to compute
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, columnName, columnName);
   sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&countInGroup);
   sqlStatement->BindSelectedColumn(&countUntested);
   sqlStatement->BindSelectedColumn(&countTested);
   sqlStatement->BindSelectedColumn(&countDoubleChecked);
   sqlStatement->BindSelectedColumn(&countInProgress);
   sqlStatement->BindSelectedColumn(&prpsAndPrimesFound);
   sqlStatement->BindSelectedColumn(&minInGroup);
   sqlStatement->BindSelectedColumn(&maxInGroup);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   // Find the min n for which all n below it are tested.
   completedThru = GetCompletedThru(candidateGroupId, countInGroup, countUntested, countDoubleChecked);

   if (completedThru == 0)
   {
      completedThru = maxInGroup;
      leadingEdge = maxInGroup;
   }
   else
   {
      // Find the min n greater than the max n with a completed test.
      leadingEdge = GetLeadingEdge(candidateGroupId, maxInGroup, completedThru);
   }

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   sqlStatement->BindInputParameter(countInGroup);
   sqlStatement->BindInputParameter(countUntested);
   sqlStatement->BindInputParameter(countTested);
   sqlStatement->BindInputParameter(countDoubleChecked);
   sqlStatement->BindInputParameter(countInProgress);
   sqlStatement->BindInputParameter(prpsAndPrimesFound);
   sqlStatement->BindInputParameter(minInGroup);
   sqlStatement->BindInputParameter(maxInGroup);
   sqlStatement->BindInputParameter(completedThru);
   sqlStatement->BindInputParameter(leadingEdge);
   sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

int32_t  PrimeStatsUpdater::GetCompletedThru(char* candidateGroupId, int32_t countInGroup, int32_t countUntested, int32_t countDoubleChecked)
{
   SQLStatement* sqlStatement;
   bool          success;
   int32_t       completedThru = 0;
   char         *columnName = GetGroupingColumnForCandidateGroupStats();

   // First, get the lowest value that has no completed tests (or has not been double-checked).
   if (ib_NeedsDoubleCheck)
   {
      if (countInGroup > countDoubleChecked)
      {
         const char* selectNTT =
            "select $null_func$(min(%s), 0) from Candidate " \
            " where candidateGroupId = ? and doubleChecked = 0";

         sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectNTT, columnName);
         sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);
         sqlStatement->BindSelectedColumn(&completedThru);
         success = sqlStatement->FetchRow(true);
         delete sqlStatement;
      }
   }
   else
   {
      if (countUntested > 0)
      {
         const char* selectNTT =
            "select $null_func$(min(%s), 0) from Candidate " \
            " where candidateGroupId = ? and completedTests = 0";

         sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectNTT, columnName);
         sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);
         sqlStatement->BindSelectedColumn(&completedThru);
         success = sqlStatement->FetchRow(true);
         delete sqlStatement;
      }
   }

   return completedThru;
}

int32_t  PrimeStatsUpdater::GetLeadingEdge(char* candidateGroupId, int32_t maxInGroup, int32_t completedThru)
{
   SQLStatement* sqlStatement;
   bool          success;
   int32_t       maxCompleted = 0, leadingEdge = 0;
   char         *columnName = GetGroupingColumnForCandidateGroupStats();

   if (ib_NeedsDoubleCheck)
   {
      // Select the max value that has been double-checked.
      const char* selectLE =
         "select $null_func$(max(%s), ?) from Candidate " \
         " where candidateGroupId = ? and doubleChecked = 1";

      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE, columnName);
      sqlStatement->BindInputParameter(maxInGroup);
      sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);
      sqlStatement->BindSelectedColumn(&maxCompleted);
      success = sqlStatement->FetchRow(true);
      delete sqlStatement;

      if (!success) return false;
   }
   else
   {
      // Select the max value that has been tested.
      const char* selectLE =
         "select $null_func$(max(%s), ?) from Candidate " \
         " where candidateGroupId = ? and (completedTests = 1 or hasPendingTest = 1)";

      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE, columnName);
      sqlStatement->BindInputParameter(maxInGroup);
      sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);
      sqlStatement->BindSelectedColumn(&maxCompleted);
      success = sqlStatement->FetchRow(true);
      delete sqlStatement;

      if (!success) return false;
   }

   if (maxInGroup == maxCompleted)
      return maxCompleted;

   // Select the min value greater than the largest value greater that has been sent out.
   const char* selectLE =
      "select $null_func$(min(%s), ?) from Candidate " \
      " where candidateGroupId = ? and %s > ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE, columnName, columnName);
   sqlStatement->BindInputParameter(maxCompleted);
   sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);
   sqlStatement->BindInputParameter(maxCompleted);
   sqlStatement->BindSelectedColumn(&leadingEdge);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   return leadingEdge;
}

// This procedure is used to recompute the user stats based upon completed tests.
// This should only be used if the stats appear to be whacked.
bool  PrimeStatsUpdater::RollupUserStats(void)
{
   SQLStatement *sqlStatement;
   bool        success;
   const char *deleteSQL = "delete from UserStats";
   const char *insertSQL = "insert into UserStats (UserID) (select distinct UserID from CandidateTest);";
   const char *updateSQL = "update UserStats " \
                           "   set TestsPerformed = (select count(*) from CandidateTest, CandidateTestResult " \
                           "                          where CandidateTest.UserID = UserStats.UserID " \
                           "                            and CandidateTest.CandidateName = CandidateTestResult.CandidateName " \
                           "                            and CandidateTest.TestID = CandidateTestResult.TestID), " \
                           "       PRPsFound = (select count(*) from UserPrimes " \
                           "                     where UserPrimes.UserID = UserStats.UserID " \
                           "                       and TestResult = 1), " \
                           "       PrimesFound = (select count(*) from UserPrimes " \
                           "                       where UserPrimes.UserID = UserStats.UserID " \
                           "                         and TestResult = 2), " \
                           "       GFNDivisorsFound = (select count(*) from CandidateGFNDivisor " \
                           "                            where UserID = UserStats.UserID), " \
                           "       TotalScore = (select SUM((DecimalLength / 10000.0)* (DecimalLength / 10000.0)) " \
                           "                       from Candidate, CandidateTest, CandidateTestResult " \
                           "                      where CandidateTest.UserID = UserStats.UserID " \
                           "                        and CandidateTest.CandidateName = Candidate.CandidateName " \
                           "                        and CandidateTest.CandidateName = CandidateTestResult.CandidateName " \
                           "                        and CandidateTest.TestID = CandidateTestResult.TestID)";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   if (!success) return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   if (!success) return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   return success;
}

// This procedure is used to recompute the team stats based upon completed tests.
// This should only be used if the stats appear to be whacked.
bool  PrimeStatsUpdater::RollupTeamStats(void)
{
   SQLStatement *sqlStatement;
   bool        success;
   const char *deleteSQL = "delete from TeamStats";
   const char *insertSQL = "insert into TeamStats (TeamID) (select distinct TeamID from CandidateTest)";
   const char *updateSQL = "update TeamStats " \
                           "   set TestsPerformed = (select count(*) from CandidateTest, CandidateTestResult " \
                           "                          where CandidateTest.TeamID = TeamStats.TeamID " \
                           "                            and CandidateTest.CandidateName = CandidateTestResult.CandidateName " \
                           "                            and CandidateTest.TestID = CandidateTestResult.TestID), " \
                           "       PRPsFound = (select count(*) from UserPrimes " \
                           "                     where UserPrimes.TeamID = TeamStats.TeamID " \
                           "                       and TestResult = 1), " \
                           "       PrimesFound = (select count(*) from UserPrimes " \
                           "                       where UserPrimes.TeamID = TeamStats.TeamID " \
                           "                         and TestResult = 2), " \
                           "       GFNDivisorsFound = (select count(*) from CandidateGFNDivisor " \
                           "                            where TeamID = TeamStats.TeamID), " \
                           "       TotalScore = (select SUM((DecimalLength / 10000.0)* (DecimalLength / 10000.0)) " \
                           "                       from Candidate, CandidateTest, CandidateTestResult " \
                           "                      where CandidateTest.TeamID = TeamStats.TeamID " \
                           "                        and CandidateTest.CandidateName = Candidate.CandidateName " \
                           "                        and CandidateTest.CandidateName = CandidateTestResult.CandidateName " \
                           "                        and CandidateTest.TestID = CandidateTestResult.TestID)";


   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   if (!success) return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   if (!success) return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   return success;
}

// This procedure is used to recompute the user team stats based upon completed tests.
// This should only be used if the stats appear to be whacked.
bool  PrimeStatsUpdater::RollupUserTeamStats(void)
{
   SQLStatement *sqlStatement;
   bool        success;
   const char *deleteSQL = "delete from UserTeamStats";
   const char *insertSQL = "insert into UserTeamStats (UserID, TeamID) (select distinct UserID, TeamID from CandidateTest)";
   const char *updateSQL = "update UserTeamStats " \
                           "   set TestsPerformed = (select count(*) from CandidateTest, CandidateTestResult " \
                           "                          where CandidateTest.TeamID = UserTeamStats.TeamID " \
                           "                            and CandidateTest.CandidateName = CandidateTestResult.CandidateName " \
                           "                            and CandidateTest.TestID = CandidateTestResult.TestID), " \
                           "       PRPsFound = (select count(*) from UserPrimes " \
                           "                     where UserPrimes.TeamID = UserTeamStats.TeamID " \
                           "                       and UserPrimes.UserID = UserTeamStats.UserID " \
                           "                       and TestResult = 1), " \
                           "       PrimesFound = (select count(*) from UserPrimes " \
                           "                       where UserPrimes.TeamID = UserTeamStats.TeamID " \
                           "                         and UserPrimes.UserID = UserTeamStats.UserID " \
                           "                         and TestResult = 2), " \
                           "       GFNDivisorsFound = (select count(*) from CandidateGFNDivisor " \
                           "                            where UserID = UserTeamStats.UserID " \
                           "                              and TeamID = UserTeamStats.TeamID), " \
                           "       TotalScore = (select SUM((DecimalLength / 10000.0)* (DecimalLength / 10000.0)) " \
                           "                       from Candidate, CandidateTest, CandidateTestResult " \
                           "                      where CandidateTest.TeamID = UserTeamStats.TeamID " \
                           "                        and CandidateTest.CandidateName = Candidate.CandidateName " \
                           "                        and CandidateTest.CandidateName = CandidateTestResult.CandidateName " \
                           "                        and CandidateTest.TestID = CandidateTestResult.TestID)";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   if (!success) return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   if (!success) return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   success = sqlStatement->Execute();
   delete sqlStatement;

   return success;
}

// Due to a completed test update any stats that need to be updated.
bool  PrimeStatsUpdater::UpdateStats(string   userID,
                                     string   teamID,
                                     string   candidateName,
                                     double   decimalLength,
                                     int32_t  gfnDivisors,
                                     int32_t  numberOfTests,
                                     int32_t  numberOfPRPs,
                                     int32_t  numberOfPrimes)
{
   bool  success;

   success = UpdateUserStats(userID, decimalLength, gfnDivisors, numberOfTests, numberOfPRPs, numberOfPrimes);

   if (!success) return false;

   if (teamID != NO_TEAM)
   {
      success = UpdateUserTeamStats(userID, teamID, decimalLength, gfnDivisors, numberOfTests, numberOfPRPs, numberOfPrimes);

      if (!success) return false;

      success = UpdateTeamStats(teamID, decimalLength, gfnDivisors, numberOfTests, numberOfPRPs, numberOfPrimes);

      if (!success) return false;
   }

   return success;
}

// Update the user stats for the completed test.  This routine presumes
// that all other updates have been committed before calling this function.
bool  PrimeStatsUpdater::UpdateUserStats(string   userID,
                                         double   decimalLength,
                                         int32_t  gfnDivisors,
                                         int32_t  numberOfTests,
                                         int32_t  numberOfPRPs,
                                         int32_t  numberOfPrimes)
{
   SQLStatement *sqlStatement;
   double      testScore;
   int32_t     rowCount;
   bool        success;
   const char *selectCount = "select count(*) from UserStats " \
                             " where UserID = ?";
   const char *insertSQL = "insert into UserStats " \
                           "( UserId, TestsPerformed, PRPsFound, PrimesFound, GFNDivisorsFound, TotalScore ) " \
                           "values ( ?,?,?,?,?,? )";
   const char *updateSQL = "update UserStats " \
                           "   set TestsPerformed     = TestsPerformed + ?, " \
                           "       PRPsFound          = PRPsFound + ?, " \
                           "       PrimesFound        = PrimesFound + ?, " \
                           "       GFNDivisorsFound   = GFNDivisorsFound + ?, " \
                           "       TotalScore         = TotalScore + ? " \
                           " where UserId             = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectCount);
   sqlStatement->BindInputParameter(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&rowCount);

   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success) return false;

   testScore = (double) decimalLength;
   testScore /= 10000.0;
   testScore *= testScore;
   testScore *= numberOfTests;

   if (rowCount == 0)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
      sqlStatement->BindInputParameter(numberOfTests);
      sqlStatement->BindInputParameter(numberOfPRPs);
      sqlStatement->BindInputParameter(numberOfPrimes);
      sqlStatement->BindInputParameter(gfnDivisors);
      sqlStatement->BindInputParameter(testScore);
   }
   else
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(numberOfTests);
      sqlStatement->BindInputParameter(numberOfPRPs);
      sqlStatement->BindInputParameter(numberOfPrimes);
      sqlStatement->BindInputParameter(gfnDivisors);
      sqlStatement->BindInputParameter(testScore);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
   }

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

// Update the user team stats for the completed test.  This routine presumes
// that all other updates have been committed before calling this function.
bool  PrimeStatsUpdater::UpdateUserTeamStats(string   userID,
                                             string   teamID,
                                             double   decimalLength,
                                             int32_t  gfnDivisors,
                                             int32_t  numberOfTests,
                                             int32_t  numberOfPRPs,
                                             int32_t  numberOfPrimes)
{
   SQLStatement *sqlStatement;
   double      testScore;
   int32_t     rowCount;
   bool        success;
   const char *selectCount = "select count(*) from UserTeamStats " \
                             " where UserID = ? " \
		                       "   and TeamID = ?";
   const char *insertSQL = "insert into UserTeamStats " \
                           "( UserId, TeamID, TestsPerformed,PRPsFound, PrimesFound, GFNDivisorsFound, TotalScore ) " \
                           "values ( ?,?,?,?,?,?,? )";
   const char *updateSQL = "update UserTeamStats " \
                           "   set TestsPerformed     = TestsPerformed + ?, " \
                           "       PRPsFound          = PRPsFound + ?, " \
                           "       PrimesFound        = PrimesFound + ?, " \
                           "       GFNDivisorsFound   = GFNDivisorsFound + ?, " \
                           "       TotalScore         = TotalScore + ? " \
                           " where UserId             = ? " \
                           "   and TeamID             = ?";

   if (teamID.length() == 0) return true;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectCount);
   sqlStatement->BindInputParameter(userID, ID_LENGTH);
   sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&rowCount);

   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success) return false;

   testScore = (double) decimalLength;
   testScore /= 10000.0;
   testScore *= testScore;
   testScore *= numberOfTests;

   if (rowCount == 0)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
      sqlStatement->BindInputParameter(numberOfTests);
      sqlStatement->BindInputParameter(numberOfPRPs);
      sqlStatement->BindInputParameter(numberOfPrimes);
      sqlStatement->BindInputParameter(gfnDivisors);
      sqlStatement->BindInputParameter(testScore);
   }
   else
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(numberOfTests);
      sqlStatement->BindInputParameter(numberOfPRPs);
      sqlStatement->BindInputParameter(numberOfPrimes);
      sqlStatement->BindInputParameter(gfnDivisors);
      sqlStatement->BindInputParameter(testScore);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   }

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

// Update the team stats for the completed test.  This routine presumes
// that all other updates have been committed before calling this function.
bool  PrimeStatsUpdater::UpdateTeamStats(string   teamID,
                                         double   decimalLength,
                                         int32_t  gfnDivisors,
                                         int32_t  numberOfTests,
                                         int32_t  numberOfPRPs,
                                         int32_t  numberOfPrimes)
{
   SQLStatement *sqlStatement;
   double      testScore;
   int32_t     rowCount;
   bool        success;
   const char *selectCount = "select count(*) from TeamStats " \
                             " where TeamID = ?";
   const char *insertSQL = "insert into TeamStats " \
                           "( TeamID, TestsPerformed, PRPsFound, PrimesFound, GFNDivisorsFound, TotalScore ) " \
                           "values ( ?,?,?,?,?,? )";
   const char *updateSQL = "update TeamStats " \
                           "   set TestsPerformed     = TestsPerformed + ?, " \
                           "       PRPsFound          = PRPsFound + ?, " \
                           "       PrimesFound        = PrimesFound + ?, " \
                           "       GFNDivisorsFound   = GFNDivisorsFound + ?, " \
                           "       TotalScore         = TotalScore + ? " \
                           " where TeamID             = ?";

   if (teamID.length() == 0) return true;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectCount);
   sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&rowCount);

   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success) return false;

   testScore = (double) decimalLength;
   testScore /= 10000.0;
   testScore *= testScore;
   testScore *= numberOfTests;

   if (rowCount == 0)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
      sqlStatement->BindInputParameter(numberOfTests);
      sqlStatement->BindInputParameter(numberOfPRPs);
      sqlStatement->BindInputParameter(numberOfPrimes);
      sqlStatement->BindInputParameter(gfnDivisors);
      sqlStatement->BindInputParameter(testScore);
   }
   else
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(numberOfTests);
      sqlStatement->BindInputParameter(numberOfPRPs);
      sqlStatement->BindInputParameter(numberOfPrimes);
      sqlStatement->BindInputParameter(gfnDivisors);
      sqlStatement->BindInputParameter(testScore);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   }

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}
