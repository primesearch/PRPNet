#ifndef _GFNDivisorWorkUnitTest_

#define _GFNDivisorWorkUnitTest_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "WorkUnitTest.h"
#include "gfndivisor.h"

class WorkUnitTest;

class GFNDivisorWorkUnitTest : public WorkUnitTest
{
public:
   GFNDivisorWorkUnitTest(Log* theLog, int32_t serverType, string workSuffix,
      workunit_t* wu, TestingProgramFactory* testingProgramFactory);

   ~GFNDivisorWorkUnitTest();

   bool     TestWorkUnit(WorkUnitTest* masterWorkUnit);
   void     SendResults(Socket* theSocket);
   void     Save(FILE* saveFile);
   void     Load(FILE* saveFile, string lineIn, string prefix);

   gfndivisor_t* GetGFNDivisors() { return ip_FirstGFNDivisor; }

protected:
   void     LogMessage(WorkUnitTest* parentWorkUnit);
   string   GetResultPrefix(void) { return ""; };

   // This represents the number being tested
   uint32_t ii_n;
   uint64_t il_MinK;
   uint64_t il_MaxK;

   gfndivisor_t* ip_FirstGFNDivisor;
private:
};

#endif // #ifndef _GFNDivisorWorkUnitTest_
