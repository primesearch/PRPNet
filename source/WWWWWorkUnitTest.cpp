#include "WWWWWorkUnitTest.h"

WWWWWorkUnitTest::WWWWWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                                   workunit_t *wu, int32_t specialThreshhold,
                                   TestingProgramFactory *testingProgramFactory)
                                   : WorkUnitTest(theLog, serverType, workSuffix, wu)
{
   is_Program = "na";
   is_ProgramVersion = "na";
   ii_SpecialThreshhold = specialThreshhold;
   ip_TestingProgramFactory = testingProgramFactory;

   il_LowerLimit = wu->l_LowerLimit;
   il_UpperLimit = wu->l_UpperLimit;

   iwut_State = WUT_NOTSTARTED;
   iwut_Result = R_UNKNOWN;
   ip_FirstWWWW = NULL;
   id_Seconds = 0.0;
   il_PrimesTested = 0;
   is_Checksum = "na";
}

WWWWWorkUnitTest::~WWWWWorkUnitTest()
{
   wwww_t   *wwwwPtr;

   while (ip_FirstWWWW)
   {
      wwwwPtr = (wwww_t *) ip_FirstWWWW->m_NextWWWW;
      delete ip_FirstWWWW;
      ip_FirstWWWW = wwwwPtr;
   }
}

bool  WWWWWorkUnitTest::TestWorkUnit(WorkUnitTest *masterWorkUnit)
{
   wwww_t      *wwwwPtr;
   WWWWProgram *testingProgram;
   Log         *testLog;
   testresult_t   testResult;

   testingProgram = ip_TestingProgramFactory->GetWWWWProgram();
   is_Program = testingProgram->GetInternalProgramName();
   is_ProgramVersion = testingProgram->GetProgramVersion();

   iwut_State = WUT_INPROGRESS;
   
   testResult = testingProgram->Execute(ii_ServerType, ii_SpecialThreshhold, il_LowerLimit, il_UpperLimit);

   if (testResult != TR_COMPLETED)
      return false;

   iwut_State = WUT_COMPLETED;

   LogMessage(masterWorkUnit);

   ip_FirstWWWW = testingProgram->GetWWWWList();
   id_Seconds = testingProgram->GetSeconds();
   il_PrimesTested = testingProgram->GetPrimesTested();
   is_Checksum = testingProgram->GetChecksum();

   testLog = new Log(0, "test_results.log", 0, false);
   testLog->LogMessage("Server: %s, Range: %" PRId64":%" PRId64"  Program: %s  Checksum: %s  Time: %.0lf seconds",
                        is_WorkSuffix.c_str(), il_LowerLimit, il_UpperLimit, is_Program.c_str(), is_Checksum.c_str(), id_Seconds);

   wwwwPtr = ip_FirstWWWW;
   while (wwwwPtr)
   {
      testLog->LogMessage("Server: %s, Found: %" PRId64" %d %d",
                          is_WorkSuffix.c_str(), wwwwPtr->l_Prime, wwwwPtr->i_Remainder, wwwwPtr->i_Quotient);
      wwwwPtr = (wwww_t *) wwwwPtr->m_NextWWWW;
   }

   delete testLog;
   return true;
}

void  WWWWWorkUnitTest::SendResults(Socket *theSocket)
{
   wwww_t   *wwwwPtr;

   wwwwPtr = ip_FirstWWWW;
   while (wwwwPtr)
   {
      theSocket->Send("Found: %" PRId64" %d %d", wwwwPtr->l_Prime, wwwwPtr->i_Remainder, wwwwPtr->i_Quotient);
      wwwwPtr = (wwww_t *) wwwwPtr->m_NextWWWW;
   }
}

void     WWWWWorkUnitTest::Save(FILE *saveFile)
{
   wwww_t   *wwwwPtr;

   fprintf(saveFile, "%" PRId64" %" PRId64": %s %s %d %lf %" PRId64" %s %d\n",
           il_LowerLimit, il_UpperLimit, is_Program.c_str(), is_ProgramVersion.c_str(),
           iwut_State, id_Seconds, il_PrimesTested, is_Checksum.c_str(), (ip_FirstWWWW ? 1 : 0));

   if (!ip_FirstWWWW)
      return;

   wwwwPtr = ip_FirstWWWW;
   while (wwwwPtr)
   {
      fprintf(saveFile, "WWWW: %" PRId64" %+d %+d\n", wwwwPtr->l_Prime, wwwwPtr->i_Remainder, wwwwPtr->i_Quotient);
      wwwwPtr = (wwww_t *) wwwwPtr->m_NextWWWW;
   }

   fprintf(saveFile, "End WWWW\n");
}

void     WWWWWorkUnitTest::Load(FILE *saveFile, string lineIn, string prefix)
{
   wwww_t  *wwwwPrevious = 0, *wwwwNext;
   char     line[500], *ptr;
   int      countScanned;
   int      hasWWWWs;
   char     program[100], programVersion[100], Checksum[50];
   char     tempLine[BUFFER_SIZE];

   strcpy(tempLine, lineIn.c_str());
   ptr = strstr(tempLine, ": ");

   countScanned = sscanf(ptr+2, "%s %s %d %lf %" PRId64" %s %d",
              program, programVersion, (int *) &iwut_State, &id_Seconds, &il_PrimesTested, Checksum, &hasWWWWs);

   if (countScanned != 7)
   {
      printf("Unable to parse line [%s] from save file.  Exiting\n", tempLine);
      exit(-1);
   }

   is_Program = program;
   is_ProgramVersion = programVersion;
   is_Checksum = Checksum;

   if (!hasWWWWs)
      return;

   while (fgets(line, BUFFER_SIZE, saveFile) != NULL)
   {
      if (!memcmp(line, "End WWWW", 8))
         return;

      if (memcmp(line, "WWWW: ", 6))
      {
         printf("Expected prefix [%s] on line [%s] was not found.  Exiting\n", prefix.c_str(), line);
         exit(-1);
      }

      wwwwNext = new wwww_t;
      if (!wwwwPrevious)
         ip_FirstWWWW = wwwwNext;
      else
         wwwwPrevious->m_NextWWWW = wwwwNext;
    
      if (sscanf(line+6, "%" PRId64" %d %d", &wwwwNext->l_Prime, &wwwwNext->i_Remainder, &wwwwNext->i_Quotient) != 3)
      {
         printf("Unable to scan line [%s] from save file.  Exiting\n", line);
         exit(-1);
      }

      if (wwwwNext->i_Remainder == 0 &&wwwwNext->i_Quotient == 0 )
         wwwwNext->t_WWWWType = WWWW_PRIME;
      else
         wwwwNext->t_WWWWType = WWWW_SPECIAL;

      wwwwNext->m_NextWWWW = NULL;
      wwwwPrevious = wwwwNext;
   }
}

void     WWWWWorkUnitTest::LogMessage(WorkUnitTest *masterWorkUnit)
{
   if (iwut_State == WUT_COMPLETED)
      ip_Log->LogMessage("%s: Range %" PRId64" to %" PRId64" completed", is_WorkSuffix.c_str(), il_LowerLimit, il_UpperLimit);
}
