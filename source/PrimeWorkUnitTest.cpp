#include "PrimeWorkUnitTest.h"

PrimeWorkUnitTest::PrimeWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                                     workunit_t *wu, TestingProgramFactory *testingProgramFactory)
                                     : WorkUnitTest(theLog, serverType, workSuffix, wu)
{
   ip_Log = theLog;
   ip_TestingProgramFactory = testingProgramFactory;
   is_ParentName = wu->s_Name;
   is_ChildName = wu->s_Name;
   is_Residue = "notdone";
   is_Program = "na";
   is_Prover = "na";
   is_ProgramVersion = "na";
   is_ProverVersion = "na";

   iwut_State = WUT_NOTSTARTED;
   iwut_Result = R_UNKNOWN;
   ip_NextWorkUnitTest = NULL;
   ip_FirstGFN = NULL;
   ib_SearchedForGFNDivisors = false;
   ib_HadTestFailure = false;
   id_Seconds = 0.0;
   ii_DecimalLength = 0;
}

PrimeWorkUnitTest::~PrimeWorkUnitTest()
{
   gfn_t   *gfnPtr;

   while (ip_FirstGFN)
   {
      gfnPtr = (gfn_t *) ip_FirstGFN->m_NextGFN;
      delete ip_FirstGFN;
      ip_FirstGFN = gfnPtr;
   }
}

bool     PrimeWorkUnitTest::TestWorkUnit(WorkUnitTest *parentWorkUnit)
{
   result_t testResult;

   if (this != parentWorkUnit)
   {
      testResult = parentWorkUnit->GetWorkUnitTestResult();
      if (!PRP_OR_PRIME(testResult))
         return true;
   }

   ip_TestingProgramFactory->SetNumber(ii_ServerType, is_WorkSuffix, is_ChildName, il_k, ii_b, ii_n, ii_c);

   ib_HadTestFailure = false;
   iwut_State = WUT_INPROGRESS;

   // Start with a PRP test. Note that it is possible that the helper chosen
   // to do the PRP test actually does a primality test, such as the case of
   // Proth numbers with LLR or phrot.
   // Exit the client if this step is not completed or if no helper program is found.
   if (DoPRPTest() != TR_COMPLETED)
      return false;

   // Do a primality test if needed.  Exit the client if this step is cancelled.
   if (DoPrimalityTest() == TR_CANCELLED)
      return false;

   // Check for GFN divisors test if needed.  Exit the client if this step is cancelled.
   if (CheckForGFNDivisibility() == TR_CANCELLED)
      return false;

   LogMessage(parentWorkUnit);

   iwut_State = WUT_COMPLETED;

   return true;
}

