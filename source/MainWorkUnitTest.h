#ifndef _MainWorkUnitTest_

#define _MainWorkUnitTest_

#include "PrimeWorkUnitTest.h"

class MainWorkUnitTest : public PrimeWorkUnitTest
{
public:
   MainWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                    workunit_t *wu, TestingProgramFactory *testingProgramFactory);

   ~MainWorkUnitTest();

private:
   void     LogMessage(WorkUnitTest *masterWorkUnit);
   string   GetResultPrefix(void) { return MAIN_PREFIX; };
};

#endif // #ifndef _MainWorkUnitTest_

