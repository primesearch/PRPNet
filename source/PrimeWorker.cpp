#include <signal.h>
#include <ctype.h>

#include "PrimeWorker.h"
#include "PrimeWorkUnitTest.h"

PrimeWorker::PrimeWorker(Log *theLog, TestingProgramFactory *testingProgramFactory,
               ClientSocket *theSocket, int32_t maxWorkUnits, string workSuffix)
      : Worker(theLog, testingProgramFactory, theSocket, maxWorkUnits, workSuffix)
{
   ii_ServerType = 0;
   is_ServerVersion = "";
}

bool  PrimeWorker::ProcessWorkUnit(int32_t &specialsFound, bool inProgressOnly, double &seconds)
{
   bool          wasCompleted;
   workunit_t   *wu, *srWU;
   WorkUnitTest *wuTest, *wuTestNext;
   result_t      prpTestResult;

   seconds = 0.0;

   specialsFound = 0;

   wu = GetNextIncompleteWorkUnit(inProgressOnly);
   if (!wu) return false;

   wuTest = (WorkUnitTest *) wu->m_FirstWorkUnitTest;
   while (wuTest)
   {
      wuTestNext = wuTest->GetNextWorkUnitTest();

      wasCompleted = wuTest->TestWorkUnit(wu->m_FirstWorkUnitTest);

      if (!wasCompleted)
      {
         ip_Log->Debug(DEBUG_WORK, "%s: ProcessWorkUnit did not complete %s", is_WorkSuffix.c_str(), wu->s_Name);
         return false;
      }

      wuTest = wuTestNext;
   }

   ii_CompletedWorkUnits++;

   wuTest = (WorkUnitTest *) wu->m_FirstWorkUnitTest;

   prpTestResult = wuTest->GetWorkUnitTestResult();

   wu->i_DecimalLength = wuTest->GetDecimalLength();

   seconds = wuTest->GetSeconds();

   if (PRP_OR_PRIME(prpTestResult))
   {
      specialsFound = 1;

      // If this workunit is PRP or prime and the server is Sierpinski/Riesel,
      // then skip any other workunits for the same k/b/c that have a higher n
      // as the client does not need to test them.
      if (ii_ServerType == ST_SIERPINSKIRIESEL)
      {
         srWU = ip_FirstWorkUnit;
         while (srWU)
         {
            if (!IsWorkUnitCompleted(srWU))
            {
               if (srWU->l_k == wu->l_k && srWU->i_b == wu->i_b &&
                   srWU->i_c == wu->i_c && srWU->i_n > wu->i_n)
               {
                  srWU->b_SRSkipped = true;
                  ip_Log->LogMessage("%s: %s will not be tested because %s is %s", is_WorkSuffix.c_str(),
                                     srWU->s_Name, wu->s_Name, (prpTestResult == R_PRP ? "PRP" : "prime"));
               }
            }

            srWU = (workunit_t *) srWU->m_NextWorkUnit;
         };
      }
   }

   ip_Log->Debug(DEBUG_WORK, "%s: ProcessWorkUnit completed testing %s", is_WorkSuffix.c_str(), wu->s_Name);
   return true;
}

