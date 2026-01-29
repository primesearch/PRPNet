#include <stdlib.h>
#include "GFNDivisorProgram.h"

void     GFNDivisorProgram::SendStandardizedName(Socket* theSocket, uint32_t returnWorkUnit)
{
   theSocket->Send(GetStandardizedName().c_str());
}

testresult_t   GFNDivisorProgram::Execute(int32_t serverType, uint32_t n, uint64_t minK, uint64_t maxK)
{
   char           command[200], line[200], rate[200];
   testresult_t   testResult;
   FILE          *fp;

   ii_ServerType = serverType;
   rate[0] = 0;

   if ((fp = fopen("gfn_rates.txt", "r")) != NULL)
   {
      uint32_t gfnN, factorsPerSecond, secondsPerFactor, minutesForRate;

      while (fgets(line, 2000, fp) != NULL)
      {
         if (memcmp(line, "rate=", 5))
            continue;

         StripCRLF(line);
         if (sscanf(line, "rate=%u:%u:%u:%u", &gfnN, &factorsPerSecond, &secondsPerFactor, &minutesForRate) == 4)
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

            if (gfnN >= n)
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

   if (DoesFileExist("gfndsieve.log"))
      unlink("gfndsieve.log");

   if (DoesFileExist("gfnd.abcd"))
      snprintf(command, 200, "%s %s -igfnd.abcd", is_ExeName.c_str(), rate);
   else
      snprintf(command, 200, "%s %s -n%u -N%u -k%" PRIu64" -K%" PRIu64"", is_ExeName.c_str(),
         rate, n, n, minK, maxK);

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   testResult = ParseTestResults();

   return testResult;
}

// Parse the output from WWWW and determine if the candidate is PRP
testresult_t   GFNDivisorProgram::ParseTestResults()
{
   char        line[250], fileName[30];
   bool        isCompleted;
   int32_t     tryCount;
   FILE* fp;

   strcpy(fileName, "gfndsieve.log");

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
      if (strstr(line, "Stopped due to removal rate"))
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

   unlink("gfndsieve.log");

   return TR_COMPLETED;
}

void  GFNDivisorProgram::DetermineVersion(void)
{
   char  command[200], line[200], * ptr1, * ptr2 = 0;
   FILE* fp;

   snprintf(command, 200, "%s -h > gfnd.out", is_ExeName.c_str());

   system(command);

   fp = fopen("gfnd.out", "r");
   if (!fp)
   {
      printf("Could not determine version of gfndsieve being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("gfnd.out");

      printf("Could not determine version of gfndsieve being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("gfnd.out");
   unlink("gfndsieve.log");

   // The first line should look like this:
   //   gfndsieve v1.0, a program to find factors of k*2^n+1 numbers for variable k and n
   ptr1 = strstr(line, " v");
   if (ptr1)
      ptr2 = strchr(ptr1, ',');

   if (!ptr1 || !ptr2)
   {
      printf("Could not determine version of gfndsieve being used.  Missing version data\n");
      exit(0);
   }
   *ptr2 = 0;

   is_ProgramVersion = ptr1 + 1;

   ptr1 = strchr(line, ' ');
   *ptr1 = 0;
   is_ProgramName = line;
}
