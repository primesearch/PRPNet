#include "GFNStatsUpdater.h"
#include "SQLStatement.h"

// Update the groups stats.  This routine presumes that all other updates
// have been committed before calling this function.
bool  GFNStatsUpdater::RollupGroupStats(bool deleteInsert)
{
   SQLStatement  *sqlStatement;
   int32_t        theN;
   bool           success;
   const char    *deleteSQL = "delete from CandidateGroupStats";
   const char    *insertSQL = "insert into CandidateGroupStats (n) (select distinct n from Candidate)";
   const char    *selectSQL = "select n from CandidateGroupStats order by n";

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
   sqlStatement->BindSelectedColumn(&theN);

   while (sqlStatement->FetchRow(false))
   {
      if (!UpdateGroupStats(0, 0, theN, 0))
         return false;
      else
         ip_DBInterface->Commit();
   }

   sqlStatement->CloseCursor();
   delete sqlStatement;
   return true;
}

bool  GFNStatsUpdater::UpdateGroupStats(string candidateName)
{
   SQLStatement  *sqlStatement;
   int32_t        theN;
   bool           success;
   const char    *selectSQL = "select n from Candidate where CandidateName = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&theN);

   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   return UpdateGroupStats(0, 0, theN, 0);
}

bool  GFNStatsUpdater::UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC)
{
   SQLStatement *sqlStatement;
   bool          success;
   int32_t       nextToTest, leadingEdge;
   char          completedSQL[200];
   const char   *updateSQL = "update CandidateGroupStats " \
                             "   set CountInGroup = (select count(*) from Candidate " \
                             "                        where n = CandidateGroupStats.n), " \
                             "       CountUntested = (select count(*) from Candidate " \
                             "                         where n = CandidateGroupStats.n " \
                             "                           and CompletedTests = 0), " \
                             "       CountTested = (select count(*) from Candidate " \
                             "                       where n = CandidateGroupStats.n " \
                             "                         and CompletedTests > 0), " \
                             "       CountDoubleChecked = (select count(*) from Candidate " \
                             "                              where n = CandidateGroupStats.n " \
                             "                                and DoubleChecked = 1), " \
                             "       CountInProgress = (select count(*) from Candidate " \
                             "                           where n = CandidateGroupStats.n " \
                             "                             and HasPendingTest > 0), " \
                             "       MinInGroup = (select min(b) from Candidate " \
                             "                      where n = CandidateGroupStats.n), " \
                             "       MaxInGroup = (select max(b) from Candidate " \
                             "                      where n = CandidateGroupStats.n), " \
                             "       CompletedThru = %s, " \
                             "       LeadingEdge = ?, " \
                             "       PRPandPrimesFound = (select count(*) from Candidate " \
                             "                             where n = CandidateGroupStats.n " \
                             "                               and MainTestResult > 0) " \
                             " where n = ?";
   const char   *selectNTT = "select $null_func$(min(b), 0) from Candidate " \
                             " where n = ? and %s";
   const char   *selectLE  = "select $null_func$(max(b), 0) from Candidate " \
                             " where n = ? " \
                             "   and (CompletedTests > 0 or HasPendingTest = 1)";

   if (ib_NeedsDoubleCheck)
      sprintf(completedSQL, "DoubleChecked = 0");
   else
      sprintf(completedSQL, "CompletedTests = 0");

   // First, get the lowest value that has no completed tests
   // (or has not been double-checked).
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectNTT, completedSQL);
   sqlStatement->BindInputParameter(theN);
   sqlStatement->BindSelectedColumn(&nextToTest);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   // Second, get the highest value that has either a completed test or a pending test
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE);
   sqlStatement->BindInputParameter(theN);
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
      sprintf(completedSQL, "(select max(b) from Candidate where n = %d)", theN);
   else
      sprintf(completedSQL, "$null_func$((select max(b) from Candidate where n = %d and b < %d), %d)",
              theN, nextToTest, nextToTest);

   // Finally, update the group stats
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, completedSQL);
   sqlStatement->BindInputParameter(leadingEdge);
   sqlStatement->BindInputParameter(theN);

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

bool   GFNStatsUpdater::InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                        int32_t theC, double decimalLength)
{
   const char *insertSQL = "insert into Candidate " \
                           "( CandidateName, DecimalLength, b, n, c, LastUpdateTime ) " \
                           "values ( ?,?,?,?,?,? )";

   if (!ip_CandidateLoader)
   {
      ip_CandidateLoader = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      ip_CandidateLoader->BindInputParameter(candidateName, NAME_LENGTH, true);
      ip_CandidateLoader->BindInputParameter(decimalLength);
      ip_CandidateLoader->BindInputParameter(theB);
      ip_CandidateLoader->BindInputParameter(theN);
      ip_CandidateLoader->BindInputParameter(theC);
      ip_CandidateLoader->BindInputParameter((int64_t) 1);
   }

   ip_CandidateLoader->SetInputParameterValue(candidateName, true);
   ip_CandidateLoader->SetInputParameterValue(decimalLength);
   ip_CandidateLoader->SetInputParameterValue(theB);
   ip_CandidateLoader->SetInputParameterValue(theN);
   ip_CandidateLoader->SetInputParameterValue(theC);
   ip_CandidateLoader->SetInputParameterValue((int64_t) time(NULL));

   return ip_CandidateLoader->Execute();
}
