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
      uint32_t gfnN, factorsPerSecond, secondsPerFactor, minutesForRate;

      while (fgets(line, 2000, fp) != NULL)
      {
         StripCRLF(line);
         if (sscanf(line, "%u:%u:%u:%u", &gfnN, &factorsPerSecond, &secondsPerFactor, &minutesForRate) == 4)
         {
            if ((factorsPerSecond == 0 && secondsPerFactor == 0) ||
               (factorsPerSecond > 0 && secondsPerFactor > 0))
            {
               ip_Log->LogMessage("%s: gfn_rates.txt line %s is invalid", is_Suffix.c_str(), line);
#ifdef WIN32
               SetQuitting(1);
#else
               raise(SIGINT);
#endif
               return TR_CANCELLED;
            }

            if (gfnN == n)
            {
               if (factorsPerSecond > 0)
                  snprintf(rate, sizeof(rate), "-4%u", factorsPerSecond);

               if (secondsPerFactor > 0)
                  snprintf(rate, sizeof(rate),  "-5%u -6%u", secondsPerFactor, minutesForRate);
            }
         }
      }

      fclose(fp);
   }

   snprintf(command, 200, "%s %s -x -n%d -k%" PRIu64" -K%" PRIu64" > dmd.log", is_ExeName.c_str(),
      rate, n, minK, maxK);

   system(command);

   testResult = ParseTestResults();

   return testResult;
}

// Parse the output from WWWW and determine if the candidate is PRP
testresult_t   DMDivisorProgram::ParseTestResults()
{
   char        line[250], fileName[30];
   bool        isCompleted;
   int32_t     tryCount, n;
   int64_t     k;
   FILE* fp;

   id_Seconds = 0;

   strcpy(fileName, "dmd.log");

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

   isCompleted = false;
   while (fgets(line, sizeof(line), fp))
   {
      if (strstr(line, "slower than target"))
         isCompleted = true;

      if (strstr(line, "Primes tested: ") == nullptr)
         continue;

      char* ptr = strstr(line, "Time: ");
      if (ptr)
         sscanf(ptr, "Time: %lf", &id_Seconds);
   }
   fclose(fp);

   if (!isCompleted)
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

   unlink("dmd.log");

   ip_FirstDMDivisor = nullptr;

   if ((fp = fopen("dm_factors.txt", "r")) != NULL)
   {
      while (fgets(line, sizeof(line), fp))
      {
         if (sscanf(line, "Found factor 2*%" PRId64"*(2^%d-1)+1", &k, &n) != 2)
         {
            ip_Log->LogMessage("%s: Could not find divisior in file.  Assuming user stopped with ^C",
               is_Suffix.c_str());
#ifdef WIN32
            SetQuitting(1);
#else
            raise(SIGINT);
#endif
         }

         dmdivisor_t *dmdPtr = (dmdivisor_t *) malloc(sizeof(dmdivisor_t));
         dmdPtr->m_NextDMDivisor = 0;

         dmdPtr->n = n;
         dmdPtr->k = k;
         strcpy(dmdPtr->s_Divisor, line);

         if (!ip_FirstDMDivisor)
            ip_FirstDMDivisor = dmdPtr;
         else
         {
            dmdivisor_t *listPtr = ip_FirstDMDivisor;

            while (listPtr->m_NextDMDivisor)
               listPtr = (dmdivisor_t *) listPtr->m_NextDMDivisor;

            listPtr->m_NextDMDivisor = dmdPtr;
         }
      }

      fclose(fp);
      unlink("dm_factors.txt");
   }

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
   ptr1 = strchr(line, 'v');
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
