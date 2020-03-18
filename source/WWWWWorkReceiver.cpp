#include "WWWWWorkReceiver.h"
#include "WWWWStatsUpdater.h"

WWWWWorkReceiver:: WWWWWorkReceiver(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                                      string userID, string emailID, 
                                      string machineID, string instanceID, string teamID)
                                      : WorkReceiver(dbInterface, theSocket, globals,
                                                   userID, emailID, machineID, instanceID, teamID)
{
}

WWWWWorkReceiver::~WWWWWorkReceiver()
{
}

void  WWWWWorkReceiver::ProcessMessage(string theMessage)
{
   char     tempMessage[200];
   char    *readBuf;
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

int32_t  WWWWWorkReceiver::ReceiveWorkUnit(string theMessage)
{
   char     tempMessage[200];
   char     name[60];
   int64_t  primesTested, lowerLimit, upperLimit, testID;
   int32_t  returnCode;
   bool     success;
   SQLStatement  *sqlStatement;
   const char    *selectSQL = "select $null_func$(PrimesTested, 0) from WWWWRangeTest " \
                              " where LowerLimit = ? " \
                              "   and UpperLimit = ? " \
                              "   and TestID = ? ";
   
   strcpy(tempMessage, theMessage.c_str());
   if (sscanf(tempMessage, "WorkUnit: %" PRId64" %" PRId64" %" PRId64"",
                           &lowerLimit, &upperLimit, &testID) != 3)
   {
      ip_Socket->Send("ERROR: Could not parse WorkUnit [%s]", theMessage.c_str());
      return false;
   }

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);
   sqlStatement->BindInputParameter(testID);
   sqlStatement->BindSelectedColumn(&primesTested);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   sprintf(name, "range %" PRId64":%" PRId64"", lowerLimit, upperLimit);

   // Verify whether or not the test exists.  It might have expired or the client
   // might be trying to send bad results.
   if (!success)
   {
      // Send INFO rather than ERROR.  Outside of an SQL error, this can happen if
      // the server expired the test or if the client reporting to the wrong server.
      // The client has checks for the latter and SQL errors are extremely unlikely,
      // so this most likely will happen because the test had expired.
      ip_Socket->Send("INFO: Test was ignored.  Range %" PRId64":%" PRId64" was not found", lowerLimit, upperLimit);
      ip_Log->LogMessage("%s (%s): Test for %s was not found",
                            is_EmailID.c_str(), is_MachineID.c_str(), name);
      return false;
   }

   // Verify whether or not the test has already been reported as completed.
   if (primesTested > 1)
   {
      ip_Socket->Send("INFO: Test for %s was already reported", name);
      ip_Log->LogMessage("%s (%s %s): Test for %s was already reported",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
      return false;
   }

   returnCode = ReceiveWorkUnit(lowerLimit, upperLimit, testID);

   switch (returnCode)
   {
      case CT_BAD_TERMINATOR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The message terminater was truncated", name);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to truncated terminator",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return false;

      case CT_WRONG_TERMINATOR:     
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The message terminater was incorrect", name);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to an incorrect terminator",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return false;

      case CT_MISSING_TERMINATOR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The message terminater was missing", name);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to missing terminator",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return false;

      case CT_MISSING_DETAIL:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The message lacked statistics", name);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to missing stats",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return false;

      case CT_SQL_ERROR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The server encounted an SQL error", name);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to SQL error",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return false;

      case CT_PARSE_ERROR:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The server could not parse the message", name);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to a parsing error",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return false;

      case CT_OBSOLETE:
         ip_Socket->Send("INFO:  Test for %s was ignored.  Your version of wwwwcl is obsolete and MUST be upgraded", name);
         ip_Log->LogMessage("%s (%s %s): Igonred test on %s due to obsolete wwwwcl version",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return true;

      case CT_ABANDONED:
         ip_Socket->Send("INFO: Test for %s was abandoned", name);
         ip_Log->LogMessage("Test for %s was abandoned by %s, machine %s, instance %s", name,
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
         return true;

      case CT_SUCCESS:
         ip_Socket->Send("INFO: Test for %s was accepted", name);
         return true;

      default:
         ip_Socket->Send("ERROR:  Test for %s was rejected.  The server encounted an unknown error", name);
         ip_Log->LogMessage("%s (%s %s): Rejected test on %s due to unknown error",
                            is_EmailID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str(), name);
         return false;
   }

   return false;
}

int32_t  WWWWWorkReceiver::ReceiveWorkUnit(int64_t lowerLimit, int64_t upperLimit, int64_t testID)
{
   int32_t  returnCode;

   returnCode = ProcessWorkUnit(lowerLimit, upperLimit, testID);

   if (returnCode == CT_SUCCESS || returnCode == CT_ABANDONED || returnCode == CT_OBSOLETE)
      ip_DBInterface->Commit();
   else
      ip_DBInterface->Rollback();

   return returnCode;
}

int32_t  WWWWWorkReceiver::ProcessWorkUnit(int64_t lowerLimit, int64_t upperLimit, int64_t testID)
{
   char     *theMessage, checkSum[30];
   int64_t   endLowerLimit, endUpperLimit, endTestID, prime, primesTested;
   int32_t   remainder, quotient;
   bool      abandoned = false, obsolete = false, haveStats = false, doubleChecked, success;
   int32_t   gotTerminator, completedTests;
   int32_t   finds = 0, nearFinds = 0;
   double    secondsToTestRange;
   SQLStatement *sqlStatement;
   const char *selectSQL = "select CompletedTests " \
                           "  from WWWWRange " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ?";
   const char *updateRange = "update WWWWRange " \
                             "   set HasPendingTest = 0, " \
                             "       CompletedTests = CompletedTests + 1, " \
                             "       DoubleChecked = ?, " \
                             "       LastUpdateTime = ? " \
                             " where LowerLimit = ? " \
                             "   and UpperLimit = ?";
   const char *updateRangeTest = "update WWWWRangeTest " \
                                 "   set PrimesTested = ?, " \
                                 "       CheckSum = ?, " \
                                 "       SecondsToTestRange = ?, " \
                                 "       SearchingProgram = ?, " \
                                 "       SearchingProgramVersion = ? " \
                                 " where LowerLimit = ? " \
                                 "   and UpperLimit = ? " \
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
            if (sscanf(theMessage, "End of WorkUnit: %" PRId64" %" PRId64" %" PRId64"", &endLowerLimit, &endUpperLimit, &endTestID) != 3)
               return CT_BAD_TERMINATOR;

            if (endLowerLimit != lowerLimit || endUpperLimit != upperLimit || endTestID != testID)
               return CT_WRONG_TERMINATOR;
         }

         gotTerminator = true;
         break;
      }

      if (!strcmp(theMessage, "Test Abandoned"))
         abandoned = true;
      else if (!memcmp(theMessage, "Stats: ", 7))
      {
         if (sscanf(theMessage+7, "%s %s %" PRId64" %s %lf", ic_Program, ic_ProgramVersion, &primesTested, checkSum, &secondsToTestRange) != 5)
         {
            ip_Log->LogMessage("%d: Could not parse [%s]", ip_Socket->GetSocketID(), theMessage);
            return CT_PARSE_ERROR;
         }
         if (BadProgramVersion(ic_ProgramVersion))
         {
            ip_Log->LogMessage("%d: Obsolete wwwwcl (%s) used: [%s]", ip_Socket->GetSocketID(), ic_ProgramVersion, theMessage);
            obsolete = true;
         }
         haveStats = true;
      }
      else if (!memcmp(theMessage, "Found: ", 7))
      {
         if (sscanf(theMessage+7, "%" PRId64" %d %d", &prime, &remainder, &quotient) != 3)
         {
            ip_Log->LogMessage("%d: Could not parse [%s]", ip_Socket->GetSocketID(), theMessage);
            return CT_PARSE_ERROR;
         }
         
         if (!ProcessFind(lowerLimit, upperLimit, testID, prime, remainder, quotient))
            return CT_SQL_ERROR;

         if (!remainder && !quotient)
            finds++;
         else
            nearFinds++;
      }

      theMessage = ip_Socket->Receive();
   }

   if (!haveStats && !abandoned) return CT_MISSING_DETAIL;

   // Something was messed up in transmission.  We cannot accept the results
   if (!gotTerminator) return CT_MISSING_TERMINATOR;

   // Delete the test if abandoned
   if (abandoned || obsolete)
   {
      if (AbandonTest(lowerLimit, upperLimit, testID))
         return abandoned ? CT_ABANDONED : CT_OBSOLETE;
      else
         return CT_SQL_ERROR;
   }

   // Get some information about the candidate before continuing
   completedTests = 0;
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);
   sqlStatement->BindSelectedColumn(&completedTests);
   sqlStatement->FetchRow(true);
   delete sqlStatement;
   completedTests++;

   doubleChecked = CheckDoubleCheck(lowerLimit, upperLimit, primesTested, checkSum);

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateRange);
   sqlStatement->BindInputParameter(doubleChecked);
   sqlStatement->BindInputParameter((int64_t) time(NULL));
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);

   success = sqlStatement->Execute();

   delete sqlStatement;

   if (!success) return CT_SQL_ERROR;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateRangeTest);
   sqlStatement->BindInputParameter(primesTested);
   sqlStatement->BindInputParameter(checkSum, sizeof(checkSum));
   sqlStatement->BindInputParameter(secondsToTestRange);
   sqlStatement->BindInputParameter(ic_Program, NAME_LENGTH);
   sqlStatement->BindInputParameter(ic_ProgramVersion, NAME_LENGTH);
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);
   sqlStatement->BindInputParameter(testID);

   success = sqlStatement->Execute();

   delete sqlStatement;

   if (!success) return CT_SQL_ERROR;

   LogResults(lowerLimit, upperLimit, finds, nearFinds);

   success = ((WWWWStatsUpdater *) ip_StatsUpdater)->UpdateStats(is_UserID, is_TeamID, finds, nearFinds);

   if (!success) return CT_SQL_ERROR;

   return CT_SUCCESS;
}

