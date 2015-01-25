#ifndef _SophieGermainWorkUnitTest_

#define _SophieGermainWorkUnitTest_

#include "PrimeWorkUnitTest.h"

class SophieGermainWorkUnitTest : public PrimeWorkUnitTest
{
public:
   SophieGermainWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                    workunit_t *wu, TestingProgramFactory *testingProgramFactory,
                    sgtype_t sgType);

   ~SophieGermainWorkUnitTest();

private:
   void     LogMessage(WorkUnitTest *masterWorkUnit);
   string   GetResultPrefix(void) { return (isg_Type == SG_NM1 ? SGNM1_PREFIX : SGNP1_PREFIX); };

   sgtype_t isg_Type;
};

#endif // #ifndef _SophieGermainWorkUnitTest_