// PRP test the given number
testresult_t   PrimeWorkUnitTest::DoPRPTest(void)
{
   TestingProgram   *testingProgram = NULL;
   testresult_t      testResult;
   bool              isNotDone = false, isInProgress = false;
   Log              *testLog;

   // Skip this test if the test has been completed.
   if (is_Residue == "notdone")    isNotDone = true;
   if (is_Residue == "inprogress") isInProgress = true;

   if (!isNotDone && !isInProgress)
   {
      ip_Log->Debug(DEBUG_WORK, "%s has had a PRP test, another PRP test is not needed", is_ChildName.c_str());
      return TR_COMPLETED;
   }

   testingProgram = ip_TestingProgramFactory->GetPRPTestingProgram(ii_ServerType, il_k, ii_b, ii_n);

   if (!testingProgram)
   {
      ip_Log->LogMessage("A required PRP testing program is missing.  Unable to test %s", is_ChildName.c_str());
      ip_Log->LogMessage("The client will now shut down due to this error", is_ChildName.c_str());
      return TR_CANCELLED;
   }

   is_Residue = "inprogress";

   testResult = testingProgram->Execute(TT_PRP);

   if (testResult == TR_CANCELLED)
      return TR_CANCELLED;

   // If genefer could not PRP test the number, use one of the other
   // helpers, if they are configured.
   if (testingProgram->HadTestFailure())
   {
      ib_HadTestFailure = true;

      if (ip_TestingProgramFactory->GetPRPTestingProgram() != NULL)
      {
         testingProgram = ip_TestingProgramFactory->GetPRPTestingProgram();
         testResult = testingProgram->Execute(TT_PRP);

         if (testResult == TR_CANCELLED)
            return TR_CANCELLED;

         ib_HadTestFailure = false;
      }
   }

   iwut_Result = R_COMPOSITE;
   id_Seconds = testingProgram->GetSeconds();

   is_Program = testingProgram->GetInternalProgramName();
   is_ProgramVersion = testingProgram->GetProgramVersion();
   is_Residue = testingProgram->GetResidue();

   ii_DecimalLength = testingProgram->GetDecimalLength();

   if (testingProgram->IsPRP())
   {
      is_Residue = "PRP";
      iwut_Result = R_PRP;
   }

   if (testingProgram->IsPrime())
   {
      is_Residue = " PRIME";
      iwut_Result = R_PRIME;
      is_Prover = testingProgram->GetInternalProgramName();
      is_ProverVersion = testingProgram->GetProgramVersion();
   }

   testLog = new Log(0, "test_results.log", 0, false);
   testLog->LogMessage("Server: %s, Candidate: %s  Program: %s  Residue: %s  Time: %.0lf seconds",
                        is_WorkSuffix.c_str(), is_ChildName.c_str(), is_Program.c_str(), is_Residue.c_str(), id_Seconds);
   delete testLog;

   return testResult;
}

testresult_t   PrimeWorkUnitTest::DoPrimalityTest(void)
{
   TestingProgram   *testingProgram;
   testresult_t      testResult;
   double            seconds;
   Log              *testLog;

   switch (ConvertResidueToResult(is_Residue))
   {
      case R_COMPOSITE:
         ip_Log->Debug(DEBUG_WORK, "%s is composite, a primality test is not needed", is_ChildName.c_str());
         return TR_COMPLETED;

      case R_PRIME:
         ip_Log->Debug(DEBUG_WORK, "%s is already proven prime, primality test is not needed", is_ChildName.c_str());
         return TR_COMPLETED;

      case R_PRP:
         // For Wagstaff, pfgw cannot prove primality
         if (ii_ServerType == ST_WAGSTAFF)
            return TR_COMPLETED;

         // For XYYX, pfgw cannot prove primality
         if (ii_ServerType == ST_XYYX)
            return TR_COMPLETED;
         
         // Don't know if pfgw can prove primality, so don't try
         if (ii_ServerType == ST_GENERIC)
            return TR_COMPLETED;

         // For GFN, factorials and primorials, don't do primality tests for larger n because
         // they can take days and PFGW does not checkpoint during the primality test.  If PFGW
         // is modified to checkpoint during primality tests, then this could be removed.
         if (ii_ServerType == ST_GFN && ii_n > 200000)
            return TR_COMPLETED;

         if (ii_ServerType == ST_FACTORIAL && ii_b > 500000)
            return TR_COMPLETED;

         if (ii_ServerType == ST_PRIMORIAL && ii_b > 500000)
            return TR_COMPLETED;

         testingProgram = ip_TestingProgramFactory->GetPFGWProgram();
         if (!testingProgram && ip_TestingProgramFactory->GetPhrotProgram())
            testingProgram = ip_TestingProgramFactory->GetLLRProgram();
         
         if (!testingProgram || ii_c > 1 || ii_c < -1)
         {
            ip_Log->Debug(DEBUG_WORK, "%s is PRP, but no program is available to prove primality", is_ChildName.c_str());
            return TR_COMPLETED;
         }

         testResult = testingProgram->Execute(TT_PRIMALITY);

         if (testResult == TR_CANCELLED)
         {
            ip_Log->Debug(DEBUG_WORK, "%s terminated during primality test.  Shutting down client",
                          testingProgram->GetInternalProgramName().c_str());
            return testResult;
         }

         seconds = testingProgram->GetSeconds();

         if (testingProgram->IsPrime())
         {
            is_Prover = testingProgram->GetInternalProgramName();
            is_Residue = " PRIME";
            iwut_Result = R_PRIME;
            ip_Log->LogMessage("%s proven prime by %s.  Time:  %.0lf seconds",
                               is_ChildName.c_str(), is_Prover.c_str(), seconds);
         }
         else
            ip_Log->LogMessage("%s could not prove primality for %s.  Time:  %.0lf seconds", 
                               testingProgram->GetInternalProgramName().c_str(), is_ChildName.c_str(), seconds);

         id_Seconds += seconds;

         testLog = new Log(0, "test_results.log", 0, false);
         testLog->LogMessage("Server: %s Candidate: %s  Program: %s  Residue: %s  Time: %.0lf seconds",
                             is_WorkSuffix.c_str(), is_ChildName.c_str(), is_Prover.c_str(), is_Residue.c_str(), seconds);


         delete testLog;
         break;

      case R_UNKNOWN:
         return TR_CANCELLED;
   }

   return TR_COMPLETED;
}

