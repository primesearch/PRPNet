#include "PrimeMail.h"
#include "SQLStatement.h"

PrimeMail::PrimeMail(globals_t *globals, string serverName, uint32_t portID)
                     : Mail(globals, serverName, portID)
{
}

void  PrimeMail::MailSpecialResults(void)
{
   SQLStatement *selectStatement;
   SQLStatement *updateStatement;
   char     candidateName[NAME_LENGTH+1], emailID[ID_LENGTH+1];
   double   decimalLength;
   int64_t  testID = 0;
   const char *selectSQL = "select Candidate.CandidateName, " \
                           "       Candidate.DecimalLength, " \
                           "       CandidateTest.TestID, " \
                           "       CandidateTest.EmailID " \
                           "  from Candidate, " \
                           "       CandidateTest " \
                           " where Candidate.CandidateName = CandidateTest.CandidateName " \
                           "   and CandidateTest.EmailSent = 0 " \
                           "   and Candidate.MainTestResult > 0";
   const char *updateSQL = "update CandidateTest " \
                           "   set EmailSent = 1 " \
                           " where CandidateName = ? " \
                           "   and TestId = ?";

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   selectStatement->BindSelectedColumn(&decimalLength);
   selectStatement->BindSelectedColumn(&testID);
   selectStatement->BindSelectedColumn(emailID, ID_LENGTH);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(candidateName, NAME_LENGTH, false);
   updateStatement->BindInputParameter(testID);

   while (selectStatement->FetchRow(false))
   {
      if (!NotifyUser(emailID, candidateName, testID, decimalLength)) continue;

      updateStatement->SetInputParameterValue(candidateName, true);
      updateStatement->SetInputParameterValue(testID);

      if (updateStatement->Execute())
         ip_DBInterface->Commit();
      else
         ip_DBInterface->Rollback();

      ip_Log->LogMessage("Sent e-mail for %s", candidateName);
   }

   delete updateStatement;
   delete selectStatement;
}

void  PrimeMail::MailLowWorkNotification(int32_t daysLeft)
{
   bool  success;

   success = NewMessage(is_FromEmailID.c_str(), "PRPNet Running Dry");
   if (success)
   {
      ip_Socket->StartBuffering();
      ip_Socket->SetAutoNewLine(false);

      AppendLine(0, "There PRPNet server for project %s has an estimated %d days left of work.", is_ProjectName.c_str(), daysLeft);
      
      ip_Socket->SetAutoNewLine(true);
      SendMessage();
   }
}

