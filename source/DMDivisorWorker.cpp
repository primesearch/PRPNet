#include <signal.h>
#include <ctype.h>

#include "DMDivisorWorker.h"
#include "DMDivisorWorkUnitTest.h"

DMDivisorWorker::DMDivisorWorker(Log* theLog, TestingProgramFactory* testingProgramFactory,
   ClientSocket* theSocket, int32_t maxWorkUnits, string workSuffix)
   : Worker(theLog, testingProgramFactory, theSocket, maxWorkUnits, workSuffix)
{
   ii_ServerType = 0;
   is_ServerVersion = "";
}

bool  DMDivisorWorker::ProcessWorkUnit(int32_t& specialsFound, bool inProgressOnly, double& seconds)
{
   bool          wasCompleted;
   workunit_t* wu;
   DMDivisorWorkUnitTest* wuTest, * wuTestNext;
   dmdivisor_t* dmdPtr;

   seconds = 0.0;

   specialsFound = 0;

   wu = GetNextIncompleteWorkUnit(inProgressOnly);
   if (!wu) return false;

   wuTest = (DMDivisorWorkUnitTest*)wu->m_FirstWorkUnitTest;
   while (wuTest)
   {
      wuTestNext = (DMDivisorWorkUnitTest*)wuTest->GetNextWorkUnitTest();

      wasCompleted = wuTest->TestWorkUnit((WorkUnitTest*)wu->m_FirstWorkUnitTest);

      if (!wasCompleted)
      {
         ip_Log->Debug(DEBUG_WORK, "%s: ProcessWorkUnit did not complete %s", is_WorkSuffix.c_str(), wu->s_Name);
         return false;
      }

      wuTest = wuTestNext;
   }

   ii_CompletedWorkUnits++;

   wuTest = (DMDivisorWorkUnitTest*)wu->m_FirstWorkUnitTest;

   dmdPtr = wuTest->GetDMDivisors();
   while (dmdPtr)
   {
      specialsFound++;
      dmdPtr = (dmdivisor_t*)dmdPtr->m_NextDMDivisor;
   }

   seconds = wuTest->GetSeconds();

   ip_Log->Debug(DEBUG_WORK, "%s: ProcessWorkUnit completed testing %s", is_WorkSuffix.c_str(), wu->s_Name);
   return true;
}

bool     DMDivisorWorker::GetWork(void)
{
   char* readBuf;
   int32_t     endLoop;
   int32_t     toScan, wasScanned;
   workunit_t* wu;

   if (ii_CurrentWorkUnits)
      return false;

   ip_Log->LogMessage("%s: Getting work from server %s at port %d",
      is_WorkSuffix.c_str(), ip_Socket->GetServerName().c_str(), ip_Socket->GetPortID());

   ip_Socket->Send("GETWORK %s %d", PRPNET_VERSION, ii_MaxWorkUnits);
   ip_TestingProgramFactory->SendPrograms(ip_Socket);
   ip_Socket->Send("End of Message");

   ii_CompletedWorkUnits = ii_CurrentWorkUnits = 0;
   endLoop = false;
   ii_ServerType = ip_Socket->GetServerType();
   is_ServerVersion = ip_Socket->GetServerVersion();

   readBuf = ip_Socket->Receive(60);
   while (readBuf && !endLoop)
   {
      // Keepalive tells the client that the server is still searching for
      // workunits to send.
      if (!memcmp(readBuf, "keepalive", 9))
         ;
      else if (!memcmp(readBuf, "WorkUnit: ", 10))
      {
         wu = new workunit_t;
         wu->m_FirstWorkUnitTest = 0;

         toScan = 4;
         wasScanned = sscanf(readBuf, "WorkUnit: %u %" PRIu64" %" PRIu64" %" PRIu64"",
            &wu->i_n, &wu->l_minK, &wu->l_maxK, &wu->l_TestID);

         if (toScan == wasScanned)
         {
            snprintf(wu->s_Name, sizeof(wu->s_Name), "%u_%" PRIu64"_%" PRIu64"", wu->i_n, wu->l_minK, wu->l_maxK);
            wu->m_FirstWorkUnitTest = new DMDivisorWorkUnitTest(ip_Log, ii_ServerType, is_WorkSuffix, wu,
               ip_TestingProgramFactory);
            AddWorkUnitToList(wu);
         }
         else
         {
            delete wu;
            ip_Log->LogMessage("%s: Could not parse WorkUnit [%s]", is_WorkSuffix.c_str(), readBuf);
         }
      }
      else if (!memcmp(readBuf, "End of Message", 14))
         endLoop = true;
      else if (!memcmp(readBuf, "INFO", 4))
         ip_Log->TestMessage("%s: %s", is_WorkSuffix.c_str(), readBuf);
      else if (!memcmp(readBuf, "INACTIVE", 8))
      {
         ip_Log->LogMessage("%s: No active candidates found on server", is_WorkSuffix.c_str());
         return false;
      }
      else if (!memcmp(readBuf, "ERROR", 5))
      {
         ip_Log->LogMessage("%s: %s", is_WorkSuffix.c_str(), readBuf);
         return false;
      }
      else
      {
         ip_Log->LogMessage("%s: GetWork error.  Message [%s] cannot be parsed", is_WorkSuffix.c_str(), readBuf);
         return false;
      }

      if (endLoop)
         break;

      readBuf = ip_Socket->Receive(60);
   }

   if (ii_CurrentWorkUnits == 0)
      return false;

   ip_Log->LogMessage("%s: PRPNet server is version %s", is_WorkSuffix.c_str(), ip_Socket->GetServerVersion().c_str());

   return true;
}

