#include "PrimeWorkReceiver.h"
#include "PrimeStatsUpdater.h"
#include "SierpinskiRieselStatsUpdater.h"

PrimeWorkReceiver::PrimeWorkReceiver(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                                     string userID, string emailID,
                                     string machineID, string instanceID, string teamID)
                                     : WorkReceiver(dbInterface, theSocket, globals,
                                                    userID, emailID, machineID, instanceID, teamID)
{
   ib_OneKPerInstance = globals->b_OneKPerInstance;
}

PrimeWorkReceiver::~PrimeWorkReceiver()
{
}

void  PrimeWorkReceiver::ProcessMessage(string theMessage)
{
   char    *readBuf;
   char     tempMessage[BUFFER_SIZE];
   int32_t  testsReturned, testsAccepted, ignoreBad;

   strcpy(tempMessage, theMessage.c_str());
   if (sscanf(tempMessage, "RETURNWORK %s", is_ClientVersion) != 1)
   {
      ip_Socket->Send("ERROR: RETURNWORK error.  The client version is not specified");
      return;
   }
    
   testsReturned = testsAccepted = 0;
   ignoreBad = false;
   readBuf = ip_Socket->Receive();
   while (readBuf)
   {
      if (!memcmp(readBuf, "End of Message", 14))
         break;

      if (!memcmp(readBuf, "WorkUnit: ", 10))
      {
         ignoreBad = false;
         testsReturned++;

         if (ReceiveWorkUnit(readBuf))
         {
            testsAccepted++;
            ip_DBInterface->Commit();
         }
         else
         {
            ignoreBad = true;
            ip_DBInterface->Rollback();
         }
      }
      else
      {
         if (!ignoreBad)
            ip_Socket->Send("ERROR: ReceiveCompletedWork error.  Message [%s] cannot be parsed", readBuf);
      }

      readBuf = ip_Socket->Receive();
   }

   if (!testsReturned)
      ip_Socket->Send("INFO: There were no identifiable test results");
   else
   {
      if (testsReturned == testsAccepted)
         ip_Socket->Send("INFO: All %d test results were accepted", testsReturned);
      else
         ip_Socket->Send("INFO: %d of %d test results were accepted", testsAccepted, testsReturned);
   }

   ip_Socket->Send("End of Message");
}

