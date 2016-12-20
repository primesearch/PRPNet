#include "CandidateTestResult.h"
#include <string.h>

CandidateTestResult::CandidateTestResult(DBInterface *dbInterface, Log *theLog, string candidateName,
                                         int64_t testID, int32_t testIndex, bool briefTestLog,
                                         string userID, string emailID, string machineID, string instanceID, string teamID)
{
   ip_DBInterface = dbInterface;
   ip_Log = theLog;
   is_ParentName = candidateName;
   is_ChildName = candidateName;
   is_WhichTest= "Main";
   is_UserID = userID;
   is_EmailID = emailID;
   is_MachineID = machineID;
   is_InstanceID = instanceID;
   is_TeamID = teamID;

   ii_TestID = testID;
   ii_TestIndex = testIndex;

   ib_CheckedGFNDivisibility = false;
   ib_HadRoundOffError = false;
   ib_HaveSQLError = false;
   ib_BriefTestLog = briefTestLog;
   ii_GFNDivisorCount = 0;
   is_Residue = "";
   is_Program = "";
   is_Prover = "";
   is_ProgramVersion = "";
   is_ProverVersion = "";
}

CandidateTestResult::~CandidateTestResult()
{
}

bool     CandidateTestResult::ProcessChildMessage(string socketMessage)
{
   char    *ptr;
   char     tempMessage[BUFFER_SIZE];
   char     residue[RESIDUE_LENGTH+1];
   char     program[NAME_LENGTH+1];
   char     prover[NAME_LENGTH+1];
   char     programVersion[NAME_LENGTH+1];
   char     proverVersion[NAME_LENGTH+1];
   uint32_t ii;

   strcpy(tempMessage, socketMessage.c_str());

   if (!memcmp(tempMessage, "Start Child ", 12))
      is_ChildName = tempMessage+12;
   else if (!strcmp(tempMessage, "GFN Test Done"))
      ib_CheckedGFNDivisibility = true;
   else if (!memcmp(tempMessage, "Genefer ROE: ", 13))
   {
      InsertGeneferROE(tempMessage+13);
      ib_HadRoundOffError = true;
   }
   else if (!memcmp(tempMessage, "GFN: ", 5))
      InsertGFNDivsior(tempMessage + 5);
   else 
   {
      ptr = strstr(tempMessage, " Test: ");
      if (!ptr)
         return false;

      *ptr = 0;
      ptr += 7;

      is_WhichTest = tempMessage;

      if (sscanf(ptr, "%d %s %s %s %s %s %lf", (int *)
          &ir_TestResult, residue, program, programVersion,
          prover, proverVersion, &id_SecondsForTest) != 7)
         return false;

      if (program[0] == 0 || program[0] == ' ' || residue[0] == 0 || residue[0] == ' ')
	      return false;

      for (ii=0; ii<strlen(residue); ii++)
         residue[ii] = toupper(residue[ii]);

      char *period = strchr(residue, '.');
      if (period) *period = 0;
   }

   is_Residue = residue;
   is_Program = program;
   is_Prover = prover;
   is_ProgramVersion = programVersion;
   is_ProverVersion = proverVersion;

   return true;
}

