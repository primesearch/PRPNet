#include "TestingProgram.h"

TestingProgram::TestingProgram(Log *theLog, string programName)
{
   ip_Log = theLog;

   is_ExeName = programName;

   if (programName.find("./") == string::npos)
      is_InternalProgramName = programName;
   else
      is_InternalProgramName = programName.substr(2);

   is_ProgramVersion = "unknown";
   ib_IsPRP = ib_IsPrime = ib_TestFailure = false;
   is_Residue.clear();
   ip_FirstGFN = 0;
   ii_Affinity = -1;
   ii_NormalPriority = 0;
}

void  TestingProgram::SetNumber(int32_t serverType, string suffix, string workUnitName,
                                int64_t theK, int32_t theB, int32_t theN, int32_t theC)
{
   is_Suffix = suffix;
   is_InFileName = "work_" + suffix + ".in";
   is_OutFileName = "work_" + suffix + ".out";

   ii_ServerType = serverType;
   is_WorkUnitName = workUnitName;

   ib_IsPRP = ib_IsPrime = ib_TestFailure = false;
   is_Residue.clear();
   ip_FirstGFN = 0;

   il_k = theK;
   ii_b = theB;
   ii_n = theN;
   ii_c = theC;
}

bool  TestingProgram::ValidateExe(void)
{
   struct stat buf;

   if (is_ExeName == "")
      return false;

   if (stat(is_ExeName.c_str(), &buf) == -1)
   {
      printf("Could not find executable '%s'.\n", is_ExeName.c_str());
      return false;
   }

   DetermineVersion();
   return true;
}
