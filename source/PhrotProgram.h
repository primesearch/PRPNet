#ifndef _PhrotProgram_

#define _PhrotProgram_

#include "TestingProgram.h"

class PhrotProgram : public TestingProgram
{
public:
   PhrotProgram(Log *theLog, string programName) :
      TestingProgram(theLog, programName) {};

   void        SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "phrot"; };

   testresult_t   Execute(testtype_t testType);

   void        DetermineVersion(void);

private:
   testresult_t   ParseTestResults(testtype_t testType);
};

#endif // #ifndef _PhrotProgram_