bool     CandidateTestResult::ProcessMessage(string socketMessage)
{
   char    *ptr1, *ptr2;
   uint32_t ii;
   char     tempMessage[BUFFER_SIZE];
   char     residue[RESIDUE_LENGTH+1];
   char     program[NAME_LENGTH+1];
   char     prover[NAME_LENGTH+1];
   
   strcpy(tempMessage, socketMessage.c_str());

   if (!strcmp(tempMessage, "GFN Test Done"))
      ib_CheckedGFNDivisibility = true;
   else if (!memcmp(tempMessage, "Test Result: ", 13))
   {
      if (sscanf(tempMessage, "Test Result: %s %s", program, residue) != 2)
         return false;

      if (program[0] == 0 || program[0] == ' ' || residue[0] == 0 || residue[0] == ' ')
	      return false;

      for (ii=0; ii<strlen(residue); ii++)
         residue[ii] = toupper(residue[ii]);

      ir_TestResult = ConvertResidueToResult(residue);

      if (PRP_OR_PRIME(ir_TestResult))
         *residue = 0;

      ib_HadRoundOffError = !strcmp(residue, "ROUNDOFFERR");

      if (ib_HadRoundOffError)
      {
         // In older versions the client would send a single space
         // delimited string.  We need to bust that up.
         ptr1 = program;
         ptr2 = strchr(ptr1, ' ');
         while (*ptr1 && *ptr2)
         {
            *ptr2 = 0;
            InsertGeneferROE(ptr1);
            ptr1 = ptr2+1;
            ptr2 = strchr(ptr1, ' ');
         }

         if (*ptr1)
            InsertGeneferROE(ptr1);
      }
   }
   else if (!memcmp(tempMessage, "Proven By: ", 11))
      strcpy(prover, tempMessage+11);
   else if (!memcmp(tempMessage, "GFN: ", 5))
      InsertGFNDivsior(tempMessage + 5);
   else if (!memcmp(tempMessage, "Seconds: ", 9))
      sscanf(tempMessage, "Seconds: %lf", &id_SecondsForTest);
   else
      return false;

   is_Residue = residue;
   is_Program = program;
   is_Prover = prover;

   return true;
}

void     CandidateTestResult::InsertGFNDivsior(string gfn)
{
   SQLStatement *sqlStatement;
   const char *insertSQL = "insert into CandidateGFNDivisor " \
                           "( CandidateName, TestedNumber, GFN, EmailID, MachineID, InstanceID, UserID, TeamID ) " \
                           "values ( ?,?,?,?,?,?,?,? )";
   ii_GFNDivisorCount++;

   if (!ib_HaveSQLError)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(is_ParentName, NAME_LENGTH);
      sqlStatement->BindInputParameter(is_ChildName, NAME_LENGTH);
      sqlStatement->BindInputParameter(gfn, NAME_LENGTH);
      sqlStatement->BindInputParameter(is_EmailID, ID_LENGTH);
      sqlStatement->BindInputParameter(is_MachineID, ID_LENGTH);
      sqlStatement->BindInputParameter(is_InstanceID, ID_LENGTH);
      sqlStatement->BindInputParameter(is_UserID, ID_LENGTH);
      sqlStatement->BindInputParameter(is_TeamID, ID_LENGTH);

      ib_HaveSQLError = !sqlStatement->Execute();

      if (ib_HaveSQLError)
         ip_Log->LogMessage("Failed to insert into CandidateGFNDivisor for candidate %s", is_ChildName.c_str());

      delete sqlStatement;
   }
}

void     CandidateTestResult::InsertGeneferROE(string geneferVersion)
{
   SQLStatement *sqlStatement;
   const char *insertSQL = "insert into GeneferROE " \
                           "( CandidateName, GeneferVersion ) " \
                           "values ( ?,? )";
   if (!ib_HaveSQLError)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(is_ParentName, NAME_LENGTH);
      sqlStatement->BindInputParameter(geneferVersion, NAME_LENGTH);

      ib_HaveSQLError = !sqlStatement->Execute();

      if (ib_HaveSQLError)
         ip_Log->LogMessage("Failed to insert into GeneferROE for candidate %s", is_ParentName.c_str());

      delete sqlStatement;
   }
}

