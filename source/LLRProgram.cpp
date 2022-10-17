#include "LLRProgram.h"

void     LLRProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   theSocket->Send(GetStandardizedName());
}

testresult_t   LLRProgram::Execute(testtype_t testType)
{
   char           command[100];
   testresult_t   testResult;
   FILE          *fp;
   int            tryCount;

   id_Seconds = 0;

   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());
   unlink("lresults.txt");
   unlink("llr.ini");

   tryCount = 1;
   while ((fp = fopen(is_InFileName.c_str(), "wt")) == NULL)
   {
      // Try up to five times to open the file before bailing
      if (tryCount < 5)
      {
         Sleep(500);
         tryCount++;
      }
      else
      {
         ip_Log->LogMessage("%s: Could not open file [%s] for writing.  Exiting program",
                            is_Suffix.c_str(), is_InFileName.c_str());
         exit(-1);
      }
   }
   
   if (ii_ServerType == ST_CAROLKYNEA)
   {
      fprintf(fp, "ABC (%d^$a$b)^2-2\n", ii_b);
      fprintf(fp, "%d %+d\n", ii_n, ii_c); 
   }
   else if (ii_ServerType == ST_WAGSTAFF)
   {
      fprintf(fp, "ABC (2^$a+1)/3\n");
      fprintf(fp, "%d\n", ii_n); 
   }
   else
   {
      fprintf(fp, "ABC $a*$b^$c%+d\n", ii_c);
      fprintf(fp, "%" PRId64" %d %d\n", il_k, ii_b, ii_n); 
   }

   
   fclose(fp);

   tryCount = 1;
   while ((fp = fopen("llr.ini", "wt")) == NULL)
   {
      // Try up to five times to open the file before bailing
      if (tryCount < 5)
      {
         Sleep(500);
         tryCount++;
      }
      else
      {
         ip_Log->LogMessage("%s: Could not open file llr.ini for writing.  Exiting program", is_Suffix.c_str());
         exit(-1);
      }
   }

   fprintf(fp, "RunOnBattery=0\n");
   fprintf(fp, "Work=0\n");
   fprintf(fp, "PgenInputFile=%s\n", is_InFileName.c_str());
   fprintf(fp, "PgenOutputFile=%s\n", is_OutFileName.c_str());
   fprintf(fp, "PgenLine=1\n");
   fprintf(fp, "WorkDone=0\n");
   if (ii_Affinity >= 0)
      fprintf(fp, "Affinity=%u\n", ii_Affinity);
   fclose(fp);

   sprintf(command, "%s -d", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   testResult = ParseTestResults(testType);

   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());
   unlink("lresults.txt");
   unlink("llr.ini");

   return testResult;
}

// Parse the output from LLR and determine if the candidate is PRP or prime
testresult_t   LLRProgram::ParseTestResults(testtype_t testType)
{
   char        line[250], fileName[30], scale[200], *ptr, *endOfResidue;
   int32_t     part1, part2;
   FILE       *fp;
   int         tryCount = 0;

   id_Seconds = 0;

   strcpy(fileName, "lresults.txt");

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

   *line = 0;
   while (!strstr(line, "probable prime") && !strstr(line, "is prime") && !strstr(line, "RES64") &&
          !strstr(line, "Res64") && !strstr(line, "Fermat PRP") &&
          !strstr(line, "composite") && !strstr(line, "small factor"))
     if (!fgets(line, sizeof(line), fp))
     {
        ip_Log->LogMessage("%s: Test results not found in file [%s].  Assuming user stopped with ^C",
                           is_Suffix.c_str(), fileName);
#ifdef WIN32
        SetQuitting(1);
#else
        raise(SIGINT);
#endif
        fclose(fp);
        return TR_CANCELLED;
     }

   fclose(fp);

   // The time is typically in the format "Time : 144.499 ms".
   ptr = strstr(line, "Time : ");
   if (ptr)
   {
      if (sscanf(ptr, "Time : %d.%d %s", &part1, &part2, scale) == 3)
      {
         if (!memcmp(scale, "sec", 3))
            id_Seconds = (double) part1 + ((double) part2 * .001);
         else if (!memcmp(scale, "ms", 2))
            id_Seconds = (double) part1 * .001;
      }
   }

   if (strstr(line, "is prime"))
   {
      ib_IsPrime = true;
      return TR_COMPLETED;
   }

   if (strstr(line, "is a probable prime") || strstr(line, "Fermat PRP"))
   {
      ib_IsPRP = true;
      return TR_COMPLETED;
   }

   ptr = strstr(line, "RES64:");
   if (!ptr) ptr = strstr(line, "Res64:");

   if (!ptr)
      if (strstr(line, "small factor"))
      {
         is_Residue = "small_factor";
         return TR_COMPLETED;
      }

   if (!ptr)
   {
      ip_Log->LogMessage("%s: Could not find RES64 residue [%s].  Is llr broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   ptr += 7;
   endOfResidue = strchr(ptr, ' ');
   if (!endOfResidue) endOfResidue = strchr(ptr, '.');

   if (!endOfResidue)
   {
      ip_Log->LogMessage("%s: Could not find terminator for the residue in [%s].  Is LLR broken?",
                         is_Suffix.c_str(), fileName);
      exit(0);
   }

   *endOfResidue = 0;
   endOfResidue--;
   if (*endOfResidue == '.') 
   {
      *endOfResidue = 0;
      endOfResidue--;
   }

   is_Residue = ptr;

   return TR_COMPLETED;
}

void  LLRProgram::DetermineVersion(void)
{
   char  command[200], line[200], *ptr, *ptr2;
   FILE *fp;

   sprintf(command, "%s -v > a.out", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of LLR being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("a.out");

      printf("Could not determine version of LLR being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("a.out");

   // The first line should look like this:
   // Primality Testing of k*b^n+/-1 Program - Version 3.8.0
   ptr = strstr(line, "Version");
   if (!ptr)
   {
      printf("Could not determine version of LLR being used.  Missing version data\n");
      exit(0);
   }

   ptr2 = strchr(ptr+8, ',');
   if (ptr2) *ptr2 = 0;

   StripCRLF(ptr + 8);
   is_ProgramVersion = ptr + 8;
}
