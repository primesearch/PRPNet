#ifndef _DMDProgram_

#define _DMDProgram_

#include "TestingProgram.h"
#include "dmdivisor.h"

class DMDivisorProgram : public TestingProgram
{
public:
   DMDivisorProgram(Log* theLog, string programName) :
      TestingProgram(theLog, programName) {
   };

   void        SendStandardizedName(Socket* theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "dmdsieve"; };

   testresult_t   Execute(int32_t serverType, uint32_t n, uint64_t minK, uint64_t maxK);

   void        DetermineVersion(void);

   dmdivisor_t* GetDMDivisorList(void) { return ip_FirstDMDivisor; };

private:
   bool        GetLineWithTestResult(char* line, uint32_t lineLength);
   void        AddDMDivisorToList(char* divisor);

   testresult_t   ParseTestResults();

   string         is_ProgramName;
   dmdivisor_t   *ip_FirstDMDivisor;
};

#endif // #ifndef _DMDProgram_
