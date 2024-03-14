#include "FixedBNCStatsUpdater.h"
#include "SQLStatement.h"

// Update the groups stats.  This routine presumes that all other updates
// have been committed before calling this function.
bool  FixedBNCStatsUpdater::RollupGroupStats(bool deleteInsert)
{
   SQLStatement  *sqlStatement;
   int32_t        theB, theN, theC, theD;
   bool           success;
   const char    *deleteSQL = "delete from CandidateGroupStats";
   const char    *insertSQL = "insert into CandidateGroupStats (b, n, c, d) (select distinct b, n, c, d from Candidate)";
   const char    *selectSQL = "select b, n, c, d from CandidateGroupStats order by b, n, c, d";

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
   sqlStatement->BindSelectedColumn(&theN);
   sqlStatement->BindSelectedColumn(&theC);
   sqlStatement->BindSelectedColumn(&theD);

   while (sqlStatement->FetchRow(false))
   {
      if (!UpdateGroupStats(0, theB, theN, theC, theD))
         return false;
      else
         ip_DBInterface->Commit();
   }

   sqlStatement->CloseCursor();
   delete sqlStatement;
   return true;
}

bool  FixedBNCStatsUpdater::UpdateGroupStats(string candidateName)
{
   SQLStatement  *sqlStatement;
   bool           success;
   int32_t        theB, theN, theC, theD;
   const char    *selectSQL = "select b, n, c, d from Candidate where CandidateName = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&theB);
   sqlStatement->BindSelectedColumn(&theN);
   sqlStatement->BindSelectedColumn(&theC);
   sqlStatement->BindSelectedColumn(&theD);

   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   return UpdateGroupStats(0, theB, theN, theC, theD);
}

