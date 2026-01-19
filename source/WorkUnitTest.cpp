#include "WorkUnitTest.h"

WorkUnitTest::WorkUnitTest(Log *theLog, int32_t serverType, string workSuffix, workunit_t *wu)
{
   ip_Log = theLog;
   ii_ServerType = serverType;
   is_WorkSuffix = workSuffix;
   is_Program = "na";
   is_ProgramVersion = "na";

   iwut_State = WUT_NOTSTARTED;
   iwut_Result = R_UNKNOWN;
   ip_NextWorkUnitTest = NULL;
   id_Seconds = 0.0;
}

bool WorkUnitTest::DoesFileExist(char* fileName)
{
   FILE* fp;

   fp = fopen(fileName, "r");
   if (!fp)
      return false;

   fclose(fp);
   return true;
}

void WorkUnitTest::DeleteIfExists(char* fileName)
{
   FILE* fp;

   fp = fopen(fileName, "r");
   if (!fp)
      return;

   fclose(fp);

   unlink(fileName);
}