void  DMDivisorWorker::ReturnWork(uint32_t returnOption)
{
   char* readBuf;
   bool        completed, accepted;
   workunit_t* wu, * wuNext;

   ip_Log->LogMessage("%s: Returning work to server %s at port %d",
      is_WorkSuffix.c_str(), ip_Socket->GetServerName().c_str(), ip_Socket->GetPortID());

   wu = ip_FirstWorkUnit;

   ip_FirstWorkUnit = 0;
   ii_CompletedWorkUnits = ii_CurrentWorkUnits = 0;

   ip_Socket->Send("RETURNWORK %s", PRPNET_VERSION);

   accepted = true;
   while (wu)
   {
      wuNext = (workunit_t*)wu->m_NextWorkUnit;
      wu->m_NextWorkUnit = 0;

      completed = IsWorkUnitCompleted(wu);

      // If the workunit is not completed and we are to only report
      // completed workunits, then add this one back to the list
      if (!accepted || (!completed && returnOption == RO_COMPLETED))
      {
         AddWorkUnitToList(wu);
         wu = wuNext;
         continue;
      }

      // Report the workunit in its current state to the server.  If the server
      // does not accept the workunit, then add it back to the list
      accepted = ReturnWorkUnit(wu, completed);

      if (!accepted)
         AddWorkUnitToList(wu);
      else
         DeleteWorkUnit(wu);

      wu = wuNext;
   }

   ip_Socket->Send("End of Message");

   readBuf = ip_Socket->Receive(2);

   while (readBuf)
   {
      if (!memcmp(readBuf, "End of Message", 14))
         break;

      ip_Log->LogMessage("%s: %s", is_WorkSuffix.c_str(), readBuf);

      readBuf = ip_Socket->Receive(2);
   }
}

// Return a single work unit to the server
bool  DMDivisorWorker::ReturnWorkUnit(workunit_t* wu, bool completed)
{
   char* theMessage;
   bool        accepted;
   DMDivisorWorkUnitTest* wuTest, * wuNext;

   wuTest = (DMDivisorWorkUnitTest*)wu->m_FirstWorkUnitTest;
   ip_Socket->StartBuffering();

   ip_Socket->Send("WorkUnit: %u %" PRIu64" %" PRIu64" %" PRIu64"", wu->i_n, wu->l_minK, wu->l_maxK, wu->l_TestID);

   if (!completed)
      ip_Socket->Send("Test Abandoned");
   else
   {
      ip_Socket->Send("Stats: %s %s %lf", wuTest->GetProgram().c_str(), wuTest->GetProgramVersion().c_str(),
         wuTest->GetSeconds());

      while (wuTest)
      {
         wuNext = (DMDivisorWorkUnitTest*)wuTest->GetNextWorkUnitTest();
         wuTest->SendResults(ip_Socket);
         wuTest = wuNext;
      }
   }

   ip_Socket->Send("End of WorkUnit: %u %" PRIu64" %" PRIu64" %" PRIu64" ", wu->i_n, wu->l_minK, wu->l_maxK, wu->l_TestID);
   ip_Socket->SendBuffer();

   theMessage = ip_Socket->Receive(10);

   if (!theMessage)
      return false;

   ip_Log->LogMessage("%s: %s", is_WorkSuffix.c_str(), theMessage);

   accepted = false;

   if (!memcmp(theMessage, "INFO", 4) && strstr(theMessage, wu->s_Name))
      accepted = true;

   return accepted;
}

