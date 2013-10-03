#ifndef _WWWWProgram_

#define _WWWWProgram_

#include "TestingProgram.h"
#include "wwww.h"

class WWWWProgram : public TestingProgram
{
public:
   WWWWProgram(Log *theLog, string programName)
      : TestingProgram(theLog, programName) { ip_FirstWWWW = NULL; };

   void        SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return is_ProgramName; };
   int64_t     GetPrimesTested(void) { return il_PrimesTested; };
   char       *GetChecksum(void) { return ic_Checksum; };

   testresult_t   Execute(testtype_t testType) { return TR_CANCELLED; };

   testresult_t   Execute(int32_t serverType, int32_t specialThreshhold, int64_t lowerLimit, int64_t upperLimit);

   void           DetermineVersion(void);

   wwww_t        *GetWWWWList(void) { return ip_FirstWWWW; };

private:
   testresult_t   ParseTestResults(testtype_t testType);

   void           AddWWWWToList(string line);

   wwww_t        *ip_FirstWWWW;

   string         is_ProgramName;
   int64_t        il_PrimesTested;
   char           ic_Checksum[20];
};

#endif // #ifndef _WWWWProgram_

