#include "WWWWProgram.h"

void     WWWWProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   ip_FirstWWWW = NULL;

   theSocket->Send(GetStandardizedName());
}

testresult_t   WWWWProgram::Execute(int32_t serverType, int32_t specialThreshhold, int64_t lowerLimit, int64_t upperLimit)
{
   const char    *searchType = "";
   char           command[200];
   testresult_t   testResult;

   ip_FirstWWWW = NULL;

   ii_ServerType = serverType;

   if (serverType == ST_WIEFERICH)    searchType = "Wieferich";
   if (serverType == ST_WILSON)       searchType = "Wilson";
   if (serverType == ST_WALLSUNSUN)   searchType = "WallSunSun";
   if (serverType == ST_WOLSTENHOLME) searchType = "Wolstenholme";

   sprintf(command, "%s -%c%s -s%d -p%" PRId64" -P%" PRId64"", is_ExeName.c_str(),
      (is_ProgramName == "wwww" ? 't' : 'T'), 
      searchType, specialThreshhold, lowerLimit, upperLimit);

   system(command);

   testResult = ParseTestResults(TT_WWWW);

   return testResult;
}

// Parse the output from WWWW and determine if the candidate is PRP
testresult_t   WWWWProgram::ParseTestResults(testtype_t testType)
{
   char        line[250], fileName[30];
   char       *ptr1, *ptr2;
   bool        isCompleted;
   int32_t     tryCount;
   FILE       *fp;

   id_Seconds = 0;

   strcpy(fileName, "wwww.log");

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
      if (strstr(line, "Primes tested"))
         isCompleted = true;
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

   fp = fopen(fileName, "r");
   while (fgets(line, sizeof(line), fp))
   {
      if (strstr(line, " is a ") && strstr(line, " prime"))
         AddWWWWToList(line);
      else if (strstr(line, " is a special instance "))
         AddWWWWToList(line);
      else if (strstr(line, "Primes tested"))
      {
         il_PrimesTested = 0;
         id_Seconds = 0.0;
         ptr1 = ptr2 = 0;

         sscanf(line, "Primes tested %" PRId64"", &il_PrimesTested);

         ptr1 = strstr(line, "Checksum");
         if (ptr1) ptr2 = strstr(ptr1, ".");
         if (ptr2)
         {
            *ptr2 = 0;
            strcpy(ic_Checksum, ptr1+9);
            ptr2 = strstr(ptr2+1, "Time");
            if (ptr2) sscanf(ptr2, "Time %lf", &id_Seconds);
         }

         if (il_PrimesTested == 0 || id_Seconds == 0.0)
         {
            ip_Log->LogMessage("%s: Could not parse stats line in [%s]", is_Suffix.c_str(), fileName);
            exit(-1);
         }
      }
   }
   fclose(fp);

   unlink("wwww.log");
   unlink("wwww.ckpt");

   return TR_COMPLETED;
}


void  WWWWProgram::AddWWWWToList(string line)
{
   wwww_t   *wwwwPtr, *listPtr;
   char     tempLine[BUFFER_SIZE];

   wwwwPtr = (wwww_t *) malloc(sizeof(wwww_t));
   wwwwPtr->m_NextWWWW = 0;

   strcpy(tempLine, line.c_str());
   if (strstr(tempLine, " is a ") && strstr(tempLine, " prime"))
   {
      wwwwPtr->t_WWWWType = WWWW_PRIME;
      sscanf(tempLine, "%" PRId64"", &wwwwPtr->l_Prime);
      wwwwPtr->i_Remainder = 0;
      wwwwPtr->i_Quotient = 0;
   }
   else
   {
      wwwwPtr->t_WWWWType = WWWW_SPECIAL;
      sscanf(tempLine, "%" PRId64" is a special instance (%d %d p)",
             &wwwwPtr->l_Prime, &wwwwPtr->i_Remainder, &wwwwPtr->i_Quotient);
   }

   if (!ip_FirstWWWW)
   {
      ip_FirstWWWW = wwwwPtr;
      return;
   }

   listPtr = ip_FirstWWWW;

   while (listPtr->m_NextWWWW)
      listPtr = (wwww_t *) listPtr->m_NextWWWW;

   listPtr->m_NextWWWW = wwwwPtr;
}

void  WWWWProgram::DetermineVersion(void)
{
   char  command[200], line[200], *ptr1, *ptr2 = 0;
   FILE *fp;

   sprintf(command, "%s -V > a.out", is_ExeName.c_str());

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of wwww being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("a.out");

      printf("Could not determine version of wwww being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("a.out");

   // The first line should look like this:
   //   wwww v1.0, a program to search for Wieferich and Wall-Sun-Sun primes.
   ptr1 = strchr(line, 'v');
   if (ptr1)
      ptr2 = strchr(ptr1, ',');

   if (!ptr1 || !ptr2)
   {
      printf("Could not determine version of wwww being used.  Missing version data\n");
      exit(0);
   }
   *ptr2 = 0;

   is_ProgramVersion = ptr1 + 1;

   ptr1 = strchr(line, ' ');
   *ptr1 = 0;
   is_ProgramName = line;
}