void     CandidateTestResult::LogResults(int32_t socketID, int32_t completedTests, bool needsDoubleCheck,
                                         bool showOnWebPage, double decimalLength)
{
   Log     *prpLog, *testLog;
   char     doubleCheck[30], logHeader[200], prover[50], residue[50];

   if (ii_TestIndex == 1)
      UpdateTest();

   InsertTestResult();

   if (ib_HaveSQLError)
      return;

   if (needsDoubleCheck)
      sprintf(doubleCheck, "DoubleCheck? %s", ((completedTests > 1) ? "Yes" : "No"));
   else
      *doubleCheck = 0;

   if (ib_BriefTestLog)
      sprintf(logHeader, "%s received by %s/%s/%s/%s/%s",
              is_ParentName.c_str(), is_EmailID.c_str(), is_UserID.c_str(),
              is_MachineID.c_str(), is_InstanceID.c_str(), is_Program.c_str());
   else
      sprintf(logHeader, "%s received by Email: %s  User: %s  Machine: %s  Instance: %s  Program: %s",
              is_ParentName.c_str(), is_EmailID.c_str(), is_UserID.c_str(),
              is_MachineID.c_str(), is_InstanceID.c_str(), is_Program.c_str());

   sprintf(prover,  "Prover: %s",  is_Prover.c_str());
   sprintf(residue, "Residue: %s", is_Residue.c_str());

   testLog = new Log(0, "completed_tests.log", 0, false);
   testLog->SetUseLocalTime(ip_Log->GetUseLocalTime());

   if (PRP_OR_PRIME(ir_TestResult))
   {
      InsertUserPrime(decimalLength, showOnWebPage);

      prpLog = new Log(0, "PRP.log", 0, false);
      prpLog->SetUseLocalTime(ip_Log->GetUseLocalTime());

      if (ir_TestResult == R_PRP)
      {
         prpLog->LogMessage("%s: PRP returned by %s using %s!", is_ParentName.c_str(), is_EmailID.c_str(), is_Program.c_str());
         testLog->LogMessage("%s  %s  %s", logHeader, residue, doubleCheck);
         ip_Log->LogMessage("%d: %s  %s  %s", socketID, logHeader, residue, doubleCheck);
      }
      else
      {
         prpLog->LogMessage("%s: Prime returned by %s using %s (%s)!",
                            is_ParentName.c_str(), is_EmailID.c_str(), is_Program.c_str(), prover);
         testLog->LogMessage("%s  %s  %s  %s", logHeader, prover, residue, doubleCheck);
         ip_Log->LogMessage("%d: %s  %s  %s  %s", socketID, logHeader, prover, residue, doubleCheck);
      }

      delete prpLog;
   }
   else
   {
      testLog->LogMessage("%s  %s  %s", logHeader, residue, doubleCheck);
      ip_Log->TestMessage("%d: %s  %s  %s", socketID, logHeader, residue, doubleCheck);
   }

   delete testLog;
}

void     CandidateTestResult::UpdateTest(void)
{
   SQLStatement *sqlStatement;
   const char *updateSQL = "update CandidateTest set IsCompleted = 1 " \
                           " where CandidateName = ? and TestID = ?";

   if (!ib_HaveSQLError)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(is_ParentName, NAME_LENGTH);
      sqlStatement->BindInputParameter(ii_TestID);

      ib_HaveSQLError = !sqlStatement->Execute();

      if (ib_HaveSQLError)
         ip_Log->LogMessage("Failed to update CandidateTest for candidate %s", is_ParentName.c_str());

      delete sqlStatement;
   }
}

