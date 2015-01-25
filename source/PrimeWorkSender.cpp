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
   is_SortSequence = globals->s_SortSequence;
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
   int32_t      sendWorkUnits, sentWorkUnits;
   char         clientVersion[20];
   char         tempMessage[200];

   strcpy(tempMessage, theMessage.c_str());

   clientVersion[0] = 0;
   if (sscanf(tempMessage, "GETWORK %s %u", clientVersion, &sendWorkUnits) != 2)
      sendWorkUnits = 1;

   is_ClientVersion = clientVersion;

   if ((ii_ServerType == ST_PRIMORIAL || ii_ServerType == ST_FACTORIAL) && !ib_HasPFGW)
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

   sentWorkUnits = ib_NoNewWork ? 0 : SelectCandidates(sendWorkUnits, ib_OneKPerInstance);

   if (!sentWorkUnits && ib_OneKPerInstance && !ib_NoNewWork)
      sentWorkUnits = SelectCandidates(sendWorkUnits, false);

   if (!sentWorkUnits)
   {
      if (ib_NoNewWork)
         ip_Socket->Send("INACTIVE: New work generation is disabled on this server.");
      else
         ip_Socket->Send("INACTIVE: No available candidates are left on this server.");
   }

   ip_Socket->Send("End of Message");

   if (sentWorkUnits > 0)
      SendGreeting("Greeting.txt");
}

int32_t  PrimeWorkSender::SelectCandidates(int32_t sendWorkUnits, bool oneKPerInstance)
{
   KeepAliveThread *keepAliveThread;
   SharedMemoryItem *threadWaiter;
   SQLStatement *selectStatement;
   char     candidateName[NAME_LENGTH+1];
   const char    *dcWhere = "", *candidateWhere = "";
   int64_t  theK, prevK = 0, lastUpdateTime;
   int32_t  theB, prevB = 0, theC, prevC = -2, theN;
   bool     encounteredError, endLoop;
   bool     kForOtherInstance = false;
   int32_t  sentWorkUnits, completedTests;
   double   decimalLength;
   const char *selectSQL = "select CandidateName, CompletedTests, " \
                           "       DecimalLength, LastUpdateTime, " \
                           "       k, b, c, n " \
                           "  from Candidate " \
                           " where HasPendingTest = 0 " \
                           "   and $null_func$(MainTestResult, 0) = 0 " \
                           "   and DoubleChecked = 0 " \
                           "   and HasSierpinskiRieselPrime = 0 " \
                           "   %s " \
                           "   %s " \
                           "   and DecimalLength > 0 " \
                           "order by %s limit 100";

   // If not double-checking, then don't return candidates that have a completed test
   if (!ib_NeedsDoubleCheck)
      dcWhere = " and CompletedTests = 0";

   if (ii_ServerType == ST_SIERPINSKIRIESEL && oneKPerInstance)
      candidateWhere = "and k >= ?";
   else
      candidateWhere = "and CandidateName > ?";

   threadWaiter = ip_Socket->GetThreadWaiter();
   threadWaiter->SetValueNoLock(KAT_WAITING);
   keepAliveThread = new KeepAliveThread();
   keepAliveThread->StartThread(ip_Socket);

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL,
                                      dcWhere, candidateWhere, is_SortSequence.c_str());
   if (ii_ServerType == ST_SIERPINSKIRIESEL && oneKPerInstance)
      selectStatement->BindInputParameter(prevK);
   else
      selectStatement->BindInputParameter(candidateName, NAME_LENGTH, false);

   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   selectStatement->BindSelectedColumn(&completedTests);
   selectStatement->BindSelectedColumn(&decimalLength);
   selectStatement->BindSelectedColumn(&lastUpdateTime);
   selectStatement->BindSelectedColumn(&theK);
   selectStatement->BindSelectedColumn(&theB);
   selectStatement->BindSelectedColumn(&theC);
   selectStatement->BindSelectedColumn(&theN);

   sentWorkUnits = 0;
   encounteredError = false;
   strcpy(candidateName, " ");

   while (sentWorkUnits < sendWorkUnits && !encounteredError)
   {
      if (ii_ServerType == ST_SIERPINSKIRIESEL && oneKPerInstance)
      {
         if (kForOtherInstance) prevK++;
         selectStatement->SetInputParameterValue(prevK, true);
      }
      else
         selectStatement->SetInputParameterValue(candidateName, true);

      kForOtherInstance = false;
      while (sentWorkUnits < sendWorkUnits && !encounteredError)
      {
         if (!selectStatement->FetchRow(false)) break;

         ip_Log->Debug(DEBUG_WORK, "%d: Checking candidate %s", ip_Socket->GetSocketID(), candidateName);

         if (completedTests > 0 && !CheckDoubleCheck(candidateName, decimalLength, lastUpdateTime))
         {
            ip_Log->Debug(DEBUG_WORK, "%d: Cannot double-check %s", ip_Socket->GetSocketID(), candidateName);
            continue;
         }

         if (ii_ServerType == ST_GFN && !CheckGenefer(candidateName))
         {
            ip_Log->Debug(DEBUG_WORK, "%d: Has wrong version of genefer %s", ip_Socket->GetSocketID(), candidateName);
            continue;
         }

         // If we can only send one k/b/c per client, then verify that no test for the
         // given k/b/c has been assigned to another instance.
         if (ii_ServerType == ST_SIERPINSKIRIESEL && oneKPerInstance)
         {
            if (prevK != theK || prevB != theB || prevC != theC)
            {
               kForOtherInstance = CheckOneKPerInstance(theK, theB, theC);
               prevK = theK;
               prevB = theB;
               prevC = theC;
            }

            if (kForOtherInstance)
            {
               ip_Log->Debug(DEBUG_WORK, "%d: k/b/c given to another instance %s", ip_Socket->GetSocketID(), candidateName);
               break;
            }
         }

         // Allow only one thread through here at a time
         ip_Locker->Lock();

         if (ReserveCandidate(candidateName))
         {
            if (completedTests == 0)
               ip_Log->Debug(DEBUG_WORK, "First check for candidate %s", candidateName);
            else
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

      if (ib_OneKPerInstance && kForOtherInstance)
         continue;

      // If we didn't retrieve enough rows (based upon our limit of 100), then we are done
      if (selectStatement->GetFetchedRowCount() < 100)
         break;
   }

   // Just in case a commit or rollback was not done
   ip_DBInterface->Rollback();

   delete selectStatement;

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

bool     PrimeWorkSender::CheckGenefer(string candidateName)
{
   SQLStatement *sqlStatement;
   int32_t  count;
   char     programName[NAME_LENGTH+1];
   const char *selectSQL = "select count(*) from GeneferROE " \
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
   int64_t  diffTime;
   int32_t  jj, count;
   bool     canDoubleCheck = false;
   SQLStatement *sqlStatement;
   char     emailIDCondition[200], machineIDCondition[200];
   const char *selectSQL = "select count(*) from CandidateTest " \
                           " where CandidateName = ? "
                           "   %s "
                           "   %s ";

   diffTime = (int64_t) time(NULL);
   diffTime -= lastUpdateTime;

   jj = 0;
   while (jj < ii_DelayCount && ip_Delay[jj].maxLength < decimalLength)
      jj++;

   if (diffTime > ip_Delay[jj].doubleCheckDelay)
   {
      if (ii_DoubleChecker == DC_ANY)
         return true;

      strcpy(emailIDCondition, "");
      strcpy(machineIDCondition, "");

      if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFEMAIL)
         sprintf(emailIDCondition, "and EmailID = %s", is_EmailID.c_str());
      
      if (ii_DoubleChecker == DC_DIFFBOTH || ii_DoubleChecker == DC_DIFFMACHINE)
         sprintf(machineIDCondition, "and MachineID = %s", is_MachineID.c_str());
      
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL, 
                                      emailIDCondition, machineIDCondition);

      sqlStatement->BindInputParameter(candidateName, NAME_LENGTH);
      sqlStatement->BindSelectedColumn(&count);
      
      sqlStatement->SetInputParameterValue(candidateName, true);

      if (sqlStatement->FetchRow(true) && count == 0)
         canDoubleCheck = true;

      delete sqlStatement;
   }

   return canDoubleCheck;
}

