#include "WWWWStatsUpdater.h"
#include "SQLStatement.h"

// This procedure is used to recompute the user stats based upon completed tests.
// This should only be used if the stats appear to be whacked.
bool  WWWWStatsUpdater::RollupUserStats(void)
{
   SQLStatement *sqlStatement;
   bool        success;
   const char *deleteSQL = "delete from UserStats";
   const char *insertSQL = "insert into UserStats (UserID) (select distinct UserID from WWWWRangeTest);";
   const char *updateSQL = "update UserStats " \
                           "   set TestsPerformed = (select count(*) from WWWWRangeTest " \
                           "                          where UserID = UserStats.UserID), " \
                           "       WWWWPrimes = (select count(*) from UserWWWWs " \
                           "                      where UserWWWWs.UserID = UserStats.UserID " \
                           "                        and Remainder = 0 and Quotient = 0), " \
                           "       NearWWWWPrimes = (select count(*) from UserWWWWs " \
                           "                          where UserWWWWs.UserID = UserStats.UserID " \
                           "                           and (Remainder <> 0 or Quotient <> 0)), " \
                           "       TotalScore = (select 1000 * count(*) from WWWWRangeTest " \
                           "                      where WWWWRangeTest.UserID = UserStats.UserID)";

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
bool  WWWWStatsUpdater::RollupTeamStats(void)
{
   SQLStatement *sqlStatement;
   bool        success;
   const char *deleteSQL = "delete from TeamStats";
   const char *insertSQL = "insert into TeamStats (TeamID) (select distinct TeamID from WWWWRangeTest)";
   const char *updateSQL = "update TeamStats " \
                           "   set TestsPerformed = (select count(*) from WWWWRangeTest " \
                           "                          where TeamID = TeamStats.TeamID), " \
                           "       WWWWPrimes = (select count(*) from UserWWWWs " \
                           "                      where UserWWWWs.TeamID = TeamStats.TeamID " \
                           "                        and Remainder = 0 and Quotient = 0), " \
                           "       NearWWWWPrimes = (select count(*) from UserWWWWs " \
                           "                          where UserWWWWs.TeamID = TeamStats.TeamID " \
                           "                           and (Remainder <> 0 or Quotient <> 0)), " \
                           "       TotalScore = (select 1000 * count(*) from WWWWRangeTest " \
                           "                      where WWWWRangeTest.TeamID = TeamStats.TeamID)";

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
bool  WWWWStatsUpdater::RollupUserTeamStats(void)
{
   SQLStatement *sqlStatement;
   bool        success;
   const char *deleteSQL = "delete from UserTeamStats";
   const char *insertSQL = "insert into UserTeamStats (UserID, TeamID) (select distinct UserID, TeamID from WWWWRangeTest)";
   const char *updateSQL = "update UserTeamStats " \
                           "   set TestsPerformed = (select count(*) from WWWWRangeTest " \
                           "                          where UserID = UserTeamStats.UserID " \
		                     "                            and TeamID = UserTeamStats.TeamID), " \
                           "       WWWWPrimes = (select count(*) from UserWWWWs " \
                           "                      where UserWWWWs.UserID = UserTeamStats.UserID " \
                           "                        and UserWWWWs.TeamID = UserTeamStats.TeamID " \
                           "                        and Remainder = 0 and Quotient = 0), " \
                           "       NearWWWWPrimes = (select count(*) from UserWWWWs " \
                           "                          where UserWWWWs.UserID = UserTeamStats.UserID " \
                           "                            and UserWWWWs.TeamID = UserTeamStats.TeamID " \
                           "                           and (Remainder <> 0 or Quotient <> 0)), " \
                           "       TotalScore = (select 1000 * count(*) from WWWWRangeTest " \
                           "                      where WWWWRangeTest.UserID = UserTeamStats.UserID "
                           "                        and WWWWRangeTest.TeamID = UserTeamStats.TeamID)";

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

// Update the groups stats.  This routine presumes that all other updates
// have been committed before calling this function.
bool  WWWWStatsUpdater::RollupGroupStats(bool deleteInsert)
{
   SQLStatement  *sqlStatement;
   bool           success;
   const char    *deleteSQL = "delete from WWWWGroupStats";
   const char    *insertSQL = "insert into WWWWGroupStats (CountInGroup) values (0)";

   if (deleteInsert)
   {
      // Delete all of the group stats, then re-insert, then do the nasty update SQL.
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
      success = sqlStatement->Execute();
      delete sqlStatement;

      if (!success) return false;

      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      success = sqlStatement->Execute();
      delete sqlStatement;

      if (!success) return false;

      ip_DBInterface->Commit();
   }

   return UpdateGroupStats();
}

bool  WWWWStatsUpdater::UpdateGroupStats(void)
{
   SQLStatement *sqlStatement;
   bool          success;
   int64_t       nextToTest, leadingEdge;
   char          completedSQL[200];
   const char   *updateSQL = "update WWWWGroupStats " \
                             "   set CountInGroup = (select count(*) from WWWWRange), " \
                             "       CountUntested = (select count(*) from WWWWRange " \
                             "                         where CompletedTests = 0), " \
                             "       CountTested = (select count(*) from WWWWRange " \
                             "                       where CompletedTests > 0), " \
                             "       CountDoubleChecked = (select count(*) from WWWWRange " \
                             "                             where DoubleChecked = 1), " \
                             "       CountInProgress = (select count(*) from WWWWRange " \
                             "                           where HasPendingTest > 0), " \
                             "       MinInGroup = (select min(LowerLimit) from WWWWRange), " \
                             "       MaxInGroup = (select max(UpperLimit) from WWWWRange), " \
                             "       CompletedThru = %s, " \
                             "       LeadingEdge = ?, " \
                             "       WWWWPrimes = (select count(*) from WWWWRangeTestResult " \
                             "                      where Duplicate = 0 and Remainder = 0 and Quotient = 0), " \
                             "       NearWWWWPrimes = (select count(*) from WWWWRangeTestResult " \
                             "                         where Duplicate = 0) ";
   const char   *selectNTT = "select $null_func$(min(LowerLimit), 0) from WWWWRange " \
                             " where %s ";
   const char   *selectLE  = "select $null_func$(max(UpperLimit), 0) from WWWWRange " \
                             " where (CompletedTests > 0 or HasPendingTest = 1) ";

   if (ib_NeedsDoubleCheck)
      sprintf(completedSQL, "DoubleChecked = 0");
   else
      sprintf(completedSQL, "CompletedTests = 0");

   // First, get the lowest value that has no completed tests
   // (or has not been double-checked).
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectNTT, completedSQL);
   sqlStatement->BindSelectedColumn(&nextToTest);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   // Second, get the highest value that has either a completed test or a pending test
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE);
   sqlStatement->BindSelectedColumn(&leadingEdge);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   // It is possible that the leading edge is actually the highest value for which
   // all lower values have a completed test (or  a double-check).  In that case,
   // set the leading edge the the next value that needs to be tested.
   if (leadingEdge < nextToTest)
      leadingEdge = nextToTest;

   // If there is nothing left to test, have the cursor select the max in the group
   // Otherwise, have it select it select the largest value in the group that has been
   // tested.  Note that the $null_func$ is needed in case only one candidate in the group
   // has been tested.  In that case it returns that candidate.
   if (nextToTest == 0)
      sprintf(completedSQL, "(select max(UpperLimit) from WWWWRange)");
   else
      sprintf(completedSQL, "$null_func$((select max(UpperLimit) from WWWWRange where UpperLimit < %" PRId64"), %" PRId64")",
              nextToTest, nextToTest);

   // Finally, update the group stats
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, completedSQL);
   sqlStatement->BindInputParameter(leadingEdge);

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

// Due to a completed test update any stats that need to be updated.
bool  WWWWStatsUpdater::UpdateStats(string   userID,
                                    string   teamID,
                                    int32_t  finds,
                                    int32_t  nearFinds)
{
   bool  success;

   success = UpdateUserStats(userID, finds, nearFinds);

   if (!success) return false;

   if (teamID != NO_TEAM)
   {
      success = UpdateUserTeamStats(userID, teamID, finds, nearFinds);

      if (!success) return false;

      success = UpdateTeamStats(teamID, finds, nearFinds);

      if (!success) return false;
   }

   // Done hourly unless we have a find
   if (finds + nearFinds > 0)
      return UpdateGroupStats();

   return success;
}

// Update the user stats for the completed test.  This routine presumes
// that all other updates have been committed before calling this function.
bool  WWWWStatsUpdater::UpdateUserStats(string   userID,
                                        int32_t  finds,
                                        int32_t  nearFinds)
{
   SQLStatement *sqlStatement;
   int32_t     rowCount;
   bool        success;
   const char *selectCount = "select count(*) from UserStats " \
                             " where UserID = ?";
   const char *insertSQL = "insert into UserStats " \
                           "( UserId, TestsPerformed, WWWWPrimes, NearWWWWPrimes, TotalScore ) " \
                           "values ( ?,1,?,?,? )";
   const char *updateSQL = "update UserStats " \
                           "   set TestsPerformed     = TestsPerformed + 1, " \
                           "       WWWWPrimes         = WWWWPrimes + ?, " \
                           "       NearWWWWPrimes     = NearWWWWPrimes + ?, " \
                           "       TotalScore         = TotalScore + ? " \
                           " where UserId             = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectCount);
   sqlStatement->BindInputParameter(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&rowCount);

   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success) return false;

   if (rowCount == 0)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
      sqlStatement->BindInputParameter(finds);
      sqlStatement->BindInputParameter(nearFinds);
      sqlStatement->BindInputParameter(1000);
   }
   else
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(finds);
      sqlStatement->BindInputParameter(nearFinds);
      sqlStatement->BindInputParameter(1000);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
   }

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

