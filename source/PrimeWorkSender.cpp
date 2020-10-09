#include "PrimeWorkSender.h"
#include "SQLStatement.h"
#include "server.h"
#include "KeepAliveThread.h"

PrimeWorkSender::PrimeWorkSender(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                                 string userID, string emailID, 
                                 string machineID, string instanceID, string teamID)
                                 : WorkSender(dbInterface, theSocket, globals,
                                              userID, emailID, machineID, instanceID, teamID)
{
   char  *readBuf;

   ib_UseLLROverPFGW = globals->b_UseLLROverPFGW;
   ib_OneKPerInstance = globals->b_OneKPerInstance;
   is_OrderBy = globals->s_SortSequence;
   ii_DelayCount = globals->i_DelayCount;
   ip_Delay = globals->p_Delay;

   ib_HasLLR = ib_HasPhrot = ib_HasPFGW = ib_HasCyclo = false;
   ib_HasAnyGenefer = ib_HasGeneferGPU = false; ib_HasGenefx64 = ib_HasGenefer = ib_HasGenefer80bit = false;
   while (true)
   {
      readBuf = ip_Socket->Receive();

      if (!readBuf)
         break;

      if (!memcmp(readBuf, "llr", 3))
         ib_HasLLR = true;
      if (!memcmp(readBuf, "phrot", 5))
         ib_HasPhrot = true;
      if (!memcmp(readBuf, "pfgw", 4))
         ib_HasPFGW = true;
      if (!memcmp(readBuf, "cyclo", 5))
         ib_HasCyclo = true;
      if (!strcmp(readBuf, GENEFER_cuda) || !strcmp(readBuf, GENEFER_OpenCL))
         ib_HasAnyGenefer = ib_HasGeneferGPU = true;
      if (!strcmp(readBuf, GENEFER_x64))
         ib_HasAnyGenefer = ib_HasGenefx64 = true;
      if (!strcmp(readBuf, GENEFER))
         ib_HasAnyGenefer = ib_HasGenefer = true;
      if (!strcmp(readBuf, GENEFER_80bit))
         ib_HasAnyGenefer = ib_HasGenefer80bit = true;

      if (!memcmp(readBuf, "End of Message", 14))
         break;
   }
}

PrimeWorkSender::~PrimeWorkSender()
{
}

void  PrimeWorkSender::ProcessMessage(string theMessage)
{
   int32_t      sendWorkUnits, sentWorkUnits = 0;
   char         clientVersion[20];
   char         tempMessage[200];

   strcpy(tempMessage, theMessage.c_str());

   clientVersion[0] = 0;
   if (sscanf(tempMessage, "GETWORK %s %u", clientVersion, &sendWorkUnits) != 2)
      sendWorkUnits = 1;

   is_ClientVersion = clientVersion;

   if ((ii_ServerType == ST_PRIMORIAL || ii_ServerType == ST_FACTORIAL || ii_ServerType == ST_MULTIFACTORIAL) && !ib_HasPFGW)
   {
      ip_Socket->Send("ERROR:  The client must run PFGW to use this server.");
      ip_Socket->Send("End of Message");
      return;
   }

   // Require a client of at least 5.3.0 so the genefer version is reported correctly
   if (ii_ServerType == ST_GFN && memcmp(clientVersion, "5.3", 3) < 0)
   {
      ip_Socket->Send("ERROR:  The client must be version 5.3.0 or greater to use this server.");
      ip_Socket->Send("End of Message");
      return;
   }

   if (ii_ServerType == ST_CYCLOTOMIC)
   {
      if (!ib_HasCyclo && !ib_HasPFGW)
      {
         ip_Socket->Send("ERROR:  The client must run cyclo or pfgw to use this server.");
         ip_Socket->Send("End of Message");
         return;
      }
   }
   else if (ii_ServerType == ST_GFN)
   {
      if (!ib_HasLLR && !ib_HasPhrot && !ib_HasPFGW && !ib_HasAnyGenefer)
      {
         ip_Socket->Send("ERROR:  The client must run Genefer, LLR, PFGW, or Phrot to use this server.");
         ip_Socket->Send("End of Message");
         return;
      }
   }
   else
   {
      if (!ib_HasLLR && !ib_HasPhrot && !ib_HasPFGW)
      {
			ip_Socket->Send("ERROR:  The client must run LLR, PFGW, or Phrot to use this server.");
			ip_Socket->Send("End of Message");
			return;
	   }
   }

   if (sendWorkUnits > ii_MaxWorkUnits)
   {
      sendWorkUnits = ii_MaxWorkUnits;
      ip_Socket->Send("INFO: Server has a limit of %u work units.", ii_MaxWorkUnits);
   }
   
   ip_Socket->Send("ServerConfig: %d", (ib_UseLLROverPFGW ? 1 : 0));
   
   if (ib_NoNewWork)
      ip_Socket->Send("INACTIVE: New work generation is disabled on this server.");
   else 
   {
      // First look for double-check work.
      if (ib_NeedsDoubleCheck)
         sentWorkUnits = SendWorkToClient(sendWorkUnits, true, false);

      // If we don't find that, look for first check work
      if (!sentWorkUnits)
         sentWorkUnits = SendWorkToClient(sendWorkUnits, false, ib_OneKPerInstance);
     
      if (!sentWorkUnits)
         ip_Socket->Send("INACTIVE: No available candidates are left on this server.");
   }

   ip_Socket->Send("End of Message");

   if (sentWorkUnits > 0)
      SendGreeting("Greeting.txt");
}

