#ifndef _WorkUnitTest_

#define _WorkUnitTest_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "Log.h"
#include "Socket.h"
#include "TestingProgramFactory.h"
#include "workunit.h"
#include "gfn.h"
#include "wwww.h"

enum wutstate_t { WUT_NOTSTARTED = 1, WUT_INPROGRESS, WUT_COMPLETED };

class WorkUnitTest;

class WorkUnitTest
{
public:
   WorkUnitTest(Log *theLog, int32_t serverType, string workSuffix, workunit_t *wu);

   virtual ~WorkUnitTest() {};

   string   GetProgram(void)           { return is_Program;         };
   string   GetProgramVersion(void)    { return is_ProgramVersion;  };
   double   GetSeconds(void)           { return id_Seconds;         };
   int32_t  GetDecimalLength(void)     { return ii_DecimalLength;   };

   virtual bool     TestWorkUnit(WorkUnitTest *masterWorkUnit) { return false; };
   virtual void     SendResults(Socket *theSocket) {};
   virtual void     Save(FILE *saveFile) {};
   virtual void     Load(FILE *saveFile, string lineIn, string prefix) {};

   void           SetNextWorkUnitTest(WorkUnitTest *nextWorkUnitTest) { ip_NextWorkUnitTest = nextWorkUnitTest; };
   WorkUnitTest  *GetNextWorkUnitTest(void)   { return ip_NextWorkUnitTest; };
   wutstate_t     GetWorkUnitTestState(void)  { return iwut_State;  };
   result_t       GetWorkUnitTestResult(void) { return iwut_Result; };

protected:
   virtual void   LogMessage(WorkUnitTest *parentWorkUnit) {};
   virtual string GetResultPrefix(void) { return ""; };

   Log     *ip_Log;
   int32_t  ii_ServerType;
   string   is_WorkSuffix;

   string   is_ProgramVersion;
   string   is_Program;
   double   id_Seconds;

   // Computed only for generic servers
   int32_t  ii_DecimalLength;

   wutstate_t  iwut_State;
   result_t    iwut_Result;

   WorkUnitTest          *ip_NextWorkUnitTest;
   TestingProgramFactory *ip_TestingProgramFactory;
};

#endif // #ifndef _WorkUnitTest_