// Check the number for GFN divisibility
testresult_t   PrimeWorkUnitTest::CheckForGFNDivisibility(void)
{
   TestingProgram   *testingProgram;
   testresult_t      testResult;

   if (ii_b != 2 || ii_c != 1 || ii_ServerType == ST_XYYX || ii_ServerType == ST_CAROLKYNEA)
   {
      ip_Log->Debug(DEBUG_WORK, "%s is not a Proth number (form k*2^n+1).  GFN divisibility check skipped", is_ChildName.c_str());
      return TR_COMPLETED;
   }

   if (!PRP_OR_PRIME(ConvertResidueToResult(is_Residue)))
   {
      ip_Log->Debug(DEBUG_WORK, "%s is not PRP or prime.  GFN divisibility check skipped", is_ChildName.c_str());
      return TR_COMPLETED;
   }

   if (ib_SearchedForGFNDivisors)
   {
      ip_Log->Debug(DEBUG_WORK, "%s has already been checked for GFN divisiblity", is_ChildName.c_str());
      return TR_COMPLETED;
   }

   testingProgram = ip_TestingProgramFactory->GetPFGWProgram();

   if (!testingProgram)
   {
      ip_Log->Debug(DEBUG_WORK, "PFGW is not available to check for GFN divisiblity");
      return TR_COMPLETED;
   }

   testResult = testingProgram->Execute(TT_GFN);

   if (testResult == TR_CANCELLED)
   {
      ip_Log->Debug(DEBUG_WORK, "PFGW terminated while checking for GFN divisors.  Shutting down client");
      return testResult;
   }

   ip_FirstGFN = testingProgram->GetGFNList();
   ib_SearchedForGFNDivisors = true;
   return testResult;
}

void  PrimeWorkUnitTest::SendResults(Socket *theSocket)
{
   TestingProgram *testingProgram;
   gfn_t          *gfnPtr;

   // If this is not the main test, i.e. a sub-test, and the test wasn't done,
   // then don't return it to the server.
   if (GetResultPrefix() != MAIN_PREFIX && iwut_Result == R_UNKNOWN)
      return;

   theSocket->Send("Start Child %s", is_ChildName.c_str());

   // Note that a failure can only occur on GFN searches when no other helper
   // programs are configured
   if (ib_HadTestFailure)
   {
      testingProgram = ip_TestingProgramFactory->GetGeneferProgram();
      testingProgram->SendStandardizedName(theSocket, true);
   }
   else
   {
      theSocket->Send("%s Test: %d %s %s %s %s %s %lf", GetResultPrefix().c_str(), iwut_Result,
                      is_Residue.c_str(), is_Program.c_str(), is_ProgramVersion.c_str(),
                      is_Prover.c_str(), is_ProverVersion.c_str(), id_Seconds);

      if (ib_SearchedForGFNDivisors)
      {
         theSocket->Send("GFN Test Done");

         gfnPtr = ip_FirstGFN;
         while (gfnPtr)
         {
            theSocket->Send("GFN: %s", gfnPtr->s_Divisor.c_str());
            gfnPtr = (gfn_t *) gfnPtr->m_NextGFN;
         }
      }
   }

   theSocket->Send("End Child %s", is_ChildName.c_str());
}