int32_t  PrimeWorkSender::SendWorkToClient(int32_t sendWorkUnits, bool doubleCheckOnly, bool oneKPerInstance)
{
   KeepAliveThread  *keepAliveThread;
   SharedMemoryItem *threadWaiter;
   bool              endLoop;
   int32_t           idx, sentWorkUnits = 0;
   int64_t           currentTime;

   threadWaiter = ip_Socket->GetThreadWaiter();
   threadWaiter->SetValueNoLock(KAT_WAITING);
   keepAliveThread = new KeepAliveThread();
   keepAliveThread->StartThread(ip_Socket);
   
   if (doubleCheckOnly)
   {
      ip_Log->Debug(DEBUG_WORK, "%d: Looking for double-check work", ip_Socket->GetSocketID());
      currentTime = (int64_t) time(NULL);

      for (idx=0; idx<ii_DelayCount-1; idx++)
      {
         sentWorkUnits = SelectDoubleCheckCandidates(sendWorkUnits, ip_Delay[idx].minLength,
            ip_Delay[idx].maxLength, currentTime - ip_Delay[idx].doubleCheckDelay);

         if (sentWorkUnits >= sendWorkUnits)
            break;
      }
   }
   else if (oneKPerInstance)
   {
      sentWorkUnits = SelectOneKPerClientCandidates(sendWorkUnits, true);

      sentWorkUnits += SelectOneKPerClientCandidates(sendWorkUnits - sentWorkUnits, false);
   }
   else if (ii_ServerType == ST_GFN)
      sentWorkUnits = SelectGFNCandidates(sendWorkUnits);
   else
      sentWorkUnits = SelectCandidates(sendWorkUnits);

   // When we know that the keepalive thread has terminated, we can then exit
   // this loop and delete the memory associated with that thread.
   endLoop = false;
   while (!endLoop)
   {
      threadWaiter->Lock();

      if (threadWaiter->GetValueHaveLock() == KAT_TERMINATED)
      {
         endLoop = true;
         threadWaiter->Release();
      }
      else
      {
         threadWaiter->SetValueHaveLock(KAT_TERMINATE);
         threadWaiter->Release();
         Sleep(1000);
      }
   }

   delete keepAliveThread;
   return sentWorkUnits;
}