// Determine if this residue matches another residue for the candidate.  Note that
// this check is done BEFORE the test results for this test are inserted into the database.
bool     WWWWWorkReceiver::CheckDoubleCheck(int64_t lowerLimit, int64_t upperLimit,
                                            int64_t primesTested, string checksum)
{
   int32_t   matchingCount = 0;
   SQLStatement *selectStatement;
   const char *selectSQL = "select count(*) from WWWWRangeTest " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ? " \
                           "   and PrimesTested = ? " \
                           "   and CheckSum = ?";

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(lowerLimit);
   selectStatement->BindInputParameter(upperLimit);
   selectStatement->BindInputParameter(primesTested);
   selectStatement->BindInputParameter(checksum, 30);
   selectStatement->BindSelectedColumn(&matchingCount);

   selectStatement->FetchRow(true);

   delete selectStatement;

   if (matchingCount > 0)
      return true;
   else
      return false;
}

// Abandon the test by deleting the CandidateTest record, then resetting the Candidate
bool     WWWWWorkReceiver::AbandonTest(int64_t lowerLimit, int64_t upperLimit, int64_t testID)
{
   bool  success;
   SQLStatement *sqlStatement;
   const char *deleteSQL = "delete from WWWWRangeTest " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ? " \
                           "   and TestID = ?";
   const char *updateSQL = "update WWWWRange " \
                           "   set HasPendingTest = 0 " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   sqlStatement->BindInputParameter(lowerLimit);
   sqlStatement->BindInputParameter(upperLimit);
   sqlStatement->BindInputParameter(testID);

   success = sqlStatement->Execute();

   delete sqlStatement;

   if (success)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(lowerLimit);
      sqlStatement->BindInputParameter(upperLimit);
      success = sqlStatement->Execute();
      delete sqlStatement;
   }

   return success;
}