void     PrimeWorkUnitTest::Save(FILE *saveFile)
{
   gfn_t          *gfnPtr;

   fprintf(saveFile, "%s: %s %s %s %s %s %s %d %d %d %lf %d\n", GetResultPrefix().c_str(),
           is_ChildName.c_str(), is_Residue.c_str(),
           is_Program.c_str(), is_ProgramVersion.c_str(),
           is_Prover.c_str(), is_ProverVersion.c_str(),
           iwut_State, iwut_Result,
           ib_SearchedForGFNDivisors, id_Seconds, (ip_FirstGFN ? 1 : 0));

   if (!ip_FirstGFN)
      return;

   gfnPtr = ip_FirstGFN;
   while (gfnPtr)
   {
      fprintf(saveFile, "%s GFN: %s\n", GetResultPrefix().c_str(), gfnPtr->s_Divisor.c_str());
      gfnPtr = (gfn_t *) gfnPtr->m_NextGFN;
   }

   fprintf(saveFile, "End GFN\n");
}

void     PrimeWorkUnitTest::Load(FILE *saveFile, string lineIn, string prefix)
{
   gfn_t   *gfnPrevious = 0, *gfnNext;
   char     line[BUFFER_SIZE], temp[20], *ptr;
   int      countScanned;
   int      hasGFNs, searchedForGFNDivisors;
   char     childName[50], residue[50], program[50], programVersion[50];
   char     prover[50], proverVersion[50], tempDivisor[BUFFER_SIZE];

   strcpy(line, lineIn.c_str());

   ptr = strstr(line, ": ");

   countScanned = sscanf(ptr+2, "%s %s %s %s %s %s %d %d %d %lf %d",
              childName, residue, program, programVersion, prover, proverVersion,
              (int *) &iwut_State, (int *) &iwut_Result,
              &searchedForGFNDivisors, &id_Seconds, &hasGFNs);

   if (countScanned != 11)
   {
      printf("Unable to parse line [%s] from save file.  Exiting\n", lineIn.c_str());
      exit(-1);
   }

   ib_SearchedForGFNDivisors = (searchedForGFNDivisors == 1 ? true : false);
   is_ChildName = childName;
   is_Residue = residue;
   is_Program = program;
   is_ProgramVersion = programVersion;
   is_Prover = prover;
   is_ProverVersion = proverVersion;

   if (!hasGFNs)
      return;

   sprintf(temp, "%s GFN", prefix.c_str());

   while (fgets(line, BUFFER_SIZE, saveFile) != NULL)
   {
      if (!memcmp(line, "End GFN", 7))
         return;

      if (memcmp(prefix.c_str(), line, strlen(temp)))
      {
         printf("Expected prefix [%s] on line [%s] was not found.  Exiting\n", temp, line);
         exit(-1);
      }

      gfnNext = new gfn_t;
      if (!gfnPrevious)
         ip_FirstGFN = gfnNext;
      else
         gfnPrevious->m_NextGFN = gfnNext;

      ptr = strstr(line, ": ");
      if (!ptr)
      {
         printf("Missing ':' on line [%s] from save file.  Exiting\n", line);
         exit(-1);
      }
    
      sscanf(ptr+2, "%s", tempDivisor);

      gfnNext->s_Divisor = tempDivisor;
      gfnNext->m_NextGFN = NULL;
      gfnPrevious = gfnNext;
   }
}