int32_t  PrimeWorkSender::SelectDoubleCheckCandidates(int32_t sendWorkUnits, double minLength, double maxLength, int64_t olderThanTime)
{
   SQLStatement *selectStatement;
   char     candidateName[NAME_LENGTH+1];
   int64_t  theK, lastUpdateTime = 0;
   int32_t  theB, theC, theN;
   double   decimalLength = 0.0;
   bool     encounteredError;
   int32_t  sentWorkUnits;

   // This will not double-check primes.  That will be changed in the future.
   const char *selectSQL = "select CandidateName, DecimalLength, LastUpdateTime, " \
                           "       k, b, c, n " \
                           "  from Candidate " \
                           " where HasPendingTest = 0 " \
                           "   and $null_func$(MainTestResult, 0) = 0 " \
                           "   and DoubleChecked = 0 " \
                           "   and CompletedTests > 0 " \
                           "   and DecimalLength >= %lf " \
                           "   and DecimalLength < %lf " \
                           "   and LastUpdateTime < %" PRId64" " \
                           "   and ((DecimalLength >= ? and LastUpdateTime > ?) or (DecimalLength > ?)) " \
                           "order by DecimalLength, LastUpdateTime limit 100";

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, minLength, maxLength, olderThanTime);

   selectStatement->BindInputParameter(decimalLength);
   selectStatement->BindInputParameter(lastUpdateTime);
   selectStatement->BindInputParameter(decimalLength);

   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   selectStatement->BindSelectedColumn(&decimalLength);
   selectStatement->BindSelectedColumn(&lastUpdateTime);
   selectStatement->BindSelectedColumn(&theK);
   selectStatement->BindSelectedColumn(&theB);
   selectStatement->BindSelectedColumn(&theC);
   selectStatement->BindSelectedColumn(&theN);

   sentWorkUnits = 0;
   encounteredError = false;
   
   // On the first open of the cursor, we start from the beginning, but if we can't find
   // enough double-check candidates, we'll start where we left off.
   lastUpdateTime = 0;
   decimalLength = 0.0;

   while (sentWorkUnits < sendWorkUnits && !encounteredError)
   {
      // If we haven't sent enough, start over again at the shortest candidate that we haven't checked
      selectStatement->SetInputParameterValue(decimalLength, true);
      selectStatement->SetInputParameterValue(lastUpdateTime);
      selectStatement->SetInputParameterValue(decimalLength);

      while (sentWorkUnits < sendWorkUnits && !encounteredError)
      {
         // Exit both loops if no rows are selected
         if (!selectStatement->FetchRow(false)) {
            encounteredError = true;
            break;
         }

         if (!CheckDoubleCheck(candidateName, decimalLength, lastUpdateTime))
         {
            ip_Log->Debug(DEBUG_WORK, "%d: Cannot double-check %s", ip_Socket->GetSocketID(), candidateName);
            continue;
         }

         // Allow only one thread through here at a time
         ip_Locker->Lock();
         
         // Reserve the candidate and send to the client
         if (ReserveCandidate(candidateName))
         {
            ip_Log->Debug(DEBUG_WORK, "Double check for candidate %s", candidateName);

            if (SendWork(candidateName, theK, theB, theN, theC))
            {
               sentWorkUnits++;
               ip_DBInterface->Commit();
            }
            else
            {
               ip_DBInterface->Rollback();
               encounteredError = true;
            }
         }

         ip_Locker->Release();
      }

      selectStatement->CloseCursor();
   }

   // Just in case a commit or rollback was not done
   ip_DBInterface->Rollback();

   delete selectStatement;

   return sentWorkUnits;
}

