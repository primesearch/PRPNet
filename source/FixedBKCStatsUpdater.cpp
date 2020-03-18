#include "FixedBKCStatsUpdater.h"
#include "SQLStatement.h"

// Update the groups stats.  This routine presumes that all other updates
// have been committed before calling this function.
bool  FixedBKCStatsUpdater::RollupGroupStats(bool deleteInsert)
{
   SQLStatement  *sqlStatement;
   int64_t        theK;
   int32_t        theB, theC;
   bool           success;
   const char    *deleteSQL = "delete from CandidateGroupStats";
   const char    *insertSQL = "insert into CandidateGroupStats (b, k, c) (select distinct b, k, c from Candidate)";
   const char    *selectSQL = "select b, k, c from CandidateGroupStats order by b, k, c";

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
   sqlStatement->BindSelectedColumn(&theB);
   sqlStatement->BindSelectedColumn(&theK);
   sqlStatement->BindSelectedColumn(&theC);

   while (sqlStatement->FetchRow(false))
   {
      if (!UpdateGroupStats(theK, theB, 0, theC))
         return false;
      else
         ip_DBInterface->Commit();
   }

   sqlStatement->CloseCursor();
   delete sqlStatement;
   return true;
}

bool  FixedBKCStatsUpdater::UpdateGroupStats(string candidateName)
{
   SQLStatement  *sqlStatement;
   bool           success;
   int64_t        theK;
   int32_t        theB, theC;
   const char    *selectSQL = "select b, k, c from Candidate where CandidateName = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&theB);
   sqlStatement->BindSelectedColumn(&theK);
   sqlStatement->BindSelectedColumn(&theC);

   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   return UpdateGroupStats(theK, theB, 0, theC);
}

bool  FixedBKCStatsUpdater::UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC)
{
   SQLStatement *sqlStatement;
   bool          success;
   int32_t       nextToTest, leadingEdge;
   char          completedSQL[200];
   const char   *updateSQL = "update CandidateGroupStats " \
                             "   set CountInGroup = (select count(*) from Candidate " \
                             "                        where b = CandidateGroupStats.b " \
                             "                          and k = CandidateGroupStats.k " \
                             "                          and c = CandidateGroupStats.c), " \
                             "       CountUntested = (select count(*) from Candidate " \
                             "                         where b = CandidateGroupStats.b " \
                             "                           and k = CandidateGroupStats.k " \
                             "                           and c = CandidateGroupStats.c " \
                             "                           and CompletedTests = 0), " \
                             "       CountTested = (select count(*) from Candidate " \
                             "                       where b = CandidateGroupStats.b " \
                             "                         and k = CandidateGroupStats.k " \
                             "                         and c = CandidateGroupStats.c " \
                             "                         and CompletedTests > 0), " \
                             "       CountDoubleChecked = (select count(*) from Candidate " \
                             "                             where b = CandidateGroupStats.b " \
                             "                               and k = CandidateGroupStats.k " \
                             "                               and c = CandidateGroupStats.c " \
                             "                                and DoubleChecked = 1), " \
                             "       CountInProgress = (select count(*) from Candidate " \
                             "                           where b = CandidateGroupStats.b " \
                             "                             and k = CandidateGroupStats.k " \
                             "                             and c = CandidateGroupStats.c " \
                             "                             and HasPendingTest > 0), " \
                             "       MinInGroup = (select min(n) from Candidate " \
                             "                      where b = CandidateGroupStats.b " \
                             "                        and k = CandidateGroupStats.k " \
                             "                        and c = CandidateGroupStats.c), " \
                             "       MaxInGroup = (select max(n) from Candidate " \
                             "                      where b = CandidateGroupStats.b " \
                             "                        and k = CandidateGroupStats.k " \
                             "                        and c = CandidateGroupStats.c), " \
                             "       CompletedThru = %s, " \
                             "       LeadingEdge = ?, " \
                             "       PRPandPrimesFound = (select count(*) from Candidate " \
                             "                             where b = CandidateGroupStats.b " \
                             "                               and k = CandidateGroupStats.k " \
                             "                               and c = CandidateGroupStats.c " \
                             "                               and MainTestResult > 0) " \
                             " where b = ? and k = ? and c = ?";
   const char   *selectNTT = "select $null_func$(min(n), 0) from Candidate " \
                             " where b = ? and k = ? and c = ? and %s";
   const char   *selectLE  = "select $null_func$(max(n), 0) from Candidate " \
                             " where b = ? and k = ? and c = ? " \
                             "   and (CompletedTests > 0 or HasPendingTest = 1)";

   if (ib_NeedsDoubleCheck)
      sprintf(completedSQL, "DoubleChecked = 0");
   else
      sprintf(completedSQL, "CompletedTests = 0");

   // First, get the lowest value that has no completed tests
   // (or has not been double-checked).
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectNTT, completedSQL);
   sqlStatement->BindInputParameter(theB);
   sqlStatement->BindInputParameter(theK);
   sqlStatement->BindInputParameter(theC);
   sqlStatement->BindSelectedColumn(&nextToTest);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   // Second, get the highest value that has either a completed test or a pending test
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE);
   sqlStatement->BindInputParameter(theB);
   sqlStatement->BindInputParameter(theK);
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
      sprintf(completedSQL, "(select max(n) from Candidate where b = %d and k = %" PRId64" and c = %d)", theB, theK, theC);
   else
      sprintf(completedSQL, "$null_func$((select max(n) from Candidate where b = %d and k = %" PRId64" and c = %d and n < %d), %d)",
              theB, theK, theC, nextToTest, nextToTest);

   // Finally, update the group stats
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, completedSQL);
   sqlStatement->BindInputParameter(leadingEdge);
   sqlStatement->BindInputParameter(theB);
   sqlStatement->BindInputParameter(theK);
   sqlStatement->BindInputParameter(theC);

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

bool   FixedBKCStatsUpdater::InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                             int32_t theC, double decimalLength)
{
   const char *insertSQL = "insert into Candidate " \
                           "( CandidateName, DecimalLength, k, b, n, c, LastUpdateTime ) " \
                           "values ( ?,?,?,?,?,?,? )";

   if (!ip_CandidateLoader)
   {
      ip_CandidateLoader = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      ip_CandidateLoader->BindInputParameter(candidateName, NAME_LENGTH, true);
      ip_CandidateLoader->BindInputParameter(decimalLength);
      ip_CandidateLoader->BindInputParameter(theK);
      ip_CandidateLoader->BindInputParameter(theB);
      ip_CandidateLoader->BindInputParameter(theN);
      ip_CandidateLoader->BindInputParameter(theC);
      ip_CandidateLoader->BindInputParameter((int64_t) 1);
   }

   ip_CandidateLoader->SetInputParameterValue(candidateName, true);
   ip_CandidateLoader->SetInputParameterValue(decimalLength);
   ip_CandidateLoader->SetInputParameterValue(theK);
   ip_CandidateLoader->SetInputParameterValue(theB);
   ip_CandidateLoader->SetInputParameterValue(theN);
   ip_CandidateLoader->SetInputParameterValue(theC);
   ip_CandidateLoader->SetInputParameterValue((int64_t) time(NULL));

   return ip_CandidateLoader->Execute();
}
