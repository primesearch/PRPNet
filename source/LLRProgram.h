#ifndef _LLRProgram_

#define _LLRProgram_

#include "TestingProgram.h"

class LLRProgram : public TestingProgram
{
public:
   LLRProgram(Log *theLog, string programName) :
      TestingProgram(theLog, programName) {};

   void        SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "llr"; };

   testresult_t   Execute(testtype_t testType);

   void        DetermineVersion(void);

private:
   testresult_t   ParseTestResults(testtype_t testType);
};

#endif // #ifndef _LLRProgram_