int32_t  PrimeWorkSender::SelectOneKPerClientCandidates(int32_t sendWorkUnits, bool firstPass)
{
   SQLStatement *selectKBCStatement;
   SQLStatement *selectStatement;
   char     candidateName[NAME_LENGTH+1];
   int64_t  theK;
   int32_t  theB, theC, theN = 0;
   int32_t  sierpinskiRieselPrimeN;
   int32_t  countInProgress;
   bool     encounteredError;
   int32_t  sentWorkUnits = 0;
   
   if (sendWorkUnits == 0)
      return 0;
    
   // This will allow a second client to grab the same k, b, c that another client has
   // but only if no clients have that k, b, c assigned to them.
   const char* selectKBCSQL1 = "select distinct k, b, c, CountInProgress, SierpinskiRieselPrimeN " \
	                           "  from CandidateGroupStats cgs " \
	                           " where SierpinskiRieselPrimeN = 0 " \
	                           "   and CountUntested > CountInProgress " \
	                           "order by CountInProgress, k, b, c ";

   // In the second pass, look for n < SierpinskiRieselPrimeN without a test
   const char* selectKBCSQL2 = "select distinct k, b, c, CountInProgress, SierpinskiRieselPrimeN " \
	                           "  from CandidateGroupStats cgs " \
	                           " where SierpinskiRieselPrimeN > (select min(n) " \
                                                              "   from Candidate " \
                                                              "  where b = cgs.b " \
                                                              "    and k = cgs.k " \
                                                              "    and c = cgs.c " \
                                                              "    and CompletedTests = 0 " \
                                                              "    and HasPendingTest = 0) " \
	                           "   and CountUntested > CountInProgress " \
	                           "order by CountInProgress, k, b, c ";

   const char *selectSQL = "select CandidateName, n " \
                           "  from Candidate " \
                           " where CompletedTests = 0 " \
                           "   and HasPendingTest = 0 " \
                           "   and DecimalLength > 0 " \
                           "   and k = %" PRId64" " \
                           "   and b = %d " \
                           "   and c = %d " \
                           "   and n > ? " \
                           "   and (n < %d  or %d = 0)" \
                           "order by %s limit 100";
   
   // Allow only one thread through this method at a time since each thread
   // is locking all Candidates for a single k/b/c.
   ip_Locker->Lock();

   if (firstPass)
      selectKBCStatement = new SQLStatement(ip_Log, ip_DBInterface, selectKBCSQL1);
   else
      selectKBCStatement = new SQLStatement(ip_Log, ip_DBInterface, selectKBCSQL2);

   selectKBCStatement->BindSelectedColumn(&theK);
   selectKBCStatement->BindSelectedColumn(&theB);
   selectKBCStatement->BindSelectedColumn(&theC);
   selectKBCStatement->BindSelectedColumn(&countInProgress);
   selectKBCStatement->BindSelectedColumn(&sierpinskiRieselPrimeN);
   
   if (selectKBCStatement->FetchRow(false))
   {
	   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, theK, theB, theC, sierpinskiRieselPrimeN, sierpinskiRieselPrimeN, is_OrderBy.c_str());

	   selectStatement->BindInputParameter(theN);

	   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
	   selectStatement->BindSelectedColumn(&theN);

	   sentWorkUnits = 0;
	   theN = 0;
	   encounteredError = false;

	   while (sentWorkUnits < sendWorkUnits && !encounteredError)
	   {
		   // If we haven't sent enough, start over again at the last n for this k/b/c
		   selectStatement->SetInputParameterValue(theN, true);

		   while (sentWorkUnits < sendWorkUnits && !encounteredError)
		   {
			   // Exit both loops if no rows are selected
			   if (!selectStatement->FetchRow(false)) {
				   encounteredError = true;
				   break;
			   }

			   // Reserve the candidate and send to the client
			   if (ReserveCandidate(candidateName))
			   {
				   ip_Log->Debug(DEBUG_WORK, "First check for candidate %s", candidateName);

				   if (SendWork(candidateName, theK, theB, theN, theC))
				   {
					   sentWorkUnits++;
					   ip_DBInterface->Commit();
				   }
				   else
				   {
					   ip_DBInterface->Rollback();
					   encounteredError = true;
				   }
			   }
		   }

		   selectStatement->CloseCursor();
	   }
       
      delete selectStatement;
   }

   selectKBCStatement->CloseCursor();

   // Just in case a commit or rollback was not done
   ip_DBInterface->Rollback();

   delete selectKBCStatement;

   ip_Locker->Release();

   return sentWorkUnits;
}

int32_t  PrimeWorkSender::SelectGFNCandidates(int32_t sendWorkUnits)
{
   SQLStatement *selectStatement;
   char     candidateName[NAME_LENGTH+1];
   int32_t  theB = 0, theN = 0;
   bool     encounteredError;
   int32_t  sentWorkUnits;

   const char *selectSQL = "select CandidateName, b, n " \
                           "  from Candidate " \
                           " where HasPendingTest = 0 " \
                           "   and CompletedTests = 0 " \
                           "   and DecimalLength > 0 " \
                           "   and ((b = ? and n > ?) or (b > ?)) " \
                           "order by %s limit 100";

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, is_OrderBy.c_str());

   selectStatement->BindInputParameter(theB);
   selectStatement->BindInputParameter(theN);
   selectStatement->BindInputParameter(theB);

   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   selectStatement->BindSelectedColumn(&theB);
   selectStatement->BindSelectedColumn(&theN);

   sentWorkUnits = 0;
   encounteredError = false;
   theB = theN = 0;

   while (sentWorkUnits < sendWorkUnits && !encounteredError)
   {
      selectStatement->SetInputParameterValue(theB, true);
      selectStatement->SetInputParameterValue(theN);
      selectStatement->SetInputParameterValue(theB);

      while (sentWorkUnits < sendWorkUnits && !encounteredError)
      {
         // Exit both loops if no rows are selected
         if (!selectStatement->FetchRow(false)) {
            encounteredError = true;
            break;
         }

         if (!CheckGenefer(candidateName))
         {
            ip_Log->Debug(DEBUG_WORK, "%d: Client does not support version of genefer needed for %s", ip_Socket->GetSocketID(), candidateName);
            continue;
         }

         // Allow only one thread through here at a time
         ip_Locker->Lock();
         
         // Reserve the candidate and send to the client
         if (ReserveCandidate(candidateName))
         {
            ip_Log->Debug(DEBUG_WORK, "First check for candidate %s", candidateName);

            if (SendWork(candidateName, 0, theB, theN, 0))
            {
               sentWorkUnits++;
               ip_DBInterface->Commit();
            }
            else
            {
               ip_DBInterface->Rollback();
               encounteredError = true;
            }
         }

         ip_Locker->Release();
      }

      selectStatement->CloseCursor();
   }

   // Just in case a commit or rollback was not done
   ip_DBInterface->Rollback();

   delete selectStatement;

   return sentWorkUnits;
}