bool     PrimeWorker::GetWork(void)
{
   char       *readBuf;
   int32_t     endLoop;
   int32_t     toScan, wasScanned;
   workunit_t *wu;

   if (ii_CurrentWorkUnits)
      return false;

   is_ServerVersion = ip_Socket->GetServerVersion();
   ii_ServerType = ip_Socket->GetServerType();

   ip_Log->LogMessage("%s: Getting work from server %s at port %d",
                      is_WorkSuffix.c_str(), ip_Socket->GetServerName().c_str(), ip_Socket->GetPortID());

   ip_Socket->Send("GETWORK %s %d", PRPNET_VERSION, ii_MaxWorkUnits);
   ip_TestingProgramFactory->SendPrograms(ip_Socket);
   ip_Socket->Send("End of Message");

   ii_CompletedWorkUnits = ii_CurrentWorkUnits = 0;
   endLoop = false;

   readBuf = ip_Socket->Receive(60);
   while (readBuf && !endLoop)
   {
      // Keepalive tells the client that the server is still searching for
      // workunits to send.
      if (!memcmp(readBuf, "keepalive", 9))
         ;
      else if (!memcmp(readBuf, "ServerConfig: ", 13))
      {
         // This message is not used from the client as of 5.4, but we'll 
         // retain support for it as it might be needed in the future.
      }
      else if (!memcmp(readBuf, "WorkUnit: ", 10))
      {
         wu = new workunit_t;
         wu->m_FirstWorkUnitTest = 0;
         wu->l_k = 0;
         wu->i_b = 0;
         wu->i_c = 0;
         wu->i_n = 0;
         wu->b_SRSkipped = false;

         if (ii_ServerType == ST_PRIMORIAL || ii_ServerType == ST_FACTORIAL)
         {
            toScan = 4;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64" %u %d",
                                 wu->s_Name,
                                &wu->l_TestID,
                                &wu->i_b,
                                &wu->i_c);
         }
         if (ii_ServerType == ST_MULTIFACTORIAL)
         {
            toScan = 5;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64" %u %u %d",
                                 wu->s_Name,
                                &wu->l_TestID,
                                &wu->i_b,
                                &wu->i_n,
                                &wu->i_c);
         }
         else if (ii_ServerType == ST_GFN)
         {
            toScan = 4;
            wu->l_k = wu->i_c = 1;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64" %u %u",
                                 wu->s_Name,
                                &wu->l_TestID,
                                &wu->i_b,
                                &wu->i_n);
         }
         else if (ii_ServerType == ST_CYCLOTOMIC)
         {
            toScan = 5;
            wu->i_c = 1;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64" %" PRIu64" %d %u",
                                 wu->s_Name,
                                &wu->l_TestID,
                                &wu->l_k,
                                &wu->i_b,
                                &wu->i_n);
         }
         else if (ii_ServerType == ST_XYYX)
         {
            toScan = 5;
            wu->l_k = 1;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64" %u %u %u",
                                 wu->s_Name,
                                &wu->l_TestID,
                                &wu->i_b,
                                &wu->i_n,
                                &wu->i_c);
         }
         else if (ii_ServerType == ST_WAGSTAFF)
         {
            toScan = 3;
            wu->l_k = 1;
            wu->i_b = 1;
            wu->i_c = 1;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64" %u",
                                 wu->s_Name,
                                &wu->l_TestID,
                                &wu->i_n);
         }
         else if (ii_ServerType == ST_GENERIC)
         {
            toScan = 2;
            wu->l_k = 1;
            wu->i_b = 1;
            wu->i_n = 1;
            wu->i_c = 1;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64"",
                                 wu->s_Name,
                                &wu->l_TestID);
         }
         else
         {
            toScan = 6;
            wasScanned = sscanf(readBuf, "WorkUnit: %s %" PRIu64" %" PRIu64" %u %u %d",
                                 wu->s_Name,
                                &wu->l_TestID,
                                &wu->l_k,
                                &wu->i_b,
                                &wu->i_n,
                                &wu->i_c);
         }
         if (toScan == wasScanned)
         {
            wu->m_FirstWorkUnitTest = ip_WorkUnitTestFactory->BuildWorkUnitTestList(ii_ServerType, wu);
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

   ip_Log->LogMessage("%s: PRPNet server is version %s", is_WorkSuffix.c_str(), is_ServerVersion.c_str());

   return true;
}

