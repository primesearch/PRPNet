#ifndef _CycloProgram_

#define _CycloProgram_

#include "TestingProgram.h"

class CycloProgram : public TestingProgram
{
public:
   CycloProgram(Log *theLog, string programName) :
      TestingProgram(theLog, programName) {};

   void        SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "cyclo"; };

   testresult_t   Execute(testtype_t testType);

   void        DetermineVersion(void);

private:
   testresult_t   ParseTestResults(testtype_t testType);
};

#endif // #ifndef _CycloProgram_