int32_t  PrimeWorkSender::SelectCandidates(int32_t sendWorkUnits)
{
   SQLStatement *selectStatement;
   char     candidateName[NAME_LENGTH+1];
   int64_t  theK;
   int32_t  theB, theC, theN;
   bool     encounteredError;
   int32_t  sentWorkUnits;

   const char *selectSQL1 = "select CandidateName, k, b, c, n " \
                            "  from Candidate " \
                            " where HasPendingTest = 0 " \
                            "   and CompletedTests = 0 " \
                            "   and DecimalLength > 0 " \
                            "order by %s limit 500";

   const char *selectSQL2 = "select c.CandidateName, c.k, c.b, c.c, c.n " \
                            "  from Candidate c, CandidateGroupStats cgs" \
                            " where c.HasPendingTest = 0 " \
                            "   and c.CompletedTests = 0 " \
                            "   and c.DecimalLength > 0 " \
                            "   and c.k = cgs.k " \
                            "   and c.b = cgs.b " \
                            "   and c.c = cgs.c " \
                            "   and (c.n < cgs.SierpinskiRieselPrimeN or cgs.SierpinskiRieselPrimeN = 0) " \
                            "order by %s limit 500";

   if (ii_ServerType == ST_SIERPINSKIRIESEL)
      selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL2, is_OrderBy.c_str());
   else
      selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL1, is_OrderBy.c_str());

   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   selectStatement->BindSelectedColumn(&theK);
   selectStatement->BindSelectedColumn(&theB);
   selectStatement->BindSelectedColumn(&theC);
   selectStatement->BindSelectedColumn(&theN);

   sentWorkUnits = 0;
   encounteredError = false;

   // We only do a single pass because we can't control the ORDER BY.  Technically we could,
   // but it would be a lot of work for little gain.  The only downside is that we might not
   // be able to send as many as the client is requesting.
   while (sentWorkUnits < sendWorkUnits && !encounteredError)
   {
      // Exit if no rows are selected
      if (!selectStatement->FetchRow(false)) {
         encounteredError = true;
         break;
      }

      // Allow only one thread through here at a time
      ip_Locker->Lock();
      
      // Reserve the candidate and send to the client
      if (ReserveCandidate(candidateName))
      {
         ip_Log->Debug(DEBUG_WORK, "First check for candidate %s", candidateName);

         if (SendWork(candidateName, theK, theB, theN, theC))
         {
            sentWorkUnits++;
            ip_DBInterface->Commit();
         }
         else
         {
            ip_DBInterface->Rollback();
            encounteredError = true;
         }
      }

      ip_Locker->Release();
   }

   selectStatement->CloseCursor();

   // Just in case a commit or rollback was not done
   ip_DBInterface->Rollback();

   delete selectStatement;

   return sentWorkUnits;
}

