#include "PrimeHelperThread.h"

#include "ABCParser.h"
#include "LengthCalculator.h"
#include "SQLStatement.h"
#include "StatsUpdaterFactory.h"
#include "StatsUpdater.h"
#include "PrimeWorkSender.h"
#include "PrimeWorkReceiver.h"

void  PrimeHelperThread::ProcessRequest(string theMessage)
{
   char     tempMessage[200];
   PrimeWorkSender *workSender;
   PrimeWorkReceiver *workReceiver;

   strcpy(tempMessage, theMessage.c_str());
   if (!memcmp(tempMessage, "ADMIN_STATS ", 12))
      if (VerifyAdminPassword(tempMessage + 12))
         AdminStatsUpdate(false);

   if (!memcmp(tempMessage, "ADMIN_SERVER_STATS ", 19))
      if (VerifyAdminPassword(tempMessage + 19))
         AdminStatsUpdate(false);

   if (!memcmp(tempMessage, "ADMIN_USER_STATS ", 17))
      if (VerifyAdminPassword(tempMessage + 17))
         AdminStatsUpdate(true);

   if (!memcmp(tempMessage, "ADMIN_ABC ", 10))
      if (VerifyAdminPassword(tempMessage + 10))
         AdminABCFile();

   if (!memcmp(tempMessage, "ADMIN_FACTOR ", 13))
      if (VerifyAdminPassword(tempMessage + 13))
         AdminFactorFile();

   if (!memcmp(tempMessage, "EXPIRE_WORKUNIT ", 16))
      if (VerifyAdminPassword(tempMessage + 16))
         ExpireWorkunitTest();

   if (!memcmp(tempMessage, "GETWORK", 7))
   {
      workSender = new PrimeWorkSender(ip_DBInterface, ip_Socket, ip_Globals,
                                       is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
      workSender->ProcessMessage(tempMessage);
      delete workSender;
   }

   if (!memcmp(tempMessage, "RETURNWORK", 10))
   {
      workReceiver = new PrimeWorkReceiver(ip_DBInterface, ip_Socket, ip_Globals,
                                           is_UserID, is_EmailID, is_MachineID, is_InstanceID, is_TeamID);
      workReceiver->ProcessMessage(tempMessage);
      delete workReceiver;
   }
}

void      PrimeHelperThread::AdminABCFile(void)
{
   SQLStatement *selectStatement;
   ABCParser *abcParser;
   LengthCalculator *lengthCalculator;
   StatsUpdaterFactory *suf;
   StatsUpdater *su;
   int32_t    totalEntries, newEntries, badEntries, dupEntries, failedInserts;
   int32_t    countFound;
   int64_t    theK;
   int32_t    theB, theN, theC;
   double     decimalLength;
   string     candidateName;
   const char  *selectSQL = "select count(*) from Candidate " \
                            " where CandidateName = ?";

   abcParser = new ABCParser(ip_Socket, ii_ServerType);

   if (!abcParser->IsValidFormat())
   {
      delete abcParser;
      return;
   }

   suf = new StatsUpdaterFactory();
   su = suf->GetInstance(ip_DBInterface, ip_Log, ii_ServerType, ip_Globals->b_NeedsDoubleCheck);
   delete suf;

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(candidateName, NAME_LENGTH, false);
   selectStatement->BindSelectedColumn(&countFound);

   totalEntries = newEntries = badEntries = dupEntries = failedInserts = 0;
   lengthCalculator = new LengthCalculator(ii_ServerType, ip_DBInterface, ip_Log);

   while (abcParser->GetNextCandidate(candidateName, theK, theB, theN, theC))
   {
      // Tell the admin tool that we are still here and have processed
      // everything it has sent.  Now the admin tool can send more.
      if (!memcmp(candidateName.c_str(), "sent", 4))
      {
         ip_Socket->Send("processed %d records", totalEntries);
         continue;
      }

      if (!memcmp(candidateName.c_str(), "ABC ", 4))
         continue;

      totalEntries++;

      if (theC != 1 && theC != -1 && (ii_ServerType == ST_SOPHIEGERMAIN || ii_ServerType == ST_TWIN))
         badEntries++;
      else
      {
         selectStatement->SetInputParameterValue(candidateName, true);

         if (selectStatement->FetchRow(true) && countFound == 0)
         {
            if (ii_ServerType == ST_GENERIC)
               decimalLength = 0.0;
            else
               decimalLength = lengthCalculator->CalculateDecimalLength(theK, theB, theN);

            if (!su->InsertCandidate(candidateName, theK, theB, theN, theC, decimalLength))
               failedInserts++;
            else
               newEntries++;

            if (newEntries % 100 == 0)
               ip_DBInterface->Commit();
         }
         else
         {
            dupEntries++;
            ip_Socket->Send("ERR: %s is a duplicate and has been rejected", candidateName.c_str());
         }

         if (totalEntries % 1000 == 0)
         {
            printf(" Processed %d entries from the admin tool\r", totalEntries);
            fflush(stdout);
         }
      }
   }

   ip_DBInterface->Commit();

   delete selectStatement;
   delete abcParser;

   if (totalEntries >= 1000)
   {
      printf(" Processed %d entries from the admin tool\n", totalEntries);
      fflush(stdout);
   }

   if (ii_ServerType == ST_GENERIC || ii_ServerType == ST_FACTORIAL ||
      ii_ServerType == ST_MULTIFACTORIAL || ii_ServerType == ST_PRIMORIAL)
      lengthCalculator->CalculateDecimalLengths(ip_Socket);

   delete lengthCalculator;

   ip_Socket->Send("Total candidates received = %6d", totalEntries);
   ip_Socket->Send("           Failed Inserts = %6d", failedInserts);
   ip_Socket->Send("     New candidates added = %6d", newEntries);
   ip_Socket->Send("      Duplicates rejected = %6d", dupEntries);
   ip_Socket->Send("Bad entries from ABC file = %6d", badEntries);
   ip_Socket->Send("End of Message");

   // These should be done after the admin tool is informed of the results
   // because they could take a wile and the admin tool really doesn't care at
   // this point if this is successful or not
   if (!su->RollupGroupStats(true))
      ip_DBInterface->Rollback();
   else
      ip_DBInterface->Commit();
   delete su;

   ip_Log->LogMessage("ADMIN:  %d new candidates added through the admin process", newEntries);
}

void      PrimeHelperThread::AdminFactorFile(void)
{
   FILE      *factorFile;
   SQLStatement *selectStatement;
   SQLStatement *deleteStatement1;
   SQLStatement *deleteStatement2;
   SQLStatement *deleteStatement3;
   SQLStatement *deleteStatement4;
   StatsUpdaterFactory *suf;
   StatsUpdater *su;
   char      *theMessage, candidateName[NAME_LENGTH+1];
   int32_t    totalFactors, badFactors, acceptedFactors, rejectedFactors;
   int32_t    endLoop, countFound, failedUpdates;
   int64_t    theFactor;
   const char *selectSQL = "select count(*) from Candidate " \
                           " where CandidateName = ? ";
   const char *deleteROE = "delete from GeneferROE where CandidateName = ? ";
   const char *deleteResult = "delete from CandidateTestResult where CandidateName = ? ";
   const char *deleteTest = "delete from CandidateTest where CandidateName = ? ";
   const char *deleteCandidate = "delete from Candidate where CandidateName = ? ";

   factorFile = fopen("factors.txt", "a+");
   if (!factorFile)
   {
      ip_Log->LogMessage("Could not open file factors.txt to hold factors from admin");
      ip_Socket->Send("Server cannot accept factors due to I/O error");
      ip_Socket->Send("End of Message");
      return;
   }

   totalFactors = badFactors = acceptedFactors = rejectedFactors = failedUpdates = 0;

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindInputParameter(candidateName, NAME_LENGTH, false);
   selectStatement->BindSelectedColumn(&countFound);

   deleteStatement1 = new SQLStatement(ip_Log, ip_DBInterface, deleteROE);
   deleteStatement1->BindInputParameter(candidateName, NAME_LENGTH, false);
   deleteStatement2 = new SQLStatement(ip_Log, ip_DBInterface, deleteResult);
   deleteStatement2->BindInputParameter(candidateName, NAME_LENGTH, false);
   deleteStatement3 = new SQLStatement(ip_Log, ip_DBInterface, deleteTest);
   deleteStatement3->BindInputParameter(candidateName, NAME_LENGTH, false);
   deleteStatement4 = new SQLStatement(ip_Log, ip_DBInterface, deleteCandidate);
   deleteStatement4->BindInputParameter(candidateName, NAME_LENGTH, false);

   endLoop = false;
   theMessage = ip_Socket->Receive();
   while (theMessage && !endLoop)
   {
      // Tell the admin tool that we are still here and have processed
      // everything it has sent.  Now the admin tool can send more.
      if (!memcmp(theMessage, "sent", 4))
         ip_Socket->Send("processed %d records", totalFactors);
      else if (!memcmp(theMessage, "End of File", 11))
         endLoop = true;
      else
      {
         totalFactors++;

         if (sscanf(theMessage, "%" PRId64" | %s", &theFactor, candidateName) != 2)
         {
            badFactors++;
            ip_Socket->Send("Factor line is not of the correct format", theMessage);
         }
         else
         {
            selectStatement->SetInputParameterValue(candidateName, true);
            deleteStatement1->SetInputParameterValue(candidateName, true);
            deleteStatement2->SetInputParameterValue(candidateName, true);
            deleteStatement3->SetInputParameterValue(candidateName, true);
            deleteStatement4->SetInputParameterValue(candidateName, true);
            if (selectStatement->FetchRow(true) && countFound == 1)
            {
               if (deleteStatement1->Execute() && deleteStatement2->Execute() && 
                   deleteStatement3->Execute() && deleteStatement4->Execute())
               {
                  fprintf(factorFile, "%s", theMessage);
                  acceptedFactors++;
                  ip_DBInterface->Commit();
               }
               else
                  failedUpdates++;
            }
            else
            {
               rejectedFactors++;
               ip_Socket->Send("Candidate %s not found on server", candidateName);
            }
         }

         if (totalFactors % 1000 == 0)
         {
            printf(" Processed %d entries from the admin tool\r", totalFactors);
            fflush(stdout);
         }
      }

      if (!endLoop)
         theMessage = ip_Socket->Receive();
   }

   delete selectStatement;
   delete deleteStatement1;
   delete deleteStatement2;
   delete deleteStatement3;
   delete deleteStatement4;
   fclose(factorFile);

   if (totalFactors >= 1000)
   {
      printf(" Processed %d entries from the admin tool\n", totalFactors);
      fflush(stdout);
   }

   suf = new StatsUpdaterFactory();
   su = suf->GetInstance(ip_DBInterface, ip_Log, ii_ServerType, ip_Globals->b_NeedsDoubleCheck);
   if (!su->RollupGroupStats(true))
      ip_DBInterface->Rollback();
   else
      ip_DBInterface->Commit();
   delete su;
   delete suf;

   ip_Log->LogMessage("ADMIN:  %d candidates were removed via a factor file through admin process", acceptedFactors);
   ip_Socket->Send("Total factors received = %6d", totalFactors);
   ip_Socket->Send("       Factors applied = %6d", acceptedFactors);
   ip_Socket->Send("        Failed updates = %6d", failedUpdates);
   ip_Socket->Send("      Unusable factors = %6d", rejectedFactors);
   ip_Socket->Send("       Invalid factors = %6d", badFactors);
   ip_Socket->Send("End of Message");
}

void      PrimeHelperThread::ExpireWorkunitTest(void)
{
   char *theMessage;
   int32_t rows;
   bool  success;
   SQLStatement *sqlStatement;
   const char *deleteSQL = "delete from CandidateTest " \
                           " where CandidateName = ? " \
                           "   and IsCompleted = 0";
   const char *updateSQL = "update Candidate " \
                           "   set HasPendingTest = 0 " \
                           " where CandidateName = ?";

   theMessage = ip_Socket->Receive();
   if (!theMessage) return;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, deleteSQL);
   sqlStatement->BindInputParameter(theMessage+7, NAME_LENGTH);

   success = sqlStatement->Execute();
   rows = sqlStatement->GetRowsAffected();

   delete sqlStatement;

   if (success && rows)
   {
      sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
      sqlStatement->BindInputParameter(theMessage+7, NAME_LENGTH);
      success = sqlStatement->Execute();
      delete sqlStatement;
   }

   if (success)
   {
      if (rows)
      {
         ip_Log->LogMessage("ADMIN:  Test for candidate %s was expired", theMessage+7);
         ip_Socket->Send("Test for candidate %s was expired", theMessage+7);
         ip_DBInterface->Commit();
      }
      else
      {
         ip_Socket->Send("Test for candidate %s was not found", theMessage+7);
         ip_DBInterface->Rollback();
      }
   }
   else
   {
      ip_Socket->Send("Failed to expire test for candidate %s", theMessage+7);
      ip_DBInterface->Rollback();
   }

   ip_Socket->Send("End of Message");
}
