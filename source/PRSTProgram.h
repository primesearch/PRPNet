#ifndef _PRSTProgram_

#define _PRSTProgram_

#include "TestingProgram.h"

class PRSTProgram : public TestingProgram
{
public:
   PRSTProgram(Log* theLog, string programName) :
      TestingProgram(theLog, programName) {};

   void        SendStandardizedName(Socket* theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "prst"; };

   testresult_t   Execute(testtype_t testType);

   void        DetermineVersion(void);

private:
   testresult_t   ParseTestResults(testtype_t testType);
};

#endif // #ifndef _PRSTProgram_

