#include "WagstaffStatsUpdater.h"
#include "SQLStatement.h"

// Update the groups stats.  This routine presumes that all other updates
// have been committed before calling this function.
bool  WagstaffStatsUpdater::RollupGroupStats(bool deleteInsert)
{
   SQLStatement  *sqlStatement;
   int32_t        theC;
   bool           success;
   const char    *deleteSQL = "delete from CandidateGroupStats";
   const char    *insertSQL = "insert into CandidateGroupStats (c) (select distinct c from Candidate)";
   const char    *selectSQL = "select c from CandidateGroupStats order by c";

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

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindSelectedColumn(&theC);

   while (sqlStatement->FetchRow(false))
   {
      if (!UpdateGroupStats(0, 0, 0, theC))
         return false;
      else
         ip_DBInterface->Commit();
   }

   sqlStatement->CloseCursor();
   delete sqlStatement;
   return true;
}

bool  WagstaffStatsUpdater::UpdateGroupStats(string candidateName)
{
   SQLStatement  *sqlStatement;
   int32_t        theC;
   bool           success;
   const char    *selectSQL = "select c from Candidate where CandidateName = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&theC);

   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   return UpdateGroupStats(0, 0, 0, theC);
}

bool  WagstaffStatsUpdater::UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC)
{
   SQLStatement *sqlStatement;
   bool          success;
   int32_t       nextToTest, leadingEdge;
   char          completedSQL[200];
   const char   *updateSQL = "update CandidateGroupStats " \
                             "   set CountInGroup = (select count(*) from Candidate " \
                             "                        where c = CandidateGroupStats.c), " \
                             "       CountUntested = (select count(*) from Candidate " \
                             "                         where c = CandidateGroupStats.c " \
                             "                           and CompletedTests = 0), " \
                             "       CountTested = (select count(*) from Candidate " \
                             "                       where c = CandidateGroupStats.c " \
                             "                         and CompletedTests > 0), " \
                             "       CountDoubleChecked = (select count(*) from Candidate " \
                             "                              where c = CandidateGroupStats.c " \
                             "                                and DoubleChecked = 1), " \
                             "       CountInProgress = (select count(*) from Candidate " \
                             "                           where c = CandidateGroupStats.c " \
                             "                             and HasPendingTest > 0), " \
                             "       MinInGroup = (select min(n) from Candidate " \
                             "                      where c = CandidateGroupStats.c), " \
                             "       MaxInGroup = (select max(n) from Candidate " \
                             "                      where c = CandidateGroupStats.c), " \
                             "       CompletedThru = %s, " \
                             "       LeadingEdge = ?, " \
                             "       PRPandPrimesFound = (select count(*) from Candidate " \
                             "                             where c = CandidateGroupStats.c " \
                             "                               and MainTestResult > 0) " \
                             " where c = ?";
   const char   *selectNTT = "select $null_func$(min(n), 0) from Candidate " \
                             " where c = ? and %s";
   const char   *selectLE  = "select $null_func$(max(n), 0) from Candidate " \
                             " where c = ? " \
                             "   and (CompletedTests > 0 or HasPendingTest = 1)";

   if (ib_NeedsDoubleCheck)
      sprintf(completedSQL, "DoubleChecked = 0");
   else
      sprintf(completedSQL, "CompletedTests = 0");

   // First, get the lowest value that has no completed tests
   // (or has not been double-checked).
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectNTT, completedSQL);
   sqlStatement->BindInputParameter(theC);
   sqlStatement->BindSelectedColumn(&nextToTest);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   // Second, get the highest value that has either a completed test or a pending test
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE);
   sqlStatement->BindInputParameter(theC);
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
      sprintf(completedSQL, "(select max(n) from Candidate where c = %d)", theC);
   else
      sprintf(completedSQL, "$null_func$((select max(n) from Candidate where c = %d and n < %d), %d)",
              theC, nextToTest, nextToTest);

   // Finally, update the group stats
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, completedSQL);
   sqlStatement->BindInputParameter(leadingEdge);
   sqlStatement->BindInputParameter(theC);

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

bool   WagstaffStatsUpdater::InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                              int32_t theC, double decimalLength)
{
   const char *insertSQL = "insert into Candidate " \
                           "( CandidateName, DecimalLength, n, c, LastUpdateTime ) " \
                           "values ( ?,?,?,?,? )";

   if (!ip_CandidateLoader)
   {
      ip_CandidateLoader = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      ip_CandidateLoader->BindInputParameter(candidateName, NAME_LENGTH, true);
      ip_CandidateLoader->BindInputParameter(decimalLength);
      ip_CandidateLoader->BindInputParameter(theN);
      ip_CandidateLoader->BindInputParameter((int32_t) 1);
      ip_CandidateLoader->BindInputParameter((int64_t) 1);
   }

   ip_CandidateLoader->SetInputParameterValue(candidateName, true);
   ip_CandidateLoader->SetInputParameterValue(decimalLength);
   ip_CandidateLoader->SetInputParameterValue(theN);
   ip_CandidateLoader->SetInputParameterValue((int32_t) 1);
   ip_CandidateLoader->SetInputParameterValue((int64_t) time(NULL));

   return ip_CandidateLoader->Execute();
}
