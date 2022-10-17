#include "CycloProgram.h"

void     CycloProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   theSocket->Send("cyclo");
}

testresult_t   CycloProgram::Execute(testtype_t testType)
{
   char           command[200];
   testresult_t   testResult;
   FILE          *fp;
   int            tryCount;

   is_OutFileName = "Cyclo.log";

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

   // Note the cyclo only supports Phi(3,b^n) where b < 0 and n is a power of 2
   // and Phi(6,b^n) where b > 0 and n is a power of 2.  Why both?  Because
   // Phi(6,x) = x^2 - x + 1 = Phi(3,-x)
   if (ii_b > 0)
      fprintf(fp, "%d %d\n", ii_b, ii_n*2);
   else
      fprintf(fp, "%d %d\n", -ii_b, ii_n*2);

   fclose(fp);

   // -t will use "terse" output
   // -o will write the results to results.out
   sprintf(command, "%s %s", is_ExeName.c_str(), is_InFileName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   testResult = ParseTestResults(testType);

   // We do not remove the phrot.ckpt file as it can be used
   // to continue a previous test that was not completed
   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());
   unlink("Cyclo.log");

   return testResult;
}

// Parse the output from phrot and determine if the candidate is PRP
testresult_t   CycloProgram::ParseTestResults(testtype_t testType)
{
   char        line[250], fileName[30], *ptr, *endOfTime, *endOfResidue;
   FILE       *fp;
   int         tryCount = 0;

   id_Seconds = 0;

   strcpy(fileName, is_OutFileName.c_str());

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
      ip_Log->LogMessage("%s: No data in file [%s].  Is cyclo broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   fclose(fp);

   // The time is typically in the format "(0.08s)".
   ptr = strstr(line, " (");
   if (!ptr)
   {
      ip_Log->LogMessage("%s: Could not find time in [%s].  Is cyclo broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   ptr += 2;
   endOfTime = strstr(ptr, " ");
   *endOfTime = 0;
   if (sscanf(ptr, "%lf", &id_Seconds) != 1)
   {
      ip_Log->LogMessage("%s: Could not scan time in [%s].  Is cyclo broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   *endOfTime = ' ';
   if (strstr(line, "is a probable prime"))
   {
      ib_IsPRP = true;
      return TR_COMPLETED;
   }

   ptr = strstr(line, "Res=");
   if (!ptr)
   {
      ip_Log->LogMessage("%s: Could not find residue in [%s].  Is cyclo broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }
   
   ptr += 5;
   endOfResidue = strchr(ptr, ']');

   if (!endOfResidue)
   {
      ip_Log->LogMessage("%s: Could not find terminator for the residue in [%s].  Is cyclo broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   *endOfResidue = 0;

   is_Residue = ptr;

   return TR_COMPLETED;
}

void  CycloProgram::DetermineVersion(void)
{
   char command[200], line[200];
   char version[20], *ptr;
   FILE *fp;

   sprintf(command, "%s -v > a.out", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of cyclo being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("a.out");

      printf("Could not determine version of cyclo being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("a.out");

   // The first line should look like this:
   // "Cyclo (OpenCL, v0.2)"

   if (sscanf(line,"Cyclo (OpenCL, v%s)", version) != 1)
   {
      printf("Could not determine version of cyclo being used.  Missing version data\n");
      exit(0);
   }
   
   ptr = strchr(version, ')');
   if (ptr) *ptr = 0;

   is_ProgramVersion = version;
}
