#ifndef _WorkUnitTestFactory_

#define _WorkUnitTestFactory_

#include "Log.h"
#include "WorkUnitTest.h"
#include "TestingProgramFactory.h"
#include "workunit.h"

class WorkUnitTestFactory
{
public:
   WorkUnitTestFactory(Log *theLog, string workSuffix, TestingProgramFactory *testingProgramFactory);

   ~WorkUnitTestFactory();

   WorkUnitTest *BuildWorkUnitTestList(int32_t serverType, workunit_t *wu);

   void     LoadWorkUnitTest(FILE *saveFile, int32_t serverType,
                             workunit_t *wu, int32_t specialThreshhold = 0);

private:
   Log                     *ip_Log;
   TestingProgramFactory   *ip_TestingProgramFactory;
   string                   is_WorkSuffix;
};

#endif // #ifndef _WorkUnitTestFactory_