int32_t  PrimeWorkReceiver::ReceiveWorkUnit(string theMessage)
{
   char     candidateName[NAME_LENGTH+1];
   char     tempMessage[BUFFER_SIZE];
   int64_t  testID;
   int32_t  isCompleted, returnCode, genericDecimalLength = -1;
   bool     success;
   SQLStatement  *sqlStatement;
   const char    *selectSQL = "select IsCompleted from CandidateTest " \
                              " where CandidateName = ? " \
                              "   and TestID = ?";
   
   strcpy(tempMessage, theMessage.c_str());
   if (ii_ServerType == ST_GENERIC)
   {
      if (sscanf(tempMessage, "WorkUnit: %s %" PRId64" %d", candidateName, &testID, &genericDecimalLength) != 3)
      {
         ip_Socket->Send("ERROR: Could not parse WorkUnit [%s]", theMessage.c_str());
         return false;
      }
   }
   else
   {
      if (sscanf(tempMessage, "WorkUnit: %s %" PRId64"", candidateName, &testID) != 2)
      {
         ip_Socket->Send("ERROR: Could not parse WorkUnit [%s]", theMessage.c_str());
         return false;
      }
   }

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindInputParameter(testID);
   sqlStatement->BindSelectedColumn(&isCompleted);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   // Verify whether or not the test exists.  It might have expired or the client
   // might be trying to send bad results.
   if (!success)
   {
      // Send INFO rather than ERROR.  Outside of an SQL error, this can happen if
      // the server expired the test or if the client reporting to the wrong server.
      // The client has checks for the latter and SQL errors are extremely unlikely,
      // so this most likely will happen because the test had expired.
      ip_Socket->Send("INFO: Test for %s was ignored.  Candidate and/or test was not found", candidateName);
      ip_Log->LogMessage("%s (%s %s): Test %" PRId64" for candidate %s was not found",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(),testID, candidateName);
      return false;
   }

   // Verify whether or not the test has already been reported as completed.
   if (isCompleted == 1)
   {
      ip_Socket->Send("INFO: Test for %s was accepted", candidateName);
      ip_Log->LogMessage("%s (%s %s): Test for %s was already reported",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
      return false;
   }

   returnCode = ReceiveWorkUnit(candidateName, testID, genericDecimalLength, is_ClientVersion);

   switch (returnCode)
   {
      case CT_BAD_TERMINATOR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The message terminater was truncated", candidateName);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to truncated terminator",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
         return false;

      case CT_WRONG_TERMINATOR:     
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The message terminater was incorrect", candidateName);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to an incorrect terminator",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
         return false;

      case CT_MISSING_TERMINATOR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The message terminater was missing", candidateName);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to missing terminator",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
         return false;

      case CT_SQL_ERROR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The server encounted an SQL error", candidateName);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to SQL error",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
         return false;

      case CT_PARSE_ERROR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The server could not parse the message", candidateName);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to a parsing error",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
         return false;

      case CT_OBSOLETE:
         ip_Socket->Send("INFO:  Test for %s was ignored.  Your application and/or prpclient is obsolete and MUST be upgraded", candidateName);
         ip_Log->LogMessage("%s (%s %s): Ignored test on %s due to obsolete or unknown application version",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
         return true;

      case CT_ABANDONED:
         ip_Socket->Send("INFO: Test for %s was abandoned", candidateName);
         ip_Log->LogMessage("Test for %s was abandoned by %s, machine %s, instance %s",
                            candidateName, is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
         return true;

      case CT_SUCCESS:
         ip_Socket->Send("INFO: Test for %s was accepted", candidateName);
         return true;

      default:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The server encounted an unknown error", candidateName);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to unknown error",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), candidateName);
         return false;
   }

   return false;
}

int32_t  PrimeWorkReceiver::ReceiveWorkUnit(string candidateName, int64_t testID, int32_t genericDecimalLength, string clientVersion)
{
   int32_t  ii, returnCode;

   ii_TestResults = 0;

   returnCode = ProcessWorkUnit(candidateName, testID, genericDecimalLength, clientVersion);

   for (ii=0; ii<ii_TestResults; ii++)
      delete ip_TestResult[ii];

   if (returnCode == CT_SUCCESS || returnCode == CT_ABANDONED)
      ip_DBInterface->Commit();
   else
      ip_DBInterface->Rollback();

   return returnCode;
}

int32_t  PrimeWorkReceiver::ProcessWorkUnit(string candidateName, int64_t testID, int32_t genericDecimalLength, string clientVersion)
{
   char     *theMessage, theName[50];
   int64_t   endTestID;
   int32_t   gfnDivisorCount = 0;
   bool      abandoned = false, obsolete = false, hasChild = false, doubleChecked, wasParsed, success;
   int32_t   gotTerminator, completedTests;
   result_t  mainTestResult;
   int32_t   ii;
   double    decimalLength;
   SQLStatement *sqlStatement;
   const char *selectSQL = "select CompletedTests, DecimalLength " \
                           "  from Candidate " \
                           " where CandidateName = ?";
   const char *updateSQL = "update Candidate " \
                           "   set HasPendingTest = 0, " \
                           "       CompletedTests = CompletedTests + 1, " \
                           "       MainTestResult = ?, " \
                           "       DoubleChecked = ?, " \
                           "       DecimalLength = ?, " \
                           "       LastUpdateTime = ? " \
                           " where CandidateName = ?";
   const char *updateTestSQL = "update CandidateTest " \
                               "   set IsCompleted = 1, " \
                               "       TeamID = ? " \
                               " where CandidateName = ?" \
                               "   and TestID = ?";

   gotTerminator = false;

   theMessage = ip_Socket->Receive(1);
   while (theMessage)
   {
      // Not only must the "End Of WorkUnit" message exist, but it must be for
      // the same candidate and test.  If not, then it will be rejected.  This
      // can happen because the socket sometimes receives large messages in
      // chunks.  One of more of the components of the message for the workunit
      // will be missing.  We cannot accept the test if that happens.
      if (!memcmp(theMessage, "End of WorkUnit", 15))
      {
         // Versions prior to 3.1.0 do not send a colon.  If an old client
         // connects, it is possible for the server to record the wrong
         // result for the test.
         if (strstr(theMessage, ":"))
         {
            if (sscanf(theMessage, "End of WorkUnit: %s %" PRId64"", theName, &endTestID) != 2)
               return CT_BAD_TERMINATOR;

            if (strcmp(theName, candidateName.c_str()) || endTestID != testID)
               return CT_WRONG_TERMINATOR;
         }

         gotTerminator = true;
         break;
      }

      if (!strcmp(theMessage, "Test Abandoned"))
         abandoned = true;
      else if (!memcmp(theMessage, "End Child ", 9))
         hasChild = false;
      else
      {
         if (!memcmp(theMessage, "Start Child ", 11))
         {
            ip_TestResult[ii_TestResults] = new CandidateTestResult(ip_DBInterface, ip_Log, candidateName,
                                                                    testID, ii_TestResults, ib_BriefTestLog,
                                                                    is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
            ii_TestResults++;
            hasChild = true;
         }

         if (hasChild)
         {
            wasParsed = ip_TestResult[ii_TestResults-1]->ProcessChildMessage(theMessage);
            if (BadProgramVersion(ip_TestResult[ii_TestResults-1]->GetProgramVersion()))
            {
               ip_Log->LogMessage("%d: Obsolete version (%s) of program (%s) used: [%s]", ip_Socket->GetSocketID(), ip_TestResult[ii_TestResults-1]->GetProgramVersion().c_str(), ip_TestResult[ii_TestResults-1]->GetProgram().c_str(), theMessage);
               obsolete = true;
            }
         }
         else
            wasParsed = ip_TestResult[ii_TestResults-1]->ProcessMessage(theMessage);

         if (!wasParsed)
         {
            ip_Log->LogMessage("%d: Could not parse [%s]", ip_Socket->GetSocketID(), theMessage);
            return CT_PARSE_ERROR;
         }
      }

      theMessage = ip_Socket->Receive();
   }

   // Something was messed up in transmission.  We cannot accept the results
   if (!gotTerminator)
      return CT_MISSING_TERMINATOR;

   // Delete the test if abandoned or if the client has round-off errors
   // Or if the task was done by an application version we don't trust
   if (abandoned || ip_TestResult[0]->HadRoundOffError() || obsolete)
   {
      if (AbandonTest(candidateName, testID))
         return (abandoned || ip_TestResult[0]->HadRoundOffError()) ? CT_ABANDONED : CT_OBSOLETE;
      else
         return CT_SQL_ERROR;
   }

   // If any updates/inserts failed up to this point, get out
   for (ii=0; ii<ii_TestResults; ii++)
      if (ip_TestResult[ii]->HadSQLError())
         return CT_SQL_ERROR;

   // Get some information about the candidate before continuing
   completedTests = 0;
   decimalLength = 0.0;
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&completedTests);
   sqlStatement->BindSelectedColumn(&decimalLength);
   sqlStatement->FetchRow(true);
   delete sqlStatement;
   completedTests++;

   doubleChecked = CheckDoubleCheck(candidateName, ip_TestResult[0]->GetResidue());
   mainTestResult = ip_TestResult[0]->GetTestResult();

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   sqlStatement->BindInputParameter(mainTestResult);
   sqlStatement->BindInputParameter(doubleChecked);
   if (ii_ServerType == ST_GENERIC)
      sqlStatement->BindInputParameter((double) genericDecimalLength);
   else
      sqlStatement->BindInputParameter(decimalLength);

   sqlStatement->BindInputParameter((int64_t) time(NULL));
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);

   // Update the Candidate table to indicate that we have another completed test
   success = sqlStatement->Execute();

   delete sqlStatement;

   if (!success) return CT_SQL_ERROR;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateTestSQL);
   sqlStatement->BindInputParameter(is_TeamID, ID_LENGTH);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindInputParameter(testID);

   // Update the CandidateTest table to indicate that this particular test is completed
   success = sqlStatement->Execute();

   delete sqlStatement;

   if (!success)  return CT_SQL_ERROR;

   for (ii=0; ii<ii_TestResults; ii++)
   {
      if (ii == 0)
         ip_TestResult[ii]->LogResults(ip_Socket->GetSocketID(), completedTests, ib_NeedsDoubleCheck, ib_ShowOnWebPage, decimalLength);
      else
         ip_TestResult[ii]->LogResults(ip_Socket->GetSocketID(), ip_TestResult[0], ib_ShowOnWebPage, decimalLength);

      if (ip_TestResult[ii]->HadSQLError())
         return CT_SQL_ERROR;

      gfnDivisorCount += ip_TestResult[ii]->GetGFNDivisorCount();
   }

   success = UpdateGroupStats(candidateName, mainTestResult);

   if (!success) return CT_SQL_ERROR;

   success = ((PrimeStatsUpdater *) ip_StatsUpdater)->UpdateStats(is_UserID, is_TeamID, candidateName, decimalLength, mainTestResult, gfnDivisorCount);

   if (!success) return CT_SQL_ERROR;

   return CT_SUCCESS;
}

