#ifndef _TwinWorkUnitTest_

#define _TwinWorkUnitTest_

#include "PrimeWorkUnitTest.h"

class TwinWorkUnitTest : public PrimeWorkUnitTest
{
public:
   TwinWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                    workunit_t *wu, TestingProgramFactory *testingProgramFactory);

   ~TwinWorkUnitTest();

private:
   void     LogMessage(WorkUnitTest *masterWorkUnit);
   string   GetResultPrefix(void) { return TWIN_PREFIX; };
};

#endif // #ifndef _TwinWorkUnitTest_

