#include "DMDivisorWorkReceiver.h"

DMDivisorWorkReceiver::DMDivisorWorkReceiver(DBInterface* dbInterface, Socket* theSocket, globals_t* globals,
   string userID, string emailID,
   string machineID, string instanceID, string teamID)
   : WorkReceiver(dbInterface, theSocket, globals,
      userID, emailID, machineID, instanceID, teamID)
{
}

DMDivisorWorkReceiver::~DMDivisorWorkReceiver()
{
}

void  DMDivisorWorkReceiver::ProcessMessage(string theMessage)
{
   char     tempMessage[200];
   char* readBuf;
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

int32_t  DMDivisorWorkReceiver::ReceiveWorkUnit(string theMessage)
{
   char     tempMessage[200];
   char     name[60];
   int64_t  kMin, kMax, kTemp, testID;
   int32_t  returnCode, n;
   bool     success;
   SQLStatement* sqlStatement;
   const char* selectSQL = "select kMax from DMDRangeTest " \
      " where n = ? " \
      "   and kMin = ? " \
      "   and kMax = ? " \
      "   and testId = ? ";

   strcpy(tempMessage, theMessage.c_str());
   if (sscanf(tempMessage, "WorkUnit: %u %" PRIu64" %" PRIu64" %" PRIu64"",
      &n, &kMin, &kMax, &testID) != 4)
   {
      ip_Socket->Send("ERROR: Could not parse WorkUnit [%s]", theMessage.c_str());
      return false;
   }

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(n);
   sqlStatement->BindInputParameter(kMin);
   sqlStatement->BindInputParameter(kMax);
   sqlStatement->BindInputParameter(testID);
   sqlStatement->BindSelectedColumn(&kTemp);
   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   snprintf(name, 60, "%u %" PRIu64" %" PRIu64"", n, kMin, kMax);

   // Verify whether or not the test exists.  It might have expired or the client
   // might be trying to send bad results.
   if (!success)
   {
      // Send INFO rather than ERROR.  Outside of an SQL error, this can happen if
      // the server expired the test or if the client reporting to the wrong server.
      // The client has checks for the latter and SQL errors are extremely unlikely,
      // so this most likely will happen because the test had expired.
      ip_Socket->Send("INFO: Test was ignored.  %s was not found", name);
      ip_Log->LogMessage("%s (%s): Test for %s was not found",
         is_EmailID.c_str(), is_MachineID.c_str(), name);
      return false;
   }

   returnCode = ReceiveWorkUnit(n, kMin, kMax, testID);

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

int32_t  DMDivisorWorkReceiver::ReceiveWorkUnit(int32_t n, int64_t kMin, int64_t kMax, int64_t testID)
{
   int32_t  returnCode;

   returnCode = ProcessWorkUnit(n, kMin, kMax, testID);

   if (returnCode == CT_SUCCESS || returnCode == CT_ABANDONED || returnCode == CT_OBSOLETE)
      ip_DBInterface->Commit();
   else
      ip_DBInterface->Rollback();

   return returnCode;
}

int32_t  DMDivisorWorkReceiver::ProcessWorkUnit(int32_t n, int64_t kMin, int64_t kMax, int64_t testID)
{
   char* theMessage;
   int64_t   endMin, endMax, theK, endTestID;
   bool      abandoned = false, haveStats = false, success;
   int32_t   gotTerminator;
   int32_t   theN, endN, finds = 0;
   double    secondsToTestRange;
   char      theDivisor[ID_LENGTH];
   SQLStatement* sqlStatement;
   const char* updateRange = "update DMDRange " \
      "   set rangeStatus = 2, " \
      "       LastUpdateTime = ? " \
      " where n = ? " \
      "   and kMin = ?" \
      "   and kMax = ?";
   const char* updateRangeTest = "update DMDRangeTest " \
      "   set SecondsToTestRange = ?, " \
      "       SearchingProgram = ?, " \
      "       SearchingProgramVersion = ? " \
      " where n = ? " \
      "   and kMin = ?" \
      "   and kMax = ?" \
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
            if (sscanf(theMessage, "End of WorkUnit: %u %" PRIu64" %" PRIu64" %" PRIu64"", &endN, &endMin, &endMax, &endTestID) != 4)
               return CT_BAD_TERMINATOR;

            if (endN != n || endMin != kMin || endMax != kMax || endTestID != testID)
               return CT_WRONG_TERMINATOR;
         }

         gotTerminator = true;
         break;
      }

      if (!strcmp(theMessage, "Test Abandoned"))
         abandoned = true;
      else if (!memcmp(theMessage, "Stats: ", 7))
      {
         if (sscanf(theMessage + 7, "%s %s %lf", ic_Program, ic_ProgramVersion, &secondsToTestRange) != 3)
         {
            ip_Log->LogMessage("%d: Could not parse [%s]", ip_Socket->GetSocketID(), theMessage);
            return CT_PARSE_ERROR;
         }
         haveStats = true;
      }
      else if (!memcmp(theMessage, "Found: ", 7))
      {
         if (sscanf(theMessage + 7, "%u %" PRIu64" %s", &theN, &theK, &theDivisor) != 3)
         {
            ip_Log->LogMessage("%d: Could not parse [%s]", ip_Socket->GetSocketID(), theMessage);
            return CT_PARSE_ERROR;
         }

         if (!ProcessFind(theN, theK, theDivisor))
            return CT_SQL_ERROR;

         finds++;
      }

      theMessage = ip_Socket->Receive();
   }

   if (!haveStats && !abandoned) return CT_MISSING_DETAIL;

   // Something was messed up in transmission.  We cannot accept the results
   if (!gotTerminator) return CT_MISSING_TERMINATOR;

   // Delete the test if abandoned
   if (abandoned)
   {
      if (AbandonTest(n, kMin, kMax, testID))
         return CT_ABANDONED;
      else
         return CT_SQL_ERROR;
   }

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateRangeTest);
   sqlStatement->BindInputParameter(secondsToTestRange);
   sqlStatement->BindInputParameter(ic_Program, NAME_LENGTH);
   sqlStatement->BindInputParameter(ic_ProgramVersion, NAME_LENGTH);
   sqlStatement->BindInputParameter(n);
   sqlStatement->BindInputParameter(kMin);
   sqlStatement->BindInputParameter(kMax);
   sqlStatement->BindInputParameter(testID);

   success = sqlStatement->Execute();

   delete sqlStatement;

   if (!success) return CT_SQL_ERROR;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateRange);
   sqlStatement->BindInputParameter(finds);
   sqlStatement->BindInputParameter((int64_t)time(NULL));
   sqlStatement->BindInputParameter(n);
   sqlStatement->BindInputParameter(kMin);
   sqlStatement->BindInputParameter(kMax);

   success = sqlStatement->Execute();

   delete sqlStatement;

   if (!success) return CT_SQL_ERROR;

   LogResults(n, kMin, kMax, finds);

   return CT_SUCCESS;
}