bool  FixedBNCStatsUpdater::UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC, int32_t theD)
{
   SQLStatement *sqlStatement;
   bool          success;
   int32_t       nextToTest, leadingEdge;
   char          completedSQL[200];
   const char   *updateSQL = "update CandidateGroupStats " \
                             "   set CountInGroup = (select count(*) from Candidate " \
                             "                        where b = CandidateGroupStats.b " \
                             "                          and n = CandidateGroupStats.n " \
                             "                          and c = CandidateGroupStats.c " \
                             "                          and d = CandidateGroupStats.d), " \
                             "       CountUntested = (select count(*) from Candidate " \
                             "                         where b = CandidateGroupStats.b " \
                             "                           and n = CandidateGroupStats.n " \
                             "                           and c = CandidateGroupStats.c " \
                             "                           and d = CandidateGroupStats.d " \
                             "                           and CompletedTests = 0), " \
                             "       CountTested = (select count(*) from Candidate " \
                             "                       where b = CandidateGroupStats.b " \
                             "                         and n = CandidateGroupStats.n " \
                             "                         and c = CandidateGroupStats.c " \
                             "                         and d = CandidateGroupStats.d " \
                             "                         and CompletedTests > 0), " \
                             "       CountDoubleChecked = (select count(*) from Candidate " \
                             "                             where b = CandidateGroupStats.b " \
                             "                               and n = CandidateGroupStats.n " \
                             "                               and c = CandidateGroupStats.c " \
                             "                               and d = CandidateGroupStats.d " \
                             "                                and DoubleChecked = 1), " \
                             "       CountInProgress = (select count(*) from Candidate " \
                             "                           where b = CandidateGroupStats.b " \
                             "                             and n = CandidateGroupStats.n " \
                             "                             and c = CandidateGroupStats.c " \
                             "                             and d = CandidateGroupStats.d " \
                             "                             and HasPendingTest > 0), " \
                             "       MinInGroup = (select min(k) from Candidate " \
                             "                      where b = CandidateGroupStats.b " \
                             "                        and n = CandidateGroupStats.n " \
                             "                        and c = CandidateGroupStats.c " \
                             "                        and d = CandidateGroupStats.d), " \
                             "       MaxInGroup = (select max(k) from Candidate " \
                             "                      where b = CandidateGroupStats.b " \
                             "                        and n = CandidateGroupStats.n " \
                             "                        and c = CandidateGroupStats.c " \
                             "                        and d = CandidateGroupStats.d), " \
                             "       CompletedThru = %s, " \
                             "       LeadingEdge = ?, " \
                             "       PRPandPrimesFound = (select count(*) from Candidate " \
                             "                             where b = CandidateGroupStats.b " \
                             "                               and n = CandidateGroupStats.n " \
                             "                               and c = CandidateGroupStats.c " \
                             "                               and d = CandidateGroupStats.d " \
                             "                               and MainTestResult > 0) " \
                             " where b = ? and n = ? and c = ? and d = ?";
   const char   *selectNTT = "select $null_func$(min(n), 0) from Candidate " \
                             " where b = ? and n = ? and c = ? and d = ? and %s";
   const char   *selectLE  = "select $null_func$(max(n), 0) from Candidate " \
                             " where b = ? and n = ? and c = ? and d = ? " \
                             "   and (CompletedTests > 0 or HasPendingTest = 1)";

   if (ib_NeedsDoubleCheck)
      snprintf(completedSQL, sizeof(completedSQL), "DoubleChecked = 0");
   else
      snprintf(completedSQL, sizeof(completedSQL), "CompletedTests = 0");

   // First, get the lowest value that has no completed tests
   // (or has not been double-checked).
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectNTT, completedSQL);
   sqlStatement->BindInputParameter(theB);
   sqlStatement->BindInputParameter(theN);
   sqlStatement->BindInputParameter(theC);
   sqlStatement->BindInputParameter(theD);
   sqlStatement->BindSelectedColumn(&nextToTest);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) return false;

   // Second, get the highest value that has either a completed test or a pending test
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectLE);
   sqlStatement->BindInputParameter(theB);
   sqlStatement->BindInputParameter(theN);
   sqlStatement->BindInputParameter(theC);
   sqlStatement->BindInputParameter(theD);
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
      snprintf(completedSQL, sizeof(completedSQL), "(select max(n) from Candidate where b = %d and n = %d and c = %d and d = %d)", theB, theN, theC, theD);
   else
      snprintf(completedSQL, sizeof(completedSQL), "$null_func$((select max(n) from Candidate where b = %d and n = %d and c = %d and d = %d and n < %d), %d)",
              theB, theN, theC, theD, nextToTest, nextToTest);

   // Finally, update the group stats
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, completedSQL);
   sqlStatement->BindInputParameter(leadingEdge);
   sqlStatement->BindInputParameter(theB);
   sqlStatement->BindInputParameter(theN);
   sqlStatement->BindInputParameter(theC);
   sqlStatement->BindInputParameter(theD);

   success = sqlStatement->Execute();

   delete sqlStatement;

   return success;
}

bool   FixedBNCStatsUpdater::InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                             int32_t theC, int32_t theD, double decimalLength)
{
   const char *insertSQL = "insert into Candidate " \
                           "( CandidateName, DecimalLength, k, b, n, c, d, LastUpdateTime ) " \
                           "values ( ?,?,?,?,?,?,?,? )";

   if (!ip_CandidateLoader)
   {
      ip_CandidateLoader = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      ip_CandidateLoader->BindInputParameter(candidateName, NAME_LENGTH, true);
      ip_CandidateLoader->BindInputParameter(decimalLength);
      ip_CandidateLoader->BindInputParameter(theK);
      ip_CandidateLoader->BindInputParameter(theB);
      ip_CandidateLoader->BindInputParameter(theN);
      ip_CandidateLoader->BindInputParameter(theC);
      ip_CandidateLoader->BindInputParameter(theD);
      ip_CandidateLoader->BindInputParameter((int64_t) 1);
   }

   ip_CandidateLoader->SetInputParameterValue(candidateName, true);
   ip_CandidateLoader->SetInputParameterValue(decimalLength);
   ip_CandidateLoader->SetInputParameterValue(theK);
   ip_CandidateLoader->SetInputParameterValue(theB);
   ip_CandidateLoader->SetInputParameterValue(theN);
   ip_CandidateLoader->SetInputParameterValue(theC);
   ip_CandidateLoader->SetInputParameterValue(theD);
   ip_CandidateLoader->SetInputParameterValue((int64_t) time(NULL));

   return ip_CandidateLoader->Execute();
}