// Save the current status
void  DMDivisorWorker::Save(FILE* fPtr)
{
   workunit_t* wu, * wuNext;
   WorkUnitTest* wuTest, * wuTestNext;

   fprintf(fPtr, "ServerVersion=%s\n", is_ServerVersion.c_str());
   fprintf(fPtr, "ServerType=%d\n", ii_ServerType);

   ii_CurrentWorkUnits = ii_CompletedWorkUnits = 0;
   wu = ip_FirstWorkUnit;
   ip_FirstWorkUnit = 0;
   while (wu)
   {
      fprintf(fPtr, "Start WorkUnit %u %" PRIu64" %" PRIu64" %" PRIu64"\n",
         wu->i_n, wu->l_minK, wu->l_maxK, wu->l_TestID);

      wuNext = (workunit_t*)wu->m_NextWorkUnit;
      wu->m_NextWorkUnit = 0;

      wuTest = (WorkUnitTest*)wu->m_FirstWorkUnitTest;
      while (wuTest)
      {
         wuTestNext = wuTest->GetNextWorkUnitTest();
         wuTest->Save(fPtr);
         wuTest = wuTestNext;
      }

      fprintf(fPtr, "End WorkUnit %" PRIu64" %s\n", wu->l_TestID, wu->s_Name);

      AddWorkUnitToList(wu);

      wu = wuNext;
   }
}

// Load the previous status
void  DMDivisorWorker::Load(string saveFileName)
{
   FILE* fPtr;
   char           line[2000];
   int32_t        countScanned;
   workunit_t* wu = 0;

   if ((fPtr = fopen(saveFileName.c_str(), "r")) == NULL)
   {
      printf("Could not open file [%s] for reading\n", saveFileName.c_str());
      exit(-1);
   }

   while (fgets(line, BUFFER_SIZE, fPtr) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "ServerType=", 11))
         ii_ServerType = atol(line + 11);
      else if (!memcmp(line, "ServerVersion=", 14))
         is_ServerVersion = line + 14;
      else if (!memcmp(line, "Start WorkUnit", 14))
      {
         wu = new workunit_t;
         wu->m_FirstWorkUnitTest = 0;
         countScanned = sscanf(line, "Start WorkUnit %u %" PRIu64" %" PRIu64" %" PRIu64"",
            &wu->i_n, &wu->l_minK, &wu->l_maxK, &wu->l_TestID);

         if (countScanned != 4)
         {
            printf("'Start WorkUnit' was not in the correct format.  Exiting\n");
            exit(-1);
         }

         snprintf(wu->s_Name, sizeof(wu->s_Name), "%u_%" PRIu64"_%" PRIu64"", wu->i_n, wu->l_minK, wu->l_maxK);
         ip_WorkUnitTestFactory->LoadWorkUnitTest(fPtr, ii_ServerType, wu);
         AddWorkUnitToList(wu);
      }
   }

   fclose(fPtr);
}

bool  DMDivisorWorker::IsWorkUnitCompleted(workunit_t* wu)
{
   WorkUnitTest* wuTest;

   wuTest = (WorkUnitTest*)wu->m_FirstWorkUnitTest;

   if (wuTest->GetWorkUnitTestState() != WUT_COMPLETED)
   {
      ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitCompleted for %s.  Returning false (not completed, main)",
         is_WorkSuffix.c_str(), wu->s_Name);
      return false;
   }

   // Subsequent tests associated with this workunit only need to be checked out
   // if the master workunit is PRP or prime.
   if (PRP_OR_PRIME(wuTest->GetWorkUnitTestResult()))
   {
      wuTest = wuTest->GetNextWorkUnitTest();
      while (wuTest)
      {
         if (wuTest->GetWorkUnitTestState() != WUT_COMPLETED)
         {
            ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitCompleted for %s.  Returning false (not completed, subtest)",
               is_WorkSuffix.c_str(), wu->s_Name);
            return false;
         }

         wuTest = wuTest->GetNextWorkUnitTest();
      }
   }

   ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitCompleted for %s.  Returning true (completed)",
      is_WorkSuffix.c_str(), wu->s_Name);
   return true;
}

bool  DMDivisorWorker::IsWorkUnitInProgress(workunit_t* wu)
{
   WorkUnitTest* wuTest;

   wuTest = (WorkUnitTest*)wu->m_FirstWorkUnitTest;

   if (wuTest->GetWorkUnitTestState() == WUT_INPROGRESS)
   {
      ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitInProgress for %s.  Returning true (in progress, main)",
         is_WorkSuffix.c_str(), wu->s_Name);
      return true;
   }

   // Subsequent tests associated with this workunit only need to be checked out
   // if the master workunit is PRP or prime.
   if (PRP_OR_PRIME(wuTest->GetWorkUnitTestResult()))
   {
      wuTest = wuTest->GetNextWorkUnitTest();
      while (wuTest)
      {
         if (wuTest->GetWorkUnitTestState() == WUT_INPROGRESS)
         {
            ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitInProgress for %s.  Returning true (in progress, subtest)",
               is_WorkSuffix.c_str(), wu->s_Name);
            return true;
         }

         wuTest = wuTest->GetNextWorkUnitTest();
      }
   }

   ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitInProgress.  Returning false (all done)", is_WorkSuffix.c_str());
   return false;
}
