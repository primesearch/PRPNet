#include "PrimeStatsUpdater.h"
#include "SQLStatement.h"

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
                           "                       from Candidate, CandidateTest " \
                           "                      where CandidateTest.UserID = UserStats.UserID " \
                           "                        and CandidateTest.CandidateName = Candidate.CandidateName)";

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
                           "                          where CandidateTest.TeamID = TeamStats.TeamID " \
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