// Abandon the test by deleting the CandidateTest record, then resetting the Candidate
bool     DMDivisorWorkReceiver::AbandonTest(int32_t n, int64_t kMin, int64_t kMax, int64_t testID)
{
   bool  success;
   SQLStatement* sqlStatement;
   const char* deleteSQL = "delete from DMDRangeTest " \
      " where n = ? " \
      "   and kMin = ? " \
      "   and kMax = ? " \
      "   and TestID = ?";
   const char* updateSQL = "update DMDRanage " \
      "   set rangeStatus = 0 " \
      " where n = ? " \
      "   and kMin = ? " \
      "   and kMax = ? ";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   sqlStatement->BindInputParameter(n);
   sqlStatement->BindInputParameter(kMin);
   sqlStatement->BindInputParameter(kMax);
   sqlStatement->BindInputParameter(testID);

   success = sqlStatement->Execute();

   delete sqlStatement;

   if (success)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(n);
      sqlStatement->BindInputParameter(kMin);
      sqlStatement->BindInputParameter(kMax);
      success = sqlStatement->Execute();
      delete sqlStatement;
   }

   return success;
}

void     DMDivisorWorkReceiver::LogResults(int32_t n, int64_t kMin, int64_t kMax, int32_t finds)
{
   Log* testLog;
   char name[60];

   snprintf(name, 60, "%u %" PRIu64" %" PRIu64"", n, kMin, kMax);

   if (ib_BriefTestLog)
      ip_Log->TestMessage("%d: %s received by %s/%s/%s/%s/%s",
         ip_Socket->GetSocketID(), name,
         is_EmailID.c_str(), is_UserID.c_str(),
         is_MachineID.c_str(), is_InstanceID.c_str(), ic_Program);
   else
      ip_Log->TestMessage("%d: %s received by Email: %s  User: %s  Machine: %s  Instance: %s  Program: %s",
         ip_Socket->GetSocketID(), name,
         is_EmailID.c_str(), is_UserID.c_str(),
         is_MachineID.c_str(), is_InstanceID.c_str(), ic_Program);

   testLog = new Log(0, "completed_tests.log", 0, false);

   if (finds == 0)
      testLog->LogMessage("%s: Nothing found", name);
   else
      testLog->LogMessage("%s: %d finds", name, finds);

   delete testLog;
}

bool     DMDivisorWorkReceiver::ProcessFind(int32_t n, int64_t k, char* divisor)
{
   Log* findLog;
   bool     success, duplicate;
   int32_t  matchingCount;
   SQLStatement* insertStatement;
   SQLStatement* selectStatement;
   const char* selectSQL = "select from DMDivisor" \
      " where n = ? " \
      "   and k = ? " \
      "   and divisor = ? ";
   const char* insertSQL = "insert into DMDivisor " \
      " ( n, k, divisor, emailID, userID, machineID, instanceID, dateReported ) " \
      " values ( ?, ?, ?, ?, ?, ?, ?, ? )";

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(n);
   selectStatement->BindInputParameter(k);
   selectStatement->BindInputParameter(divisor, ID_LENGTH, true);
   selectStatement->BindSelectedColumn(&matchingCount);
   selectStatement->FetchRow(true);
   delete selectStatement;

   duplicate = (matchingCount > 0);

   insertStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   insertStatement->BindInputParameter(n);
   insertStatement->BindInputParameter(k);
   insertStatement->BindInputParameter(divisor, ID_LENGTH, true);
   insertStatement->BindInputParameter(is_EmailID, ID_LENGTH);
   insertStatement->BindInputParameter(is_UserID, ID_LENGTH);
   insertStatement->BindInputParameter(is_MachineID, ID_LENGTH);
   insertStatement->BindInputParameter(is_InstanceID, ID_LENGTH);
   insertStatement->BindInputParameter((int64_t)time(NULL));
   success = insertStatement->Execute();

   delete insertStatement;

   if (!success) return false;

   findLog = new Log(0, "dm_finds.log", 0, false);

   findLog->LogMessage("%s found by user %s on machine %s, instance %s",
      divisor, is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());

   delete findLog;

   return true;
}