bool     PrimeWorkSender::CheckGenefer(string candidateName)
{
   SQLStatement *sqlStatement;
   int32_t  count;
   char     programName[NAME_LENGTH+1];
   const char *selectSQL = "select count(*) " \
                           "  from GeneferROE " \
                           " where CandidateName = ? " \
                           "   and GeneferVersion = ?";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH, true);
   sqlStatement->BindInputParameter(programName, NAME_LENGTH, false);
   sqlStatement->BindSelectedColumn(&count);

   sqlStatement->SetInputParameterValue(candidateName, true);
   sqlStatement->SetInputParameterValue((char *) GENEFER_80bit);

   if (sqlStatement->FetchRow(true) && count > 0)
   {
      delete sqlStatement;

      if (!ib_HasLLR && !ib_HasPhrot && !ib_HasPFGW)
         return false;
      else
         return true;
   }

   sqlStatement->SetInputParameterValue(candidateName, true);
   sqlStatement->SetInputParameterValue((char *) GENEFER);

   if (sqlStatement->FetchRow(true) && count > 0)
   {
      delete sqlStatement;

      if (!ib_HasGenefer80bit)
         return false;
      else
         return true;
   }

   sqlStatement->SetInputParameterValue(candidateName, true);
   sqlStatement->SetInputParameterValue((char *) GENEFER_x64);

   if (sqlStatement->FetchRow(true) && count > 0)
   {
      delete sqlStatement;

      if (!ib_HasGenefer && !ib_HasGenefer80bit)
         return false;
      else
         return true;
   }

   sqlStatement->SetInputParameterValue(candidateName, true);
   sqlStatement->SetInputParameterValue((char *) GENEFER_OpenCL);

   if (sqlStatement->FetchRow(true) && count > 0)
   {
      delete sqlStatement;

      if (!ib_HasGenefer && !ib_HasGenefer80bit && !ib_HasGenefx64)
         return false;
      else
         return true;
   }

   sqlStatement->SetInputParameterValue(candidateName, true);
   sqlStatement->SetInputParameterValue((char *) GENEFER_cuda);

   if (sqlStatement->FetchRow(true) && count > 0)
   {
      delete sqlStatement;

      if (!ib_HasGenefer && !ib_HasGenefer80bit && !ib_HasGenefx64)
         return false;
      else
         return true;
   }

   delete sqlStatement;

   if (!ib_HasGenefx64 && !ib_HasGenefer && !ib_HasGenefer80bit && !ib_HasGeneferGPU)
      return false;
   else
      return true;
}

// This is only called if double-checking is turned on and at least one test has been done.
bool     PrimeWorkSender::CheckDoubleCheck(string candidateName, double decimalLength, int64_t lastUpdateTime)
{
   int32_t  count;
   bool     canDoubleCheck = false;
   SQLStatement *sqlStatement;
   char     emailIDCondition[200], machineIDCondition[200];
   const char *selectSQL = "select count(*) from CandidateTest " \
                           " where CandidateName = ? "
                           "   %s "
                           "   %s ";

   if (ii_DoubleChecker == DC_ANY)
      return true;

   strcpy(emailIDCondition, "");
   strcpy(machineIDCondition, "");

   if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFEMAIL)
      sprintf(emailIDCondition, "and EmailID = ?");
      
   if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFMACHINE)
      sprintf(machineIDCondition, "and MachineID = ?");
      
   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, 
                                    emailIDCondition, machineIDCondition);

   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);

   if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFEMAIL)
      sqlStatement->BindInputParameter(is_EmailID, NAME_LENGTH);

   if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFMACHINE)
      sqlStatement->BindInputParameter(is_MachineID, NAME_LENGTH);

   sqlStatement->BindSelectedColumn(&count);
      
   sqlStatement->SetInputParameterValue(candidateName, true);

   if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFEMAIL)
      sqlStatement->SetInputParameterValue(is_EmailID, false);

   if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFMACHINE)
      sqlStatement->SetInputParameterValue(is_MachineID, false);

   if (sqlStatement->FetchRow(true) && count == 0)
      canDoubleCheck = true;

   delete sqlStatement;

   return canDoubleCheck;
}

bool     PrimeWorkSender::ReserveCandidate(string candidateName)
{
   SQLStatement *sqlStatement;
   bool        didUpdate;
   const char *updateSQL = "update Candidate " \
                           "   set HasPendingTest = 1 " \
                           " where CandidateName = ? " \
                           "   and HasPendingTest = 0";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);

   didUpdate = sqlStatement->Execute();

   // The update only fails if there is an SQL error.  If no rows are updated, then
   // it will say that it was successful, but won't have updated any rows.  Verify
   // that the row was updated before replying that this was successful.
   if (sqlStatement->GetRowsAffected() == 0)
      didUpdate = false;

   if (!didUpdate)
      ip_DBInterface->Rollback();

   delete sqlStatement;

   if (!didUpdate)
      return false;

   return UpdateGroupStats(candidateName);
}

