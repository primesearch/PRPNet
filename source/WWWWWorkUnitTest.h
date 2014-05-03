#ifndef _WWWWWorkUnitTest_

#define _WWWWWorkUnitTest_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "WorkUnitTest.h"
#include "wwww.h"

class WorkUnitTest;

class WWWWWorkUnitTest : public WorkUnitTest
{
public:
   WWWWWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                    workunit_t *wu, int32_t specialThreshhold,
                    TestingProgramFactory *testingProgramFactory);

   ~WWWWWorkUnitTest();

   wwww_t  *GetWWWWList(void)       { return ip_FirstWWWW; };
   int64_t  GetPrimesTested(void)   { return il_PrimesTested; };
   string   GetCheckSum(void)       { return is_Checksum; };

   bool     TestWorkUnit(WorkUnitTest *masterWorkUnit);
   void     SendResults(Socket *theSocket);
   void     Save(FILE *saveFile);
   void     Load(FILE *saveFile, string lineIn, string prefix);

protected:
   void     LogMessage(WorkUnitTest *parentWorkUnit);
   string   GetResultPrefix(void) { return ""; };

   // This represents the number being tested
   int64_t  il_LowerLimit;
   int64_t  il_UpperLimit;
   int32_t  ii_SpecialThreshhold;

   int64_t  il_PrimesTested;
   string   is_Checksum;

private:
   wwww_t     *ip_FirstWWWW;
};

#endif // #ifndef _WWWWWorkUnitTest_
