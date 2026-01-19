#ifndef _PFGWProgram_

#define _PFGWProgram_

#include "TestingProgram.h"

class PFGWProgram : public TestingProgram
{
public:
   PFGWProgram(Log *theLog, string programName) :
      TestingProgram(theLog, programName) {};

   void        SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "pfgw"; };

   testresult_t   Execute(testtype_t testType);

   void        DetermineVersion(void);

   testresult_t   PRPTest(char* fileName);

   testresult_t   GFNDivisibilityTest(char* fileName);

private:
   testresult_t   ParseTestResults(testtype_t testType);

   void        AddGFNToList(int32_t n, int64_t k, char *divisor);

   void        DetermineDecimalLength(void);
};

#endif // #ifndef _PFGWProgram_

