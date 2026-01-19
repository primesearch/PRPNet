#ifndef _GFNDProgram_

#define _GFNDProgram_

#include "TestingProgram.h"

class GFNDivisorProgram : public TestingProgram
{
public:
   GFNDivisorProgram(Log* theLog, string programName) :
      TestingProgram(theLog, programName) {
   };

   void        SendStandardizedName(Socket* theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "gfndsieve"; };

   testresult_t   Execute(int32_t serverType, uint32_t n, uint64_t minK, uint64_t maxK);

   void        DetermineVersion(void);

private:
   bool        GetLineWithTestResult(char* line, uint32_t lineLength);

   testresult_t   ParseTestResults();

   string         is_ProgramName;
};

#endif // #ifndef _GFNDProgram_
