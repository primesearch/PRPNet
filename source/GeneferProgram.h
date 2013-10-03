#ifndef _GeneferProgram_

#define _GeneferProgram_

#include "TestingProgram.h"

class GeneferProgram : public TestingProgram
{
public:
   GeneferProgram(Log *theLog, string programName) :
      TestingProgram(theLog, programName) { ii_ProgramCount = 0; };

   uint32_t    ValidateExe(void);

   void        AddProgram(string programName);

   void        SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit);

   string      GetStandardizedName(void) { return "genefer"; };

   testresult_t   Execute(testtype_t testType);

   void        DetermineVersion(void);

private:
   testresult_t   ParseTestResults(testtype_t testType);

   string      is_ProgramList[MAX_GENEFERS];

   string      is_StandardizedName[MAX_GENEFERS];

   int32_t     ii_ProgramCount;

   int32_t     ii_RunIndex[MAX_GENEFERS];
};

#endif // #ifndef _GeneferProgram_

