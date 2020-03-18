#include "WorkUnitTestFactory.h"
#include "MainWorkUnitTest.h"
#include "TwinWorkUnitTest.h"
#include "SophieGermainWorkUnitTest.h"
#include "WWWWWorkUnitTest.h"

WorkUnitTestFactory::WorkUnitTestFactory(Log *theLog, string workSuffix, TestingProgramFactory *testingProgramFactory)
{
   ip_Log = theLog;
   ip_TestingProgramFactory = testingProgramFactory;
   is_WorkSuffix = workSuffix;
}

WorkUnitTestFactory::~WorkUnitTestFactory()
{
}

WorkUnitTest *WorkUnitTestFactory::BuildWorkUnitTestList(int32_t serverType, workunit_t *wu)
{
   WorkUnitTest *wuFirst, *wuPrevious, *wuNext;

   wuFirst = new MainWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory);

   wuPrevious = wuFirst;

   if (serverType == ST_TWIN)
   {
      wuNext = new TwinWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory);
      wuPrevious->SetNextWorkUnitTest(wuNext);
      wuPrevious = wuNext;
   }

   if (serverType == ST_SOPHIEGERMAIN)
   {
      wuNext = new SophieGermainWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory, SG_NM1);
      wuPrevious->SetNextWorkUnitTest(wuNext);
      wuPrevious = wuNext;

      wuNext = new SophieGermainWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory, SG_NP1);
      wuPrevious->SetNextWorkUnitTest(wuNext);
      wuPrevious = wuNext;
   }

   return wuFirst;
}

void  WorkUnitTestFactory::LoadWorkUnitTest(FILE *saveFile, int32_t serverType,
                                            workunit_t *wu, int32_t specialThreshhold)
{
   char          *line, *ptr;
   char           endWorkUnit[100], prefix[50];
   WorkUnitTest  *workUnitTest = NULL, *previousWorkUnitTest = NULL;

   wu->m_FirstWorkUnitTest = NULL;
   line = new char[BUFFER_SIZE];
   sprintf(endWorkUnit, "End WorkUnit %" PRId64" %s", wu->l_TestID, wu->s_Name);

   while (fgets(line, BUFFER_SIZE, saveFile) != NULL)
   {
      StripCRLF(line);

      if (!strcmp(line, endWorkUnit))
         break;
   
      ptr = strstr(line, ": ");
      if (!ptr)
      {
         printf("Missing ':' on line [%s] from save file.  Exiting\n", line);
         exit(-1);
      }

      *ptr = 0;
      strcpy(prefix, line);
      *ptr = ':';

      workUnitTest = NULL;
      if (serverType == ST_WIEFERICH || serverType == ST_WILSON ||
          serverType == ST_WALLSUNSUN || serverType == ST_WOLSTENHOLME)
          workUnitTest = new WWWWWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, specialThreshhold, ip_TestingProgramFactory);
      else
      {
         if (!strcmp(prefix, MAIN_PREFIX))
            workUnitTest = new MainWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory);

         if (!strcmp(prefix, TWIN_PREFIX))
            workUnitTest = new TwinWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory);

         if (!strcmp(prefix, SGNM1_PREFIX))
            workUnitTest = new SophieGermainWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory, SG_NM1);

         if (!strcmp(prefix, SGNP1_PREFIX))
            workUnitTest = new SophieGermainWorkUnitTest(ip_Log, serverType, is_WorkSuffix, wu, ip_TestingProgramFactory, SG_NP1);
      }

      if (!workUnitTest)
      {
         printf("Prefix [%s] on line [%s] from save file is unknown.  Exiting\n", prefix, line);
         exit(-1);
      }

      if (!wu->m_FirstWorkUnitTest)
         wu->m_FirstWorkUnitTest = workUnitTest;
	  else
	  {
		  if (previousWorkUnitTest != NULL)
			  previousWorkUnitTest->SetNextWorkUnitTest(workUnitTest);
	  }

      previousWorkUnitTest = workUnitTest;
      workUnitTest->Load(saveFile, line, prefix);
   }

   delete [] line;
}