// Determine if this residue matches another residue for the candidate.  Note that
// this check is done BEFORE the test results for this test are inserted into the database.
bool     PrimeWorkReceiver::CheckDoubleCheck(string candidateName, string residue)
{
   int32_t   matchingResidueCount = 0;
   SQLStatement *selectStatement;
   char      withPeriod[50];
   const char *selectSQL = "select count(*) from CandidateTestResult " \
                           " where CandidateName = ? " \
                           "   and WhichTest = 'Main' " \
                           "   and (Residue = ? or Residue = ?)";

   // Versions of the client prior to 5.2.6 sent a period at the end of the residue
   // when llr was used.  This will allow a match if someone is using a new client
   // to do a double-check.
   sprintf(withPeriod, "%s.", residue.c_str());

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(candidateName, NAME_LENGTH);
   selectStatement->BindInputParameter(residue, RESIDUE_LENGTH);
   selectStatement->BindInputParameter(withPeriod, RESIDUE_LENGTH);
   selectStatement->BindSelectedColumn(&matchingResidueCount);

   selectStatement->FetchRow(true);

   delete selectStatement;

   if (matchingResidueCount > 0)
      return true;
   else
      return false;
}

// Abandon the test by deleting the CandidateTest record, then resetting the Candidate
bool     PrimeWorkReceiver::AbandonTest(string candidateName, int64_t testID)
{
   bool  success;
   SQLStatement *sqlStatement;
   const char *deleteSQL = "delete from CandidateTest " \
                           " where CandidateName = ? " \
                           "   and TestID = ?";
   const char *updateSQL = "update Candidate " \
                           "   set HasPendingTest = 0 " \
                           " where CandidateName = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindInputParameter(testID);

   success = sqlStatement->Execute();

   delete sqlStatement;

   if (success)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
      success = sqlStatement->Execute();
      delete sqlStatement;
   }

   return UpdateGroupStats(candidateName, R_UNKNOWN);
}

