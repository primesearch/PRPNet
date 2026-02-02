#include "DMDivisorWorkUnitTest.h"

DMDivisorWorkUnitTest::DMDivisorWorkUnitTest(Log* theLog, int32_t serverType, string workSuffix,
   workunit_t* wu, TestingProgramFactory* testingProgramFactory)
   : WorkUnitTest(theLog, serverType, workSuffix, wu)
{
   is_Program = "na";
   is_ProgramVersion = "na";
   ip_TestingProgramFactory = testingProgramFactory;

   ii_n = wu->i_n;
   il_MinK = wu->l_minK;
   il_MaxK = wu->l_maxK;

   iwut_State = WUT_NOTSTARTED;
   iwut_Result = R_UNKNOWN;
   ip_FirstDMDivisor = NULL;
   id_Seconds = 0.0;
}

DMDivisorWorkUnitTest::~DMDivisorWorkUnitTest()
{
   dmdivisor_t* dmdPtr;

   while (ip_FirstDMDivisor)
   {
      dmdPtr = (dmdivisor_t*)ip_FirstDMDivisor->m_NextDMDivisor;
      delete ip_FirstDMDivisor;
      ip_FirstDMDivisor = dmdPtr;
   }
}

bool  DMDivisorWorkUnitTest::TestWorkUnit(WorkUnitTest* masterWorkUnit)
{
   dmdivisor_t* dmdPtr;
   DMDivisorProgram* dmDivisorProgram;
   Log* testLog;
   time_t startTime, endTime;
   testresult_t   testResult;

   dmDivisorProgram = ip_TestingProgramFactory->GetDMDivisorProgram();
   is_Program = dmDivisorProgram->GetInternalProgramName();
   is_ProgramVersion = dmDivisorProgram->GetProgramVersion();

   dmDivisorProgram->SetSuffix(is_WorkSuffix);

   iwut_State = WUT_INPROGRESS;
   startTime = time(nullptr);

   testResult = dmDivisorProgram->Execute(ii_ServerType, ii_n, il_MinK, il_MaxK);
   if (testResult != TR_COMPLETED)
      return false;

   iwut_State = WUT_COMPLETED;

   endTime = time(nullptr);

   LogMessage(masterWorkUnit);

   ip_FirstDMDivisor = dmDivisorProgram->GetDMDivisorList();
   id_Seconds += (double)(endTime - startTime);

   testLog = new Log(0, "test_results.log", 0, false);
   testLog->LogMessage("Server: %s, n: %u  Range: %" PRIu64":%" PRIu64"  Program: %s  Time: %.0lf seconds",
      is_WorkSuffix.c_str(), ii_n, il_MinK, il_MaxK, is_Program.c_str(), id_Seconds);

   dmdPtr = ip_FirstDMDivisor;
   while (dmdPtr)
   {
      testLog->LogMessage("Server: %s, Found: %s divides 2^(2^%u-1)-1", is_WorkSuffix.c_str(), dmdPtr->s_Divisor, ii_n);
      dmdPtr = (dmdivisor_t*)dmdPtr->m_NextDMDivisor;
   }

   delete testLog;
   return true;
}

void  DMDivisorWorkUnitTest::SendResults(Socket* theSocket)
{
   dmdivisor_t* dmdPtr;

   dmdPtr = ip_FirstDMDivisor;
   while (dmdPtr)
   {
      theSocket->Send("DMDivisor: %s", dmdPtr->s_Divisor);
      dmdPtr = (dmdivisor_t*)dmdPtr->m_NextDMDivisor;
   }
}

void     DMDivisorWorkUnitTest::Save(FILE* saveFile)
{
   dmdivisor_t* dmdPtr;
   uint32_t divisorCount = 0;

   dmdPtr = ip_FirstDMDivisor;
   while (dmdPtr)
   {
      divisorCount++;
      dmdPtr = (dmdivisor_t*)dmdPtr->m_NextDMDivisor;
   }

   fprintf(saveFile, "%u %" PRIu64" %" PRIu64": %s %s %u %lf %u\n",
      ii_n, il_MinK, il_MaxK, is_Program.c_str(), is_ProgramVersion.c_str(),
      (int) iwut_State, id_Seconds, divisorCount);

   if (!divisorCount)
      return;

   dmdPtr = ip_FirstDMDivisor;
   while (dmdPtr)
   {
      fprintf(saveFile, "DMDivisor: %u %" PRIu64" %s\n", dmdPtr->n, dmdPtr->k, dmdPtr->s_Divisor);
      dmdPtr = (dmdivisor_t*)dmdPtr->m_NextDMDivisor;
   }
}

void     DMDivisorWorkUnitTest::Load(FILE* saveFile, string lineIn, string prefix)
{
   dmdivisor_t* dmdPrevious = 0, *dmdNext;
   char     line[500], * ptr;
   int      countScanned;
   int      divisorCount;
   char     program[100], programVersion[100];
   char     tempLine[BUFFER_SIZE];

   strcpy(tempLine, lineIn.c_str());
   ptr = strstr(tempLine, ": ");

   if (ptr == nullptr)
   {
      printf("Unable to parse line [%s] from save file.  Exiting\n", tempLine);
      exit(-1);
   }

   countScanned = sscanf(ptr + 2, "%s %s %u %lf %u",
      program, programVersion, (int*)&iwut_State, &id_Seconds, &divisorCount);

   if (countScanned != 5)
   {
      printf("Missing details on line [%s] from save file.  Exiting\n", tempLine);
      exit(-1);
   }

   is_Program = program;
   is_ProgramVersion = programVersion;
   ip_FirstDMDivisor = nullptr;

   if (!divisorCount)
      return;

   while (fgets(line, BUFFER_SIZE, saveFile) != NULL)
   {
      if (memcmp(line, "DMDivisor: ", 11))
      {
         printf("Expected prefix [%s] on line [%s] was not found.  Exiting\n", prefix.c_str(), line);
         exit(-1);
      }

      dmdNext = new dmdivisor_t;
      dmdNext->m_NextDMDivisor = NULL;

      if (!ip_FirstDMDivisor)
         ip_FirstDMDivisor = dmdNext;
      else
         dmdPrevious->m_NextDMDivisor = dmdNext;

      if (sscanf(line + 11, "%d %" PRId64" %s", &dmdNext->n, &dmdNext->k, dmdNext->s_Divisor) != 3)
      {
         printf("Unable to scan line [%s] from save file.  Exiting\n", line);
         exit(-1);
      }

      dmdPrevious = dmdNext;
      if (--divisorCount == 0)
         break;
   }
}

void     DMDivisorWorkUnitTest::LogMessage(WorkUnitTest* masterWorkUnit)
{
   if (iwut_State == WUT_COMPLETED)
      ip_Log->LogMessage("%s: n=%u : %" PRIu64" <= k <= %" PRIu64" completed", is_WorkSuffix.c_str(), ii_n, il_MinK, il_MaxK);
}