bool  PrimeMail::NotifyUser(string toEmailID, string candidateName, 
                            int64_t testID, double decimalLength)
{
   SQLStatement *testStatment;
   SQLStatement *srStatement;
   char     resultText[10];
   char     machineID[ID_LENGTH+1], instanceID[ID_LENGTH+1], whichTest[NAME_LENGTH+1];
   char     testedNumber[NAME_LENGTH+1];
   char     prpingProgram[NAME_LENGTH+1], provingProgram[NAME_LENGTH+1];
   int32_t  testIndex, testResult, checkedGFNDivisibility;
   int64_t  theK;
   int32_t  theB, theC, count;
   double   secondsForTest;
   const char *testResultSelect = "select Candidate.k, Candidate.b, Candidate.c, " \
                                  "       CandidateTest.MachineID, " \
                                  "       CandidateTest.InstanceID, " \
                                  "       TestIndex, WhichTest, TestedNumber, " \
                                  "       TestResult, PRPingProgram, " \
                                  "       ProvingProgram, SecondsForTest, CheckedGFNDivisibility " \
                                  "  from Candidate, CandidateTest, CandidateTestResult "
                                  " where CandidateTest.CandidateName = Candidate.CandidateName " \
                                  "   and CandidateTest.CandidateName = CandidateTestResult.CandidateName " \
                                  "   and CandidateTest.TestID = CandidateTestResult.TestID " \
                                  "   and CandidateTest.CandidateName = ? " \
                                  "   and CandidateTest.TestID = ? " \
                                  "order by TestIndex";
   const char *srStatSelect = "select count(*) from CandidateGroupStats "
                              " where k = ? and b = ? and c = ? " \
                              "   and PRPandPrimesFound = 0";

   testStatment = new SQLStatement(ip_Log, ip_DBInterface, testResultSelect);
   testStatment->BindInputParameter(candidateName, NAME_LENGTH);
   testStatment->BindInputParameter(testID);
   testStatment->BindSelectedColumn(&theK);
   testStatment->BindSelectedColumn(&theB);
   testStatment->BindSelectedColumn(&theC);
   testStatment->BindSelectedColumn(machineID, ID_LENGTH);
   testStatment->BindSelectedColumn(instanceID, ID_LENGTH);
   testStatment->BindSelectedColumn(&testIndex);
   testStatment->BindSelectedColumn(whichTest, NAME_LENGTH);
   testStatment->BindSelectedColumn(testedNumber, NAME_LENGTH);
   testStatment->BindSelectedColumn(&testResult);
   testStatment->BindSelectedColumn(prpingProgram, NAME_LENGTH);
   testStatment->BindSelectedColumn(provingProgram, NAME_LENGTH);
   testStatment->BindSelectedColumn(&secondsForTest);
   testStatment->BindSelectedColumn(&checkedGFNDivisibility);

   if (!testStatment->FetchRow(false))
   {
      delete testStatment;
      return false;
   }

   if (testResult == R_PRP)
      strcpy(resultText, "PRP");
   else
      strcpy(resultText, "Prime");

   if (!NewMessage(toEmailID, "Candidate %s was found to be %s by PRPNet!", candidateName.c_str(), resultText))
      return false;

   ip_Socket->StartBuffering();
   ip_Socket->SetAutoNewLine(false);

   AppendLine(0, "This number is approximately %.lf digits long.", decimalLength);
   AppendLine(0, "It was found on machine %s, instane %s, using the program %s.", machineID, instanceID, prpingProgram);

   if (testResult == R_PRP)
   {
      AppendLine(0, "Since the test performed as only a PRP test, another program, such as PFGW must be used to prove primality.");
      AppendLine(0, "If the number is prime and is large enough for the Prime Pages, please include the program %s in the credits.",
                          prpingProgram);
   }
   else
   {
      AppendLine(0, "It was proven prime with the program %s", provingProgram);

      if (strcmp(prpingProgram, provingProgram))
         AppendLine(0, "If the number is large enough for the Prime Pages, please include the programs %s and %s in the credits.",
                             prpingProgram, provingProgram);
      else
         AppendLine(0, "If the number is large enough for the Prime Pages, please include the program %s in the credits.",
                              provingProgram);
   }

   if (is_ProjectName.length())
      AppendLine(0, "If adding to the Prime Pages don't forget to include project \"%s\" in the credits.",
                 is_ProjectName.c_str());

   AppendGFNDivisibilityData(theB, theC, checkedGFNDivisibility, candidateName, testedNumber);

   while (testStatment->FetchRow(false))
   {
      if (testResult == R_PRP)
         strcpy(resultText, "PRP");
      else
         strcpy(resultText, "prime");

      if (!strcmp(whichTest, TWIN_PREFIX))
      {
         switch (testResult)
         {
            case R_PRP:
               AppendLine(2, "Congratulations!!!  %s and %s might be twin primes!!!", candidateName.c_str(), testedNumber);
               AppendLine(0, "A primality test must be run on %s to prove its primality.", testedNumber);
               break;
            case R_PRIME:
               AppendLine(2, "Congratulations!!!  %s and %s are twin primes!!!", candidateName.c_str(), testedNumber);
               break;
            default:
               AppendLine(2, "Your client also tested %s to determine if it is a prime twin.", testedNumber);
               AppendLine(0, "Unfortunately, it is neither PRP nor prime.");
         }
      }

      if (!strcmp(whichTest, SGNM1_PREFIX) || !strcmp(whichTest, SGNP1_PREFIX))
      {
         switch (testResult)
         {
            case R_PRP:
               AppendLine(2, "Congratulations!!!  %s and %s might be Sophie Germain primes!!!", candidateName.c_str(), testedNumber);
               AppendLine(0, "A primality test must be run on %s to prove its primality.", testedNumber);
               break;
            case R_PRIME:
               AppendLine(2, "Congratulations!!!  %s and %s are Sophie Germain primes!!!", candidateName.c_str(), testedNumber);
               break;
            default:
               AppendLine(2, "Your client also tested %s to determine if it is Sophie Germain prime.", testedNumber);
               AppendLine(0, "Unfortunately, it is neither PRP nor prime.");
         }
      }

      if (ii_ServerType == ST_SIERPINSKIRIESEL)
      {
         AppendLine(2, "This prime eliminates k=%" PRId64" for %s base %d", theK, (theC == 1 ? "Sierpinski" : "Riesel"), theB);

         srStatement = new SQLStatement(ip_Log, ip_DBInterface, srStatSelect);
         srStatement->BindInputParameter(theK);
         srStatement->BindInputParameter(theB);
         srStatement->BindInputParameter(theC);
         srStatement->BindSelectedColumn(&count);
         if (srStatement->FetchRow(true))
         {
            if (count > 0)
               AppendLine(0, "Unfortunately there are still %d k values for this conjecture without a prime.", count);
            else
            {
               AppendLine(0, "Congratulations!!!  There are no k values without a prime.");
               AppendLine(0, "It is possible that you have proven this conjecture.");
            }
         }
         delete srStatement;
      }

      AppendGFNDivisibilityData(theB, theC, checkedGFNDivisibility, candidateName, testedNumber);
   }

   ip_Socket->SetAutoNewLine(true);

   SendMessage();

   delete testStatment;

   return true;
}

void  PrimeMail::AppendGFNDivisibilityData(int32_t theB, int32_t theC,
                                           int32_t checkedGFNDivisibility,
                                           string candidateName, string testedNumber)
{
   SQLStatement *gfnStatement = 0;
   bool     foundGFNDivisor;
   char     gfn[50];
   const char *gfnDivisorSelect = "select GFN from CandidateGFNDivisor " \
                                  " where CandidateName = ? " \
                                  "   and TestedNumber = ?";

   if (theB != 2 || theC != 1)
      return;

   if (checkedGFNDivisibility)
   {
      gfnStatement = new SQLStatement(ip_Log, ip_DBInterface, gfnDivisorSelect);
      gfnStatement->BindInputParameter(candidateName, NAME_LENGTH);
      gfnStatement->BindInputParameter(testedNumber, NAME_LENGTH);
      gfnStatement->BindSelectedColumn(gfn, sizeof(gfn));

      foundGFNDivisor = false;
      if (gfnStatement->FetchRow(false))
      {
         if (!foundGFNDivisor)
            AppendLine(2, "The client searched and found the following GFN divisors for %s:", testedNumber.c_str());

         foundGFNDivisor = true;
         AppendLine(1, "   %s", gfn);
      }

      if (!foundGFNDivisor)
         AppendLine(2, "The client searched for GFN divisors for the number %s, but did not find any.", testedNumber.c_str());
   }
   else
      AppendLine(2, "The client did not run a test to search for GFN divisors.");

   delete gfnStatement;
}
