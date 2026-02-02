#include "GFNDivisorWorkUnitTest.h"

GFNDivisorWorkUnitTest::GFNDivisorWorkUnitTest(Log* theLog, int32_t serverType, string workSuffix,
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
   ip_FirstGFNDivisor = nullptr;
   id_Seconds = 0.0;
}

GFNDivisorWorkUnitTest::~GFNDivisorWorkUnitTest()
{
   gfndivisor_t* gfndPtr;

   while (ip_FirstGFNDivisor)
   {
      gfndPtr = (gfndivisor_t*)ip_FirstGFNDivisor->m_NextGFNDivisor;
      delete ip_FirstGFNDivisor;
      ip_FirstGFNDivisor = gfndPtr;
   }
}

bool  GFNDivisorWorkUnitTest::TestWorkUnit(WorkUnitTest* masterWorkUnit)
{
   gfndivisor_t* gfndPtr;
   GFNDivisorProgram* gfndProgram;
   PFGWProgram* pfgwProgram;
   Log* testLog;
   time_t startTime, endTime;
   testresult_t   testResult;

   gfndProgram = ip_TestingProgramFactory->GetGFNDivisorProgram();
   pfgwProgram = ip_TestingProgramFactory->GetPFGWProgram();
   is_Program = gfndProgram->GetInternalProgramName();
   is_ProgramVersion = gfndProgram->GetProgramVersion();

   gfndProgram->SetSuffix(is_WorkSuffix);
   pfgwProgram->SetSuffix(is_WorkSuffix);
   iwut_State = WUT_INPROGRESS;

   startTime = time(nullptr);

   if (!DoesFileExist("gfnd.pfgw") && !DoesFileExist("gfnd_prp.pfgw"))
   {
      testResult = gfndProgram->Execute(ii_ServerType, ii_n, il_MinK, il_MaxK);
      if (testResult != TR_COMPLETED)
         return false;
   }

   if (DoesFileExist("gfnd.abcd"))
   {
      if (DoesFileExist("gfnd.pfgw"))
      {
         printf("Both gfnd.abcd and gfnd.pfgw exist.  Uncertain what the next step is.  Exiting\n");
         exit(-1);
      }

      rename("gfnd.abcd", "gfnd.pfgw");
   }

   if (DoesFileExist("gfnd.pfgw"))
   {
      testResult = pfgwProgram->PRPTest("gfnd.pfgw");
      if (testResult != TR_COMPLETED)
         return false;

      unlink("gfnd.pfgw");
   }

   // Although unlikely, it is possible that no PRPs are found in the range, so if
   // the program is terminated between finishing PRP testing and updating the .save
   // file, then the PRP test might be run a second time on gfnd.pfgw
   if (DoesFileExist("pfgw.log"))
   {
      if (!DoesFileExist("gfnd_prp.pfgw"))
         rename("pfgw.log", "gfnd_prp.pfgw");
   }

   if (DoesFileExist("gfnd_prp.pfgw"))
   {
      testResult = pfgwProgram->GFNDivisibilityTest("gfnd_prp.pfgw");
      if (testResult != TR_COMPLETED)
         return false;

      unlink("gfnd_prp.pfgw");
   }

   endTime = time(nullptr);

   ip_FirstGFNDivisor = pfgwProgram->GetGFNList();
   id_Seconds += (double) (endTime - startTime);

   testLog = new Log(0, "test_results.log", 0, false);
   testLog->LogMessage("Server: %s, N: %u  Range: %" PRIu64":%" PRIu64"  Program: %s  Time: %.0lf seconds",
      is_WorkSuffix.c_str(), ii_n, il_MinK, il_MaxK, is_Program.c_str(), id_Seconds);

   gfndPtr = ip_FirstGFNDivisor;
   while (gfndPtr)
   {
      testLog->LogMessage("Server: %s, Found: %s", is_WorkSuffix.c_str(), gfndPtr->s_Divisor);
      gfndPtr = (gfndivisor_t*)gfndPtr->m_NextGFNDivisor;
   }

   delete testLog;

   LogMessage(masterWorkUnit);

   DeleteIfExists("gfn.out");
   DeleteIfExists("pfgw.ini");
   DeleteIfExists("gfnd.log");
   DeleteIfExists("pfgw-prime.log");

   iwut_State = WUT_COMPLETED;
   return true;
}

void  GFNDivisorWorkUnitTest::SendResults(Socket* theSocket)
{
   gfndivisor_t* gfndPtr;

   gfndPtr = ip_FirstGFNDivisor;
   while (gfndPtr)
   {
      theSocket->Send("GFNDivisor: %s", gfndPtr->s_Divisor);
      gfndPtr = (gfndivisor_t*)gfndPtr->m_NextGFNDivisor;
   }
}

void     GFNDivisorWorkUnitTest::Save(FILE* saveFile)
{
   gfndivisor_t* gfndPtr;
   uint32_t divisorCount = 0;

   gfndPtr = ip_FirstGFNDivisor;
   while (gfndPtr)
   {
      divisorCount++;
      gfndPtr = (gfndivisor_t*)gfndPtr->m_NextGFNDivisor;
   }

   fprintf(saveFile, "%u %" PRIu64" %" PRIu64": %s %s %u %lf %u\n",
      ii_n, il_MinK, il_MaxK, is_Program.c_str(), is_ProgramVersion.c_str(),
      (int)iwut_State, id_Seconds, divisorCount);

   if (!divisorCount)
      return;

   gfndPtr = ip_FirstGFNDivisor;
   while (gfndPtr)
   {
      fprintf(saveFile, "GFNDivisor: %s\n", gfndPtr->s_Divisor);
      gfndPtr = (gfndivisor_t*)gfndPtr->m_NextGFNDivisor;
   }
}

void     GFNDivisorWorkUnitTest::Load(FILE* saveFile, string lineIn, string prefix)
{
   gfndivisor_t* gfndPrevious = 0, *gfndNext;
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
   ip_FirstGFNDivisor = nullptr;

   if (!divisorCount)
      return;

   while (fgets(line, BUFFER_SIZE, saveFile) != NULL)
   {
      if (memcmp(line, "GFNDivisor: ", 12))
      {
         printf("Expected prefix [%s] on line [%s] was not found.  Exiting\n", prefix.c_str(), line);
         exit(-1);
      }

      gfndNext = new gfndivisor_t;
      gfndNext->m_NextGFNDivisor = NULL;

      if (!ip_FirstGFNDivisor)
         ip_FirstGFNDivisor = gfndNext;
      else
         gfndPrevious->m_NextGFNDivisor = gfndNext;

      strcpy(gfndNext->s_Divisor, line + 12);

      gfndPrevious = gfndNext;
      if (--divisorCount == 0)
         break;
   }
}

void     GFNDivisorWorkUnitTest::LogMessage(WorkUnitTest* masterWorkUnit)
{
   if (iwut_State == WUT_COMPLETED)
      ip_Log->LogMessage("%s: n=%u  Range %" PRIu64" <= k <= %" PRIu64" completed", is_WorkSuffix.c_str(), ii_n, il_MinK, il_MaxK);
}