// Update the user team stats for the completed test.  This routine presumes
// that all other updates have been committed before calling this function.
bool  WWWWStatsUpdater::UpdateUserTeamStats(string   userID,
                                            string   teamID,
                                            int32_t  finds,
                                            int32_t  nearFinds)
{
   SQLStatement *sqlStatement;
   int32_t     rowCount;
   bool        success;
   const char *selectCount = "select count(*) from UserTeamStats " \
                             " where UserID = ? " \
		                       "   and TeamID = ?";
   const char *insertSQL = "insert into UserTeamStats " \
                           "( UserId, TeamID, TestsPerformed, WWWWPrimes, NearWWWWPrimes, TotalScore ) " \
                           "values ( ?,?,1,?,?,? )";
   const char *updateSQL = "update UserTeamStats " \
                           "   set TestsPerformed     = TestsPerformed + 1, " \
                           "       WWWWPrimes         = WWWWPrimes + ?, " \
                           "       NearWWWWPrimes     = NearWWWWPrimes + ?, " \
                           "       TotalScore         = TotalScore + ? " \
                           " where UserId             = ? " \
                           "   and TeamID             = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectCount);
   sqlStatement->BindInputParameter(userID, ID_LENGTH);
   sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&rowCount);

   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success) return false;

   if (rowCount == 0)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
      sqlStatement->BindInputParameter(finds);
      sqlStatement->BindInputParameter(nearFinds);
      sqlStatement->BindInputParameter(1000);
   }
   else
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(finds);
      sqlStatement->BindInputParameter(nearFinds);
      sqlStatement->BindInputParameter(1000);
      sqlStatement->BindInputParameter(userID, ID_LENGTH);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   }

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

