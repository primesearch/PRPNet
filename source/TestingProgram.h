#ifndef _TestingProgram_

#define _TestingProgram_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include "defs.h"
#include "Log.h"
#include "gfndivisor.h"
#include "Socket.h"

#ifdef WIN32
#pragma warning( disable : 4100 )
#endif

enum testtype_t { TT_PRP = 1, TT_PRIMALITY, TT_GFN, TT_WWWW };

enum testresult_t { TR_COMPLETED = 1, TR_CANCELLED } ;

class TestingProgram
{
public:
   TestingProgram(Log *theLog, string programName);

   inline TestingProgram() {};

   virtual ~TestingProgram() {};

   bool     IsPRP(void) { return ib_IsPRP; };

   bool     IsPrime(void) { return ib_IsPrime; };

   bool     HadTestFailure(void) { return ib_TestFailure; };

   void     SetNumber(int32_t serverType, string suffix, string workUnitName,
                      uint64_t theK, uint32_t theB, uint32_t theN, int64_t theC, uint32_t theD);

   testresult_t   Execute(testtype_t testType) { return TR_CANCELLED; };

   testresult_t   ParseTestResults(testtype_t testType) { return TR_CANCELLED; };

   string      GetInternalProgramName(void) { return is_InternalProgramName; };

   string      GetProgramVersion(void) { return is_ProgramVersion; };

   string      GetResidue(void) { return is_Residue; };

   double      GetSeconds(void) { return id_Seconds; };

   int32_t     GetDecimalLength(void) { return ii_DecimalLength; };

   gfndivisor_t      *GetGFNList(void) { return ip_FirstGFNDivisor; };

   bool        ValidateExe(void);

   void        SetNormalPriority(int32_t normalPriority) { ii_NormalPriority = normalPriority; };

   void        SetCpuAffinity(string cpuAffinity) { is_CpuAffinity = cpuAffinity; };

   void        SetGpuAffinity(int32_t gpuAffinity) { ii_GpuAffinity = gpuAffinity; };

   virtual void SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit) {};

   virtual string GetStandardizedName(void) { return ""; };

   virtual void DetermineVersion(void) {};

   bool         DoesFileExist(const char* fileName);

   void         SetSuffix(string suffix) { is_Suffix = suffix; };

protected:
   Log        *ip_Log;

   string      is_ExeName;
   string      is_ExeArguments;

   string      is_Suffix;
   string      is_InFileName;
   string      is_OutFileName;

   string      is_CpuAffinity;
   int32_t     ii_GpuAffinity;
   int32_t     ii_ServerType;
   string      is_WorkUnitName;

   // The k, b, n, c, and d values for the number to be tested
   uint64_t    il_k;
   uint32_t    ii_b;
   uint32_t    ii_n;
   int64_t     il_c;
   uint32_t    ii_d;

   // Only computed for generic servers
   int32_t     ii_DecimalLength;

   int32_t     ii_NormalPriority;

   string      is_InternalProgramName;
   string      is_ProgramVersion;
   string      is_Residue;
   double      id_Seconds;

   bool        ib_IsPrime;
   bool        ib_IsPRP;
   bool        ib_TestFailure;
   bool        ib_DeleteCheckpoint;

   gfndivisor_t      *ip_FirstGFNDivisor;
};

#endif // #ifndef _TestingProgram_