void     CandidateTestResult::InsertTestResult(void)
{
   SQLStatement *sqlStatement;
   const char *insertSQL = "insert into CandidateTestResult " \
                           "( CandidateName, TestID, TestIndex, WhichTest, TestedNumber, " \
                           "  TestResult, Residue, PRPingProgram, SecondsForTest, " \
                           "  CheckedGFNDivisibility, PRPingProgramVersion, ProvingProgram, " \
                           "  ProvingProgramVersion ) "\
                           "values ( ?,?,?,?,?,?,?,?,?,?,?,?,? )";

   if (!ib_HaveSQLError)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(is_ParentName, NAME_LENGTH);
      sqlStatement->BindInputParameter(ii_TestID);
      sqlStatement->BindInputParameter(ii_TestIndex);
      sqlStatement->BindInputParameter(is_WhichTest, NAME_LENGTH);
      sqlStatement->BindInputParameter(is_ChildName, NAME_LENGTH);
      sqlStatement->BindInputParameter(ir_TestResult);
      sqlStatement->BindInputParameter(is_Residue, RESIDUE_LENGTH);
      sqlStatement->BindInputParameter(is_Program, NAME_LENGTH);
      sqlStatement->BindInputParameter(id_SecondsForTest);
      sqlStatement->BindInputParameter(ib_CheckedGFNDivisibility);
      sqlStatement->BindInputParameter(is_ProgramVersion, NAME_LENGTH);
      sqlStatement->BindInputParameter(is_Prover, NAME_LENGTH);
      sqlStatement->BindInputParameter(is_ProverVersion, NAME_LENGTH);

      ib_HaveSQLError = !sqlStatement->Execute();

      if (ib_HaveSQLError)
         ip_Log->LogMessage("Failed to insert into CandidateTestResult for candidate %s", is_ChildName.c_str());

      delete sqlStatement;
   }
}

void     CandidateTestResult::LogResults(int32_t socketID, CandidateTestResult *mainTestResult,
                                         bool showOnWebPage, double decimalLength)
{
   Log     *prpLog, *testLog;

   InsertTestResult();

   if (ib_HaveSQLError)
      return;

   testLog = new Log(0, "completed_tests.log", 0, false);
   testLog->LogMessage("%s: %s", is_ChildName.c_str(), is_Residue.c_str());
   delete testLog;

   if (PRP_OR_PRIME(ir_TestResult))
   {
      InsertUserPrime(decimalLength, showOnWebPage);

      prpLog = new Log(0, "PRP.log", 0, false);
      prpLog->SetUseLocalTime(ip_Log->GetUseLocalTime());

      if (ir_TestResult == R_PRP)
         prpLog->LogMessage("%s: PRP returned by %s using %s!",
                            is_ChildName.c_str(), is_EmailID.c_str(), is_Program.c_str());
      else
         prpLog->LogMessage("%s: Prime returned by %s using %s (proven by %s)!",
                            is_ChildName.c_str(), is_EmailID.c_str(), is_Program.c_str(), is_Prover.c_str());

      delete prpLog;
   }

   if (is_WhichTest == TWIN_PREFIX)
      LogTwinResults(socketID, mainTestResult);

   if (is_WhichTest == SGNM1_PREFIX)
      LogSophieGermainResults(socketID, mainTestResult, SG_NM1);

   if (is_WhichTest == SGNP1_PREFIX)
      LogSophieGermainResults(socketID, mainTestResult, SG_NP1);
}

void     CandidateTestResult::InsertUserPrime(double decimalLength, bool showOnWebPage)
{
   SQLStatement *sqlStatement;
   int64_t theTime;
   const char *insertSQL = "insert into UserPrimes " \
                           "( UserID, CandidateName, TestedNumber, TestResult, MachineID, InstanceID, TeamID, " \
                           "  DecimalLength, DateReported, ShowOnWebPage ) " \
                           "values ( ?,?,?,?,?,?,?,?,?,? )";

   theTime = time(NULL);

   if (!ib_HaveSQLError)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
      sqlStatement->BindInputParameter(is_UserID, ID_LENGTH);
      sqlStatement->BindInputParameter(is_ParentName, NAME_LENGTH);
      sqlStatement->BindInputParameter(is_ChildName, NAME_LENGTH);
      sqlStatement->BindInputParameter(ir_TestResult);
      sqlStatement->BindInputParameter(is_MachineID, ID_LENGTH);
      sqlStatement->BindInputParameter(is_InstanceID, ID_LENGTH);
      sqlStatement->BindInputParameter(is_TeamID, ID_LENGTH);
      sqlStatement->BindInputParameter(decimalLength);
      sqlStatement->BindInputParameter(theTime);
      sqlStatement->BindInputParameter(showOnWebPage);

      ib_HaveSQLError = !sqlStatement->Execute();

      if (ib_HaveSQLError)
            ip_Log->LogMessage("Failed to insert into CandidateTestResult for candidate %s", is_ChildName.c_str());

      delete sqlStatement;
   }
}