// If there is a pending for another user/email/instance, then do not send any tests
// for this k/b/c to the client.
bool     PrimeWorkSender::CheckOneKPerInstance(int64_t k, int32_t b, int32_t c)
{
   int32_t  count;
   SQLStatement *sqlStatement;
   bool  kForOtherClient = false;
   const char *selectSQL = "select count(*) "
                           "  from Candidate c, CandidateTest ct " \
                           " where ct.CandidateName = c.CandidateName " \
                           "   and k = ? and b = ? and c = ? " \
                           "   and (ct.EmailID <> ? or ct.MachineID <> ? or ct.InstanceID <> ?) " \
                           "   and c.HasPendingTest = 1 ";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(k);
   sqlStatement->BindInputParameter(b);
   sqlStatement->BindInputParameter(c);
   sqlStatement->BindInputParameter(is_EmailID.c_str(), ID_LENGTH);
   sqlStatement->BindInputParameter(is_MachineID.c_str(), ID_LENGTH);
   sqlStatement->BindInputParameter(is_InstanceID.c_str(), ID_LENGTH);
   sqlStatement->BindSelectedColumn(&count);

   if (sqlStatement->FetchRow(true) && count > 0)
      kForOtherClient = true;

   delete sqlStatement;

   return kForOtherClient;
}

bool     PrimeWorkSender::ReserveCandidate(string candidateName)
{
   SQLStatement *sqlStatement;
   bool     didUpdate;
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

   return didUpdate;
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
      sent = ip_Socket->Send("WorkUnit: %s %"PRId64" %d %u", candidateName.c_str(), lastUpdateTime, theB, theN);
   else if (ii_ServerType == ST_XYYX)
      sent = ip_Socket->Send("WorkUnit: %s %"PRId64" %d %u %d", candidateName.c_str(), lastUpdateTime, theB, theN, theC);
   else if (ii_ServerType == ST_GENERIC)
      sent = ip_Socket->Send("WorkUnit: %s %"PRId64"", candidateName.c_str(), lastUpdateTime);
   else if (ii_ServerType == ST_PRIMORIAL || ii_ServerType == ST_FACTORIAL)
      sent = ip_Socket->Send("WorkUnit: %s %"PRId64" %u %d", candidateName.c_str(), lastUpdateTime, theN, theC);
   else if (ii_ServerType == ST_CYCLOTOMIC)
      sent = ip_Socket->Send("WorkUnit: %s %"PRId64" %"PRId64" %d %u", candidateName.c_str(), lastUpdateTime, theK, theB, theN);
   else
      sent = ip_Socket->Send("WorkUnit: %s %"PRId64" %"PRId64" %d %u %d", candidateName.c_str(), lastUpdateTime, theK, theB, theN, theC);

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
