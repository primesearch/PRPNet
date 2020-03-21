#ifndef _Worker_

#define _Worker_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "Log.h"
#include "Socket.h"
#include "ClientSocket.h"
#include "TestingProgramFactory.h"
#include "WorkUnitTestFactory.h"
#include "workunit.h"
#include "gfn.h"

// Return options
#define RO_UNDEFINED       0
#define RO_ALL             1           // Return all workunits, even if in progress or not started
#define RO_COMPLETED       2           // Return workunits that are done
#define RO_IFDONE          3           // Return all only if all workunits are completed

class Worker
{
public:
   Worker(Log *theLog, TestingProgramFactory *testingProgramFactory,
          ClientSocket *theSocket, int32_t maxWorkUnits, string workSuffix);

   virtual ~Worker();

   // Return unique identifier for this class instance
   string  GetWorkSuffix() { return is_WorkSuffix; };

   // Process another work unit
   virtual bool ProcessWorkUnit(int32_t &specialsFound, bool inProgressOnly, double &seconds) { return false; };

   void     ReturnWork(uint32_t returnOption, ClientSocket *theSocket);

   virtual bool GetWork(void) { return false; };
   virtual void ReturnWork(uint32_t returnOption) {};

   int32_t  GetCurrentWorkUnits(void) { return ii_CurrentWorkUnits; };
   int32_t  GetCompletedWorkUnits(void) { return ii_CompletedWorkUnits; };

   virtual void Load(string saveFileName) {};
   virtual void Save(FILE *fPtr) {};

   bool     HaveInProgressTest(void);
   bool     CanDoAnotherTest(void);

protected:
   Log     *ip_Log;
   ClientSocket *ip_Socket;
   TestingProgramFactory *ip_TestingProgramFactory;
   WorkUnitTestFactory *ip_WorkUnitTestFactory;

   string   is_WorkSuffix;

   workunit_t *ip_FirstWorkUnit;

   int32_t  ii_MaxWorkUnits;
   int32_t  ii_CurrentWorkUnits;
   int32_t  ii_CompletedWorkUnits;

   workunit_t  *GetNextIncompleteWorkUnit(bool inProgressOnly);

   void     DeleteWorkUnit(workunit_t *wu);

   bool     ReturnWorkUnit(workunit_t *wu, bool completed);

   uint32_t TestWorkUnit(workunit_t *wu, uint32_t isTwinTest);

   void     AddWorkUnitToList(workunit_t *wu);

   virtual bool IsWorkUnitCompleted(workunit_t *wu) { return false; };
   virtual bool IsWorkUnitInProgress(workunit_t *wu) { return false; };
};

#endif // #ifndef _Work_

