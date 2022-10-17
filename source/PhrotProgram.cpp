#include "PhrotProgram.h"

void     PhrotProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   theSocket->Send("phrot");
}

testresult_t   PhrotProgram::Execute(testtype_t testType)
{
   char           command[200];
   testresult_t   testResult;
   FILE          *fp;
   int            tryCount;

   // We do not remove the phrot.ckpt file as it can be used
   // to continue a previous test that was not completed
   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());
   unlink("results.out");
   unlink("phrot.ini");

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

   if (ii_ServerType == ST_GFN)
   {
      fprintf(fp, "ABC $a^$b%+d\n", ii_c);
      fprintf(fp, "%d %d\n", ii_b, ii_n);
   }
   else
   {
      fprintf(fp, "ABC $a*$b^$c%+d\n", ii_c);
      fprintf(fp, "%" PRId64" %d %d\n", il_k, ii_b, ii_n);
   }

   fclose(fp);

   // -t will use "terse" output
   // -o will write the results to results.out
   sprintf(command, "%s -t -o %s", is_ExeName.c_str(), is_InFileName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   testResult = ParseTestResults(testType);

   // We do not remove the phrot.ckpt file as it can be used
   // to continue a previous test that was not completed
   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());
   unlink("results.out");
   unlink("phrot.ini");

   return testResult;
}

// Parse the output from phrot and determine if the candidate is PRP
testresult_t   PhrotProgram::ParseTestResults(testtype_t testType)
{
   char        line[250], fileName[30], *ptr, *endOfResidue;
   FILE       *fp;
   int         tryCount = 0;

   id_Seconds = 0;

   strcpy(fileName, "results.out");

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

   if (!fgets(line, sizeof(line), fp))
   {
      ip_Log->LogMessage("%s: No data in file [%s].  Is phrot broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   fclose(fp);

   // The time is typically in the format "(t=0.08s)".
   ptr = strstr(line, "s)");
   if (ptr)
   {
      *ptr = 0;
      while (*ptr != '=' && ptr > line)
         ptr--;

      if (ptr > line)
         sscanf(ptr, "=%lf", &id_Seconds);
   }

   if (strstr(line, "is prime"))
   {
      ib_IsPrime = true;
      return TR_COMPLETED;
   }

   if (strstr(line, "is PRP"))
   {
      ib_IsPRP = true;
      return TR_COMPLETED;
   }

   ptr = strstr(line, "LLR64=");
   if (!ptr)
   {
      ip_Log->LogMessage("%s: Could not find LLR64 residue [%s].  Is phrot broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   ptr += 6;
   endOfResidue = strchr(ptr, '.');

   if (!endOfResidue)
   {
      ip_Log->LogMessage("%s: Could not find terminator for the residue in [%s].  Is phrot broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   *endOfResidue = 0;

   is_Residue = ptr;

   return TR_COMPLETED;
}

void  PhrotProgram::DetermineVersion(void)
{
   char  command[200], line[200], *ptr1, *ptr2;
   FILE *fp;

   sprintf(command, "%s 2> a.out", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of phrot being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("a.out");

      printf("Could not determine version of phrot being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("a.out");

   // The first line should look like this:
   // Phil Carmody's Phrot (0.72)
   ptr1 = strstr(line, "(");
   ptr2 = strstr(line, ")");
   if (!ptr1 || !ptr2)
   {
      printf("Could not determine version of phrot being used.  Missing version data\n");
      exit(0);
   }
   *ptr2 = 0;

   is_ProgramVersion = ptr1 + 1;
}