void     WWWWWorkReceiver::LogResults(int64_t lowerLimit, int64_t upperLimit, int32_t finds, int32_t nearFinds)
{
   Log     *testLog;

   if (ib_BriefTestLog)
      ip_Log->TestMessage("%d: %" PRId64":%" PRId64" received by %s/%s/%s/%s/%s",
                          ip_Socket->GetSocketID(), lowerLimit, upperLimit,
                          is_EmailID.c_str(), is_UserID.c_str(),
                          is_MachineID.c_str(), is_InstanceID.c_str(), ic_Program);
   else
      ip_Log->TestMessage("%d: %" PRId64":%" PRId64" received by Email: %s  User: %s  Machine: %s  Instance: %s  Program: %s",
                          ip_Socket->GetSocketID(), lowerLimit, upperLimit,
                          is_EmailID.c_str(), is_UserID.c_str(),
                          is_MachineID.c_str(), is_InstanceID.c_str(), ic_Program);

   testLog = new Log(0, "completed_tests.log", 0, false);

   if (finds + nearFinds == 0)
      testLog->LogMessage("%" PRId64":%" PRId64": Nothing found", lowerLimit, upperLimit);
   else
      testLog->LogMessage("%" PRId64":%" PRId64": %d finds", lowerLimit, upperLimit, finds + nearFinds);

   delete testLog;
}

