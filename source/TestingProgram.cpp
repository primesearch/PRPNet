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
   is_CpuAffinity = "";
   ii_NormalPriority = 0;
   ii_GpuAffinity = -1;
}

void  TestingProgram::SetNumber(int32_t serverType, string suffix, string workUnitName,
                                uint64_t theK, uint32_t theB, uint32_t theN, int32_t theC, uint32_t theD)
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
   ii_d = theD;
}

bool  TestingProgram::ValidateExe(void)
{
   struct stat buf;
   char   exeName[100];

   if (is_ExeName == "")
      return false;

   is_ExeArguments = "";
   snprintf(exeName, sizeof(exeName), "%s", is_ExeName.c_str());

   char* ptr = strchr(exeName, ' ');

   if (ptr)
   {
      *ptr = 0;
      is_ExeName = exeName;

      if (is_ExeName.find("./") == string::npos)
         is_InternalProgramName = is_ExeName;
      else
         is_InternalProgramName = is_ExeName.substr(2);

      is_ExeArguments = ptr + 1;
   }

   if (stat(exeName, &buf) == -1)
   {
      printf("Could not find executable '%s'.\n", exeName);
      return false;
   }

   DetermineVersion();
   return true;
}