// Log message specific to Twin prime tests
void     CandidateTestResult::LogTwinResults(int32_t socketID, CandidateTestResult *mainTestResult)
{
   switch (mainTestResult->GetTestResult())
   {
      case R_PRIME:
         switch (ir_TestResult)
         {
            case R_PRIME:
               ip_Log->LogMessage("%d: %s/%s is a Prime Twin Pair!  Both numbers have been proven prime",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str());
               break;
            case R_PRP:
               ip_Log->LogMessage("%d: %s/%+d is a PRP Twin Pair!  %s is prime.  %s requires a primality test",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str(),
				  is_ParentName.c_str(), is_ChildName.c_str());

            default:
               ip_Log->LogMessage("%d: %s is Prime, but %s is composite!",
		                  socketID, is_ParentName.c_str(), is_ChildName.c_str());
               break;
         }
         return;

      case R_PRP:
         switch (ir_TestResult)
         {
            case R_PRP:
               ip_Log->LogMessage("%d: %s/%s is a PRP Twin Pair!  Both numbers require a primality test",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str());
               break;
            case R_PRIME:
               ip_Log->LogMessage("%d: %s/%+d is a PRP Twin Pair!  %s is prime.  %s requires a primality test",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str(),
				  is_ChildName.c_str(), is_ParentName.c_str());

            default:
               ip_Log->LogMessage("%d: %s is PRP, but %s is composite!",
		                  socketID, is_ParentName.c_str(), is_ChildName.c_str());
               break;
         }
         return;

      default:
         return;
   }
}

void     CandidateTestResult::LogSophieGermainResults(int32_t socketID, CandidateTestResult *mainTestResult, sgtype_t sgType)
{
   char     form[10];

   if (sgType == SG_NM1)
      strcpy(form, "n-1");
   else
      strcpy(form, "n+1");

   switch (mainTestResult->GetTestResult())
   {
      case R_PRIME:
         switch (ir_TestResult)
         {
            case R_PRIME:
               ip_Log->LogMessage("%d: %s/%s is a Prime Sophie Germain Pair (%s)!  Both numbers have been proven prime",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str(), form);
               break;
            case R_PRP:
               ip_Log->LogMessage("%d: %s/%+d is a PRP Sophie Germain Pair (%s)!  %s is prime.  %s requires a primality test",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str(),
				  form, is_ParentName.c_str(), is_ChildName.c_str());

            default:
               ip_Log->LogMessage("%d: %s is Prime, but Sophie Germain (%s) %s is composite!",
                                  socketID, is_ParentName.c_str(), form, is_ChildName.c_str());
               break;
         }
         return;

      case R_PRP:
         switch (ir_TestResult)
         {
            case R_PRP:
               ip_Log->LogMessage("%d: %s/%s is a PRP Sophie Germain Pair (%s)!  Both numbers require a primality test",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str(), form);
               break;
            case R_PRIME:
               ip_Log->LogMessage("%d: %s/%+d is a PRP Sophie Germain Pair (%s)!  %s is prime.  %s requires a primality test",
                                  socketID, is_ParentName.c_str(), is_ChildName.c_str(),
				  form, is_ChildName.c_str(), is_ParentName.c_str());

            default:
               ip_Log->LogMessage("%d: %s is PRP, but Sophie Germain (%s) %s is composite (%s)!",
                                  socketID, is_ParentName.c_str(), form, is_ChildName.c_str());
               break;
         }
         return;

      default:
         return;
   }
}