bool     PrimeWorkReceiver::BadProgramVersion(string version)
{
   // Genefer 3.2.0beta-0 was buggy, so block it.  Any other known version is fine.
   if (ii_ServerType == ST_GFN)
   {
      if (version.find("3.2.0beta-0") != string::npos)
         return true;
      if (version.find("unknown") != string::npos)
         return true;
   }

   return false;
}

bool     PrimeWorkReceiver::UpdateGroupStats(string candidateName, result_t mainTestResult)
{
   SQLStatement *sqlStatement;
   int64_t     theK;
   int32_t     theB, theC, theN;
   bool        success;
   const char *selectSQL = "select k, b, c, n " \
                           "  from Candidate " \
                           " where CandidateName = ?";
   const char *updateSQL = "update CandidateGroupStats " \
                           "   set CountInProgress = CountInProgress - 1, " \
                           "       CountUntested = CountUntested - 1, " \
                           "       CountTested = CountTested + 1 " \
                           " where k = %" PRId64 " " \
                           "   and b = %d " \
                           "   and c = %d " \
                           "   and CountInProgress > 0 ";


   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&theK);
   sqlStatement->BindSelectedColumn(&theB);
   sqlStatement->BindSelectedColumn(&theC);
   sqlStatement->BindSelectedColumn(&theN);
    
   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success)
      return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, theK, theB, theC);

   // It is okay if no rows are updated as we don't want CountInProgress to go negative
   success = sqlStatement->Execute();

   if (!success)
      ip_DBInterface->Rollback();

   delete sqlStatement;

   if (!success)
      return false;

   if (ii_ServerType != ST_SIERPINSKIRIESEL)
      return success;

   if (!PRP_OR_PRIME(mainTestResult))
      return success;

   SierpinskiRieselStatsUpdater *sp = (SierpinskiRieselStatsUpdater *) ip_StatsUpdater;
   success = sp->SetSierspinkiRieselPrimeN(theK, theB, theC, theN); 

   if (!success)
      ip_DBInterface->Rollback();

   return success;
}
