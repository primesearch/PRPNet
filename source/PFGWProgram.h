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

private:
   testresult_t   ParseTestResults(testtype_t testType);

   void        AddGFNToList(string divisor);
};

#endif // #ifndef _PFGWProgram_

