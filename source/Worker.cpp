#include <signal.h>
#include <ctype.h>

#include "Worker.h"
#include "WorkUnitTest.h"

Worker::Worker(Log *theLog, TestingProgramFactory *testingProgramFactory,
               ClientSocket *theSocket, int32_t maxWorkUnits, string workSuffix)
{
   ip_Log = theLog;
   ip_Socket = theSocket;
   ii_MaxWorkUnits = maxWorkUnits;
   is_WorkSuffix = workSuffix;
   ip_FirstWorkUnit = NULL;
   ii_CompletedWorkUnits = ii_CurrentWorkUnits = 0;

   ip_TestingProgramFactory = testingProgramFactory;

   ip_WorkUnitTestFactory = new WorkUnitTestFactory(ip_Log, is_WorkSuffix, testingProgramFactory);
}

Worker::~Worker()
{
   delete ip_WorkUnitTestFactory;

   workunit_t  *wu, *wuNext;

   wu = ip_FirstWorkUnit;
   while (wu)
   {
      wuNext = (workunit_t *) wu->m_NextWorkUnit;
      DeleteWorkUnit(wu);
      wu = wuNext;
   }
}

void     Worker::DeleteWorkUnit(workunit_t *wu)
{
   WorkUnitTest *wuTest, *wuTestNext;

   ip_Log->Debug(DEBUG_WORK, "%s: DeleteWorkUnit for %s", is_WorkSuffix.c_str(), wu->s_Name);

   wuTest = (WorkUnitTest *) wu->m_FirstWorkUnitTest;
   while (wuTest)
   {
      wuTestNext = wuTest->GetNextWorkUnitTest();
      delete wuTest;
      wuTest = wuTestNext;
   }

   delete wu;
}

workunit_t  *Worker::GetNextIncompleteWorkUnit(bool inProgressOnly)
{
   workunit_t   *wu;

   ip_Log->Debug(DEBUG_WORK, "%s: GetNextIncompleteWorkUnit looking for work.  In Progress=%s",
                 is_WorkSuffix.c_str(), (inProgressOnly ? "true" : "false"));

   wu = ip_FirstWorkUnit;
   while (wu)
   {
      if (inProgressOnly)
      {
         if (IsWorkUnitInProgress(wu))
            break;
      }
      else
      {
         if (!IsWorkUnitCompleted(wu))
            break;
      }

      wu = (workunit_t *) wu->m_NextWorkUnit;

      if (!wu)
      {
         ip_Log->Debug(DEBUG_WORK, "%s: GetNextIncompleteWorkUnit found nothing to test", is_WorkSuffix.c_str());
         return NULL;
      }
   };

   ip_Log->Debug(DEBUG_WORK, "%s: GetNextIncompleteWorkUnit found %s", is_WorkSuffix.c_str(), wu->s_Name);

   return wu;
}

// Add the Worker Unit to the linked list
void  Worker::AddWorkUnitToList(workunit_t *wu)
{
   workunit_t *wuNext;

   ip_Log->Debug(DEBUG_WORK, "%s: AddWorkUnitToList for %s", is_WorkSuffix.c_str(), wu->s_Name);

   wu->m_NextWorkUnit = 0;

   if (IsWorkUnitCompleted(wu))
      ii_CompletedWorkUnits++;

   ii_CurrentWorkUnits++;

   if (!ip_FirstWorkUnit)
      ip_FirstWorkUnit = wu;
   else
   {
      wuNext = ip_FirstWorkUnit;
      while (wuNext->m_NextWorkUnit)
         wuNext = (workunit_t *) wuNext->m_NextWorkUnit;

      wuNext->m_NextWorkUnit = wu;
   }
}

bool  Worker::CanDoAnotherTest(void)
{
   workunit_t *wu;

   wu = ip_FirstWorkUnit;
   while (wu)
   {
      if (!IsWorkUnitCompleted(wu))
      {
         ip_Log->Debug(DEBUG_WORK, "%s: CanDoAnotherTest for %s.  Returning true (not done)",
                       is_WorkSuffix.c_str(), wu->s_Name);
         return true;
      }

      wu = (workunit_t *) wu->m_NextWorkUnit;
   };

   ip_Log->Debug(DEBUG_WORK, "%s: CanDoAnotherTest.  Returning false (all done)", is_WorkSuffix.c_str());
   return false;
}

bool  Worker::HaveInProgressTest(void)
{
   workunit_t *wu;

   wu = ip_FirstWorkUnit;
   while (wu)
   {
      if (IsWorkUnitInProgress(wu))
      {
         ip_Log->Debug(DEBUG_WORK, "%s: HaveInProgressTest for %s.  Returning true (in progress)",
                       is_WorkSuffix.c_str(), wu->s_Name);
         return true;
      }

      wu = (workunit_t *) wu->m_NextWorkUnit;
   };

   ip_Log->Debug(DEBUG_WORK, "%s: CanDoAnotherTest.  Returning false (none in progress)", is_WorkSuffix.c_str());
   return false;
}

void  Worker::ReturnWork(uint32_t returnOption, ClientSocket *theSocket)
{
   ClientSocket *temp;

   temp = ip_Socket;
   ip_Socket = theSocket;
   ReturnWork(returnOption);
   ip_Socket = temp;
}