bool     WWWWWorkReceiver::ProcessFind(int64_t lowerLimit, int64_t upperLimit, int64_t testID,
                                       int64_t prime, int32_t remainder, int32_t quotient)
{
   Log     *findLog;
   bool     success, duplicate;
   int32_t  matchingCount;
   SQLStatement *insertStatement;
   SQLStatement *selectStatement;
   const char *selectSQL = "select count(*) from WWWWRangeTestResult " \
                           " where Prime = ? " \
                           "   and Remainder = ?";
   const char *insertSQL1 = "insert into WWWWRangeTestResult " \
                            " ( LowerLimit, UpperLimit, TestID, Prime, Remainder, Quotient, Duplicate, EmailSent ) " \
                            " values ( ?, ?, ?, ?, ?, ?, ?, 0 )";
   const char *insertSQL2 = "insert into UserWWWWs " \
                            " ( UserID, Prime, Remainder, Quotient, MachineID, InstanceID, TeamID, DateReported, ShowOnWebPage ) " \
                            " values ( ?, ?, ?, ?, ?, ?, ?, ?, ? )";

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(prime);
   selectStatement->BindInputParameter(remainder);
   selectStatement->BindInputParameter(quotient);
   selectStatement->BindSelectedColumn(&matchingCount);
   selectStatement->FetchRow(true);
   delete selectStatement;

   duplicate = (matchingCount > 0);

   insertStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL1);
   insertStatement->BindInputParameter(lowerLimit);
   insertStatement->BindInputParameter(upperLimit);
   insertStatement->BindInputParameter(testID);
   insertStatement->BindInputParameter(prime);
   insertStatement->BindInputParameter(remainder);
   insertStatement->BindInputParameter(quotient);
   insertStatement->BindInputParameter(duplicate);
   success = insertStatement->Execute();

   delete insertStatement;

   if (!success) return false;

   insertStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL2);
   insertStatement->BindInputParameter(is_UserID, ID_LENGTH);
   insertStatement->BindInputParameter(prime);
   insertStatement->BindInputParameter(remainder);
   insertStatement->BindInputParameter(quotient);
   insertStatement->BindInputParameter(is_MachineID, ID_LENGTH);
   insertStatement->BindInputParameter(is_InstanceID, ID_LENGTH);
   insertStatement->BindInputParameter(is_TeamID, ID_LENGTH);
   insertStatement->BindInputParameter((int64_t) time(NULL));
   insertStatement->BindInputParameter(ib_ShowOnWebPage);
   success = insertStatement->Execute();

   delete insertStatement;

   if (!success) return false;

   findLog = new Log(0, "wwww_finds.log", 0, false);

   if (!remainder && !quotient)
      findLog->LogMessage("%" PRId64" found by user %s on machine %s, instance %s",
                          prime, is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
   else
   {
      if (ii_ServerType == ST_WALLSUNSUN)
         findLog->LogMessage("%" PRId64" (0 %+d p) found by user %s on machine %s, instance %s",
                             prime, quotient, is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
      else
         findLog->LogMessage("%" PRId64" (%+d %+d p) found by user %s on machine %s, instance %s",
                             prime, remainder, quotient, is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
   }

   delete findLog;

   return true;
}

// wwwwcl versions prior to 2.2.5 returned invalid results for Wall-Sun-Sun.
// Accept all 1.x versions (wwwww) and acceot all 2.2.5  (wwwwcl) and above.
// Reject anything 2.x.x that is below 2.2.5.
bool     WWWWWorkReceiver::BadProgramVersion(string version)
{
   int32_t   v_major, v_minor, v_release;
   char      tempVersion[20];

   strcpy(tempVersion, version.c_str());

   if (ii_ServerType != ST_WALLSUNSUN)
      return false;                                                     // if not WSS, accept any version
   if (sscanf(tempVersion, "%d.%d.%d", &v_major, &v_minor, &v_release) == 3)
      return 10000*v_major + 100*v_minor + v_release < 20205;           //  wwwwcl; return bad (true) if version < 2.2.5
   if (sscanf(tempVersion, "%d.%d", &v_major, &v_minor) == 2)
      if (v_major == 1)
         return false;                                                  //  wwww - return valid (false) regardless of version
   return true;                                                         //  reject anything else
}