void  PrimeWorker::ReturnWork(uint32_t returnOption)
{
   char       *readBuf;
   bool        completed, accepted;
   workunit_t *wu, *wuNext;

   ip_Log->LogMessage("%s: Returning work to server %s at port %d",
                     is_WorkSuffix.c_str(), ip_Socket->GetServerName().c_str(), ip_Socket->GetPortID());

   wu = ip_FirstWorkUnit;

   ip_FirstWorkUnit = 0;
   ii_CompletedWorkUnits = ii_CurrentWorkUnits = 0;

   ip_Socket->Send("RETURNWORK %s", PRPNET_VERSION);

   accepted = true;
   while (wu)
   {
      wuNext = wu->m_NextWorkUnit;
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
bool  PrimeWorker::ReturnWorkUnit(workunit_t *wu, bool completed)
{
   char       *theMessage;
   bool        accepted;
   PrimeWorkUnitTest *wuTest, *wuNext;

   wuTest = (PrimeWorkUnitTest *) wu->m_FirstWorkUnitTest;
   ip_Socket->StartBuffering();
   
   if (ii_ServerType == ST_GENERIC)
      ip_Socket->Send("WorkUnit: %s %" PRIu64" %d", wu->s_Name, wu->l_TestID, wu->i_DecimalLength);
   else
      ip_Socket->Send("WorkUnit: %s %" PRIu64"", wu->s_Name, wu->l_TestID);

   if (!completed || wu->b_SRSkipped)
      ip_Socket->Send("Test Abandoned");
   else
   {
      while (wuTest)
      {
         wuNext = (PrimeWorkUnitTest *) wuTest->GetNextWorkUnitTest();
         wuTest->SendResults(ip_Socket);
         wuTest = wuNext;
      }
   }

   ip_Socket->Send("End of WorkUnit: %s %" PRIu64"", wu->s_Name, wu->l_TestID);
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
void  PrimeWorker::Save(FILE *fPtr)
{
   workunit_t *wu, *wuNext;
   WorkUnitTest *wuTest, *wuTestNext;

   fprintf(fPtr, "ServerVersion=%s\n", is_ServerVersion.c_str());
   fprintf(fPtr, "ServerType=%d\n", ii_ServerType);

   ii_CurrentWorkUnits = ii_CompletedWorkUnits = 0;
   wu = ip_FirstWorkUnit;
   ip_FirstWorkUnit = 0;
   while (wu)
   {
      fprintf(fPtr, "Start WorkUnit %" PRIu64" %s %" PRId64" %d %d %d %d %d\n",
         wu->l_TestID, wu->s_Name, wu->l_k, wu->i_b, wu->i_n, wu->i_c, wu->b_SRSkipped, wu->i_DecimalLength);

      wuNext = (workunit_t *) wu->m_NextWorkUnit;
      wu->m_NextWorkUnit = 0;

      wuTest = (WorkUnitTest *) wu->m_FirstWorkUnitTest;
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
void  PrimeWorker::Load(string saveFileName)
{
   FILE          *fPtr;
   char           line[2000];
   int32_t        countScanned;
   workunit_t    *wu = 0;

   if ((fPtr = fopen(saveFileName.c_str(), "r")) == NULL)
   {
      printf("Unable to open save file %s\n", saveFileName.c_str());
      exit(-1);
   }

   ii_ServerType = 0;

   while (fgets(line, BUFFER_SIZE, fPtr) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "Start WorkUnit", 14))
      {
         if (!ii_ServerType)
         {
            printf("'Start WorkUnit' found before 'ServerType' in save file.  Cannot load save file.  Exiting\n");
            exit(-1);
         }

         wu = new workunit_t;
         wu->m_FirstWorkUnitTest = 0;
         countScanned = sscanf(line, "Start WorkUnit %" PRIu64" %s %" PRId64" %d %d %d %d %d",
            &wu->l_TestID, wu->s_Name, &wu->l_k, &wu->i_b, &wu->i_n, &wu->i_c, (int *) &wu->b_SRSkipped, &wu->i_DecimalLength);

         if (countScanned == 7)
         {
            countScanned = 8;
            if (ii_ServerType == ST_GENERIC)
               wu->i_DecimalLength = (int32_t) strlen(wu->s_Name);
            else
               wu->i_DecimalLength = 0;
         }

         if (countScanned != 8)
         {
            printf("'Start WorkUnit' was not in the correct format.  Exiting\n");
            exit(-1);
         }

         ip_WorkUnitTestFactory->LoadWorkUnitTest(fPtr, ii_ServerType, wu);
         AddWorkUnitToList(wu);
      }
      else if (!memcmp(line, "ServerType=", 11))
         ii_ServerType = atol(line+11);
      else if (!memcmp(line, "ServerVersion=", 14))
         is_ServerVersion = line+14;
   }

   fclose(fPtr);
}

bool  PrimeWorker::IsWorkUnitCompleted(workunit_t *wu)
{
   WorkUnitTest *wuTest;

   if (wu->b_SRSkipped)
   {
      ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitCompleted for %s.  Returning true (skipped)",
                    is_WorkSuffix.c_str(), wu->s_Name);
      return true;
   }

   wuTest = (WorkUnitTest *) wu->m_FirstWorkUnitTest;

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

bool  PrimeWorker::IsWorkUnitInProgress(workunit_t *wu)
{
   WorkUnitTest *wuTest;

   if (wu->b_SRSkipped)
   {
      ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitInProgress for %s.  Returning false (skipped)",
                    is_WorkSuffix.c_str(), wu->s_Name);
      return false;
   }

   wuTest = (WorkUnitTest *) wu->m_FirstWorkUnitTest;

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

   ip_Log->Debug(DEBUG_WORK, "%s: IsWorkUnitInProgress for %s.  Returning false (all done)",
                 is_WorkSuffix.c_str(), wu->s_Name);
   return false;
}