bool     PrimeWorkSender::SendWork(string candidateName, int64_t theK, int32_t theB, int32_t theN, int32_t theC)
{
   int64_t  lastUpdateTime;
   bool     sent;
   SQLStatement *sqlStatement;
   SharedMemoryItem *threadWaiter;
   const char *insertSQL = "insert into CandidateTest " \
                           "( CandidateName, TestID, EmailID, MachineID, InstanceID, UserID, TeamID, EmailSent ) " \
                           "values ( ?,?,?,?,?,?,?,0 )";

   lastUpdateTime = (int64_t) time(NULL);

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, insertSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindInputParameter(lastUpdateTime);
   sqlStatement->BindInputParameter(is_EmailID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_MachineID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_InstanceID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_UserID, ID_LENGTH);
   sqlStatement->BindInputParameter(is_TeamID, ID_LENGTH);

   if (!sqlStatement->Execute())
   {
      delete sqlStatement;
      return false;
   }

   delete sqlStatement;

   threadWaiter = ip_Socket->GetThreadWaiter();
   threadWaiter->Lock();

   if (ii_ServerType == ST_GFN)
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64" %d %u", candidateName.c_str(), lastUpdateTime, theB, theN);
   else if (ii_ServerType == ST_XYYX)
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64" %d %d %d", candidateName.c_str(), lastUpdateTime, 
         (theC == 1 ? theB : theN), (theC == 1 ? theN : theB), theC);
   else if (ii_ServerType == ST_GENERIC)
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64"", candidateName.c_str(), lastUpdateTime);
   else if (ii_ServerType == ST_PRIMORIAL || ii_ServerType == ST_FACTORIAL)
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64" %u %d", candidateName.c_str(), lastUpdateTime, theN, theC);
   else if (ii_ServerType == ST_MULTIFACTORIAL)
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64" %u %u %d", candidateName.c_str(), lastUpdateTime, theN, theB, theC);
   else if (ii_ServerType == ST_CYCLOTOMIC)
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64" %" PRId64" %d %u", candidateName.c_str(), lastUpdateTime, theK, theB, theN);
   else if (ii_ServerType == ST_WAGSTAFF)
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64" %d", candidateName.c_str(), lastUpdateTime, theN);
   else 
      sent = ip_Socket->Send("WorkUnit: %s %" PRId64" %" PRId64" %d %u %d", candidateName.c_str(), lastUpdateTime, theK, theB, theN, theC);

   threadWaiter->Release();

   if (!sent)
   {
      ip_Log->LogMessage("%d: Tried to send %s to Email: %s  User: %s  Machine: %s  Instance: %s  but got no acknowledgement",
                         ip_Socket->GetSocketID(), candidateName.c_str(), is_EmailID.c_str(),
                         is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
      return false;
   }

   if (ib_BriefTestLog)
      ip_Log->TestMessage("%d: %s sent to %s/%s/%s/%s", ip_Socket->GetSocketID(),
                          candidateName.c_str(), is_EmailID.c_str(), is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());
   else
      ip_Log->TestMessage("%d: %s sent to Email: %s  User: %s  Machine: %s  Instance: %s", ip_Socket->GetSocketID(),
                          candidateName.c_str(), is_EmailID.c_str(), is_UserID.c_str(), is_MachineID.c_str(), is_InstanceID.c_str());

   return true;
}

bool     PrimeWorkSender::UpdateGroupStats(string candidateName)
{
   SQLStatement* sqlStatement;
   int64_t     theK;
   int32_t     theB, theC;
   bool        success;
   const char* selectSQL = "select k, b, c " \
                           "  from Candidate " \
                           " where CandidateName = ?";
   const char* updateSQL = "update CandidateGroupStats " \
                           "   set CountInProgress = CountInProgress + 1 " \
                           " where k = %" PRId64 " " \
                           "   and b = %d " \
                           "   and c = %d ";


   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&theK);
   sqlStatement->BindSelectedColumn(&theB);
   sqlStatement->BindSelectedColumn(&theC);

   success = sqlStatement->FetchRow(true);

   delete sqlStatement;

   if (!success)
      return false;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, theK, theB, theC);

   // It is okay if no rows are updated as we don't want CountInProgress to go negative
   success = sqlStatement->Execute();

   if (!success)
      ip_DBInterface->Rollback();

   delete sqlStatement;

   return success;
}