// Update the team stats for the completed test.  This routine presumes
// that all other updates have been committed before calling this function.
bool  WWWWStatsUpdater::UpdateTeamStats(string   teamID,
                                        int32_t  finds,
                                        int32_t  nearFinds)
{
   SQLStatement *sqlStatement;
   int32_t     rowCount;
   bool        success;
   const char *selectCount = "select count(*) from TeamStats " \
                             " where TeamID = ?";
   const char *insertSQL = "insert into TeamStats " \
                           "( TeamID, TestsPerformed, WWWWPrimes, NearWWWWPrimes, TotalScore ) " \
                           "values ( ?,1,?,?,? )";
   const char *updateSQL = "update TeamStats " \
                           "   set TestsPerformed     = TestsPerformed + 1, " \
                           "       WWWWPrimes         = WWWWPrimes + ?, " \
                           "       NearWWWWPrimes     = NearWWWWPrimes + ?, " \
                           "       TotalScore         = TotalScore + ? " \
                           " where TeamID             = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectCount);
   sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&rowCount);

   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success) return false;

   if (rowCount == 0)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
      sqlStatement->BindInputParameter(finds);
      sqlStatement->BindInputParameter(nearFinds);
      sqlStatement->BindInputParameter(1000);
   }
   else
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(finds);
      sqlStatement->BindInputParameter(nearFinds);
      sqlStatement->BindInputParameter(1000);
      sqlStatement->BindInputParameter(teamID, ID_LENGTH);
   }

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}
