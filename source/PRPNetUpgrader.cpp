#include "PRPNetUpgrader.h"
#include "LengthCalculator.h"
#include "StatsUpdaterFactory.h"
#include "StatsUpdater.h"
#include "SQLStatement.h"

PRPNetUpgrader::PRPNetUpgrader(globals_t *theGlobals, DBInterface *dbInterface)
{
   ip_DBInterface = dbInterface;
   ip_Log = theGlobals->p_Log;
   ii_ServerType = theGlobals->i_ServerType;
   ii_NeedsDoubleCheck = theGlobals->i_NeedsDoubleCheck;

   ii_CandidateCount = 0;
   ii_CandidateSize = 0;
   ii_Candidates = new Candidate *[ii_CandidateSize];
}

void  PRPNetUpgrader::UpgradeFromV2ToV3(void)
{
   StatsUpdaterFactory *suf;
   StatsUpdater *su;

   printf("The server will import files into the database.  This could take a few minutes.\n");

   ProcessUserStatsFile("user_stats.ini");
   LoadCandidates("prpserver.candidates");

   suf = new StatsUpdaterFactory();

   su = suf->GetInstance(ip_DBInterface, ip_Log, ii_ServerType);
   su->RollupGroupStats(true);
   delete suf;
   delete su;

   printf("\n\nThe server will now shut down so that you can verify that the upgrade was successful.\n\n");
}

void  PRPNetUpgrader::LoadCandidates(char *saveFileName)
{
   FILE      *fp;
   char      *readBuf, *ptr, lastName[100];
   Candidate *theCandidate = 0;
   int32_t    failedCount = 0, successCount = 0, i;
   LengthCalculator *lengthCalculator = 0;

   if ((fp = fopen(saveFileName, "r")) == NULL)
   {
      printf("File [%s] could not be opened.  Exiting\n", saveFileName);
      exit(0);
   }

   readBuf = new char[BUFFER_SIZE];

   memset(lastName, 0x00, sizeof(lastName));
   while (fgets(readBuf, BUFFER_SIZE, fp) != NULL)
   {
      StripCRLF(readBuf);

      // Empty line, skip it
      if (strlen(readBuf) == 0)
         continue;

      ptr = strchr(readBuf, ' ');
      if (!ptr)
      {
         printf("Line [%s] in file %s is not formatted correctly.\n", readBuf, saveFileName);
         continue;
      }

      *ptr = 0x00;

      // The 'N' line for each candidate must be first and each successive line that is
      // not 'N' must be for the same candidate
      if ((*(ptr+1) != 'N') && strcmp(lastName, readBuf))
      {
         printf("Name [%s] is out of sequence.  Please fix the issue and restart the server\n", readBuf);
         exit(-1);
      }

      switch (*(ptr+1))
      {
         case 'N':
            strcpy(lastName, readBuf);
            theCandidate = new Candidate(ip_Log, ii_ServerType, lastName, ptr+3, ii_NeedsDoubleCheck);
            AddCandidate(theCandidate);
            break;

         case 'T':
            theCandidate->AddTest(ptr+3);
            break;

         case 'R':
            theCandidate->AddGeneferROE(ptr+3);
            break;
      }
   }

   delete [] readBuf;

   fclose(fp);

   for (i=0; i<ii_CandidateCount; i++)
   {
      if (ii_Candidates[i]->WriteToDB(ip_DBInterface))
      {
         ip_DBInterface->Commit();
         successCount++;
      }
      else
      {
         ip_DBInterface->Rollback();
         failedCount++;
      }

      delete ii_Candidates[i];

      if ((i % 1000) == 0)
      {
          printf(" Imported %d of %d candidates\r", i, ii_CandidateCount);
          fflush(stdout);
      }
   }

   delete ii_Candidates;

   printf("Import done.  Now computing decimal lengths. . .\n");
   lengthCalculator = new LengthCalculator(ii_ServerType, ip_DBInterface, ip_Log);
   lengthCalculator->CalculateDecimalLengths(0);
   delete lengthCalculator;

   ip_Log->LogMessage("%d candidates successfully inserted into database", successCount);
   ip_Log->LogMessage("%d candidates could not be inserted into database", failedCount);
}

// Add a candidate to the list
int32_t PRPNetUpgrader::AddCandidate(Candidate *theCandidate)
{
   uint32_t    index;
   Candidate **c2;

   ii_CandidateCount++;

   if (ii_CandidateCount >= ii_CandidateSize)
   {
      ii_CandidateSize += CANDIDATE_SIZE;

      c2 = new Candidate *[ii_CandidateSize];
      memcpy(c2, ii_Candidates, ii_CandidateCount * sizeof(Candidate *));
      delete [] ii_Candidates;
      ii_Candidates = c2;
   }

   index = ii_CandidateCount - 1;
   ii_Candidates[index] = theCandidate;

   return index;
}

void  PRPNetUpgrader::ProcessUserStatsFile(char *userStatsFile)
{
   FILE    *fp;
   char     line[400], userID[100];
   int32_t  testsPerformed, prpsFound, goodScan;
   int32_t  primesFound, gfnDivisorsFound;
   int32_t  failedCount = 0, successCount = 0;
   double   totalScore;
   SQLStatement *sqlStatement;

   fp = fopen(userStatsFile, "r");

   if (!fp)
      return;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface);

   while (fgets(line, sizeof(line), fp) != NULL)
   {
      goodScan = false;
      primesFound = 0;
      gfnDivisorsFound = 0;

      if (sscanf(line, "%s %u %u %u %u %lf", userID, &testsPerformed, &prpsFound, &primesFound, &gfnDivisorsFound, &totalScore) == 6)
         goodScan = true;
      else if (sscanf(line, "%s %u %u %lf", userID, &testsPerformed, &prpsFound, &totalScore) == 4)
         goodScan = true;

      sqlStatement->PrepareInsert("UserStats");

      sqlStatement->InsertNameValuePair("UserID", userID);
      sqlStatement->InsertNameValuePair("TestsPerformed", testsPerformed);
      sqlStatement->InsertNameValuePair("PRPsFound", prpsFound);
      sqlStatement->InsertNameValuePair("PrimesFound", primesFound);
      sqlStatement->InsertNameValuePair("GFNDivisorsFound", gfnDivisorsFound);
      sqlStatement->InsertNameValuePair("totalScore", totalScore);

      if (sqlStatement->ExecuteInsert())
      {
         ip_DBInterface->Commit();
         successCount++;
      }
      else
      {
         ip_DBInterface->Rollback();
         failedCount++;
      }
   }

   delete sqlStatement;

   ip_DBInterface->Commit();

   ip_Log->LogMessage("%d users successfully inserted into database", successCount);
   ip_Log->LogMessage("%d users could not be inserted into database", failedCount);
}
