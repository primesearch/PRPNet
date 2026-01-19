#ifndef _DMDivisorWorkUnitTest_

#define _DMDivisorWorkUnitTest_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "WorkUnitTest.h"
#include "dmdivisor.h"

class WorkUnitTest;

class DMDivisorWorkUnitTest : public WorkUnitTest
{
public:
   DMDivisorWorkUnitTest(Log* theLog, int32_t serverType, string workSuffix,
      workunit_t* wu, TestingProgramFactory* testingProgramFactory);

   ~DMDivisorWorkUnitTest();

   dmdivisor_t* GetDMDivisors(void) { return ip_FirstDMDivisor; };

   bool     TestWorkUnit(WorkUnitTest* masterWorkUnit);
   void     SendResults(Socket* theSocket);
   void     Save(FILE* saveFile);
   void     Load(FILE* saveFile, string lineIn, string prefix);

protected:
   void     LogMessage(WorkUnitTest* parentWorkUnit);
   string   GetResultPrefix(void) { return ""; };

   // This represents the number being tested
   uint32_t ii_n;
   uint64_t il_MinK;
   uint64_t il_MaxK;

private:
   dmdivisor_t* ip_FirstDMDivisor;
};

#endif // #ifndef _DMDivisorWorkUnitTest_
