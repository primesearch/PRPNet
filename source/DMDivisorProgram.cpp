#include <stdlib.h>
#include "DMDivisorProgram.h"

void     DMDivisorProgram::SendStandardizedName(Socket* theSocket, uint32_t returnWorkUnit)
{
   theSocket->Send(GetStandardizedName().c_str());
}

testresult_t   DMDivisorProgram::Execute(int32_t serverType, uint32_t n, uint64_t minK, uint64_t maxK)
{
   char           command[200], line[200], rate[200];
   testresult_t   testResult;
   FILE* fp;

   ii_ServerType = serverType;
   rate[0] = 0;

   if ((fp = fopen("dm_rates.txt", "r")) != NULL)
   {
      uint32_t dmN, factorsPerSecond, secondsPerFactor, minutesForRate;

      while (fgets(line, 2000, fp) != NULL)
      {
         if (memcmp(line, "rate=", 5))
            continue;

         StripCRLF(line);
         if (sscanf(line, "rate=%u:%u:%u:%u", &dmN, &factorsPerSecond, &secondsPerFactor, &minutesForRate) == 4)
         {
            if ((factorsPerSecond == 0 && secondsPerFactor == 0) ||
               (factorsPerSecond > 0 && secondsPerFactor > 0))
            {
               ip_Log->LogMessage("%s: dm_rates.txt line %s is invalid", is_Suffix.c_str(), line);
#ifdef WIN32
               SetQuitting(1);
#else
               raise(SIGINT);
#endif
               return TR_CANCELLED;
            }

            if (dmN == n)
            {
               if (factorsPerSecond > 0)
                  snprintf(rate, sizeof(rate), "-4%u", factorsPerSecond);

               if (secondsPerFactor > 0)
                  snprintf(rate, sizeof(rate), "-5%u -6%u", secondsPerFactor, minutesForRate);

               break;
            }
         }
      }

      fclose(fp);
   }

   if (DoesFileExist("dmdsieve.log"))
      unlink("dmdsieve.log");

   snprintf(command, 200, "%s %s -x -y -n%d -k%" PRIu64" -K%" PRIu64"", is_ExeName.c_str(),
      rate, n, minK, maxK);

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   testResult = ParseTestResults();

   return testResult;
}

// Parse the output from WWWW and determine if the candidate is PRP
testresult_t   DMDivisorProgram::ParseTestResults()
{
   char        line[250], fileName[30], *ptr;
   bool        isCompleted = false, testedForDivisors = false;
   int32_t     tryCount;
   FILE* fp;

   id_Seconds = 0;
   ip_FirstDMDivisor = nullptr;
   strcpy(fileName, "dmdsieve.log");

   Sleep(100);
   tryCount = 1;
   while ((fp = fopen(fileName, "r")) == NULL)
   {
      // Try up to five times to open the file before bailing
      if (tryCount < 5)
      {
         Sleep(500);
         tryCount++;
      }
      else
      {
         ip_Log->LogMessage("%s: Could not open file [%s] for reading.  Assuming user stopped with ^C",
            is_Suffix.c_str(), fileName);
#ifdef WIN32
         SetQuitting(1);
#else
         raise(SIGINT);
#endif
         return TR_CANCELLED;
      }
   }

   while (fgets(line, sizeof(line), fp))
   {
      if (strstr(line, "Stopped due to removal rate"))
         isCompleted = true;

      if (strstr(line, "Double-Mersenne divisibility check for") && strstr(line, "finished"))
         testedForDivisors = true;

      if (strstr(line, "Primes tested: ") == nullptr)
         continue;

      ptr = strstr(line, "Time: ");
      if (ptr)
         sscanf(ptr, "Time: %lf", &id_Seconds);
   }
   fclose(fp);

   if (!isCompleted || !testedForDivisors)
   {
      ip_Log->LogMessage("%s: Could not find completion line in log file [%s].  Assuming user stopped with ^C",
         is_Suffix.c_str(), fileName);
#ifdef WIN32
      SetQuitting(1);
#else
      raise(SIGINT);
#endif
      return TR_CANCELLED;
   }

   strcpy(fileName, "dm_factors.txt");
   if ((fp = fopen(fileName, "r")) != NULL)
   {
      while (fgets(line, sizeof(line), fp))
      {
         if (!strlen(line))
            continue;

         ptr = strstr(line, " of ");

         if (memcmp(line, "Found factor ", 13) || !ptr)
         {
            ip_Log->LogMessage("%s:  File [%s] has malformed line.  Cannot continue",
               is_Suffix.c_str(), fileName);
#ifdef WIN32
            SetQuitting(1);
#else
            raise(SIGINT);
#endif
            return TR_CANCELLED;
         }

         *ptr = 0;

         AddDMDivisorToList(line + 13);
      }

      fclose(fp);
      unlink("dm_factors.txt");
   }

   unlink("dmdsieve.log");

   return TR_COMPLETED;
}

void  DMDivisorProgram::DetermineVersion(void)
{
   char  command[200], line[200], * ptr1, * ptr2 = 0;
   FILE* fp;

   snprintf(command, 200, "%s -h > dmd.out", is_ExeName.c_str());

   system(command);

   fp = fopen("dmd.out", "r");
   if (!fp)
   {
      printf("Could not determine version of dmdsieve being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("dmd.out");

      printf("Could not determine version of dmdsieve being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("dmd.out");

   // The first line should look like this:
   //   dmdsieve v1.0, a program to find terms that are potential factors of 2^(2^n-1)-1
   ptr1 = strstr(line, " v");
   if (ptr1)
      ptr2 = strchr(ptr1, ',');

   if (!ptr1 || !ptr2)
   {
      printf("Could not determine version of dmdsieve being used.  Missing version data\n");
      exit(0);
   }
   *ptr2 = 0;

   is_ProgramVersion = ptr1 + 1;

   ptr1 = strchr(line, ' ');
   *ptr1 = 0;
   is_ProgramName = line;
}

void  DMDivisorProgram::AddDMDivisorToList(char* divisor)
{
   dmdivisor_t* dmdPtr, * listPtr;

   dmdPtr = (dmdivisor_t*)malloc(sizeof(dmdivisor_t));
   dmdPtr->m_NextDMDivisor = 0;
   strcpy(dmdPtr->s_Divisor, divisor);

   if (!ip_FirstDMDivisor)
   {
      ip_FirstDMDivisor = dmdPtr;
      return;
   }

   listPtr = ip_FirstDMDivisor;

   while (listPtr->m_NextDMDivisor)
      listPtr = (dmdivisor_t*)listPtr->m_NextDMDivisor;

   listPtr->m_NextDMDivisor = dmdPtr;
}