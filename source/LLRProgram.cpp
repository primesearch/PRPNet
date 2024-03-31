#include "LLRProgram.h"

void     LLRProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   theSocket->Send(GetStandardizedName().c_str());
}

testresult_t   LLRProgram::Execute(testtype_t testType)
{
   char           command[100];
   testresult_t   testResult;
   FILE          *fp;
   int            tryCount;

   id_Seconds = 0;

   unlink(is_OutFileName.c_str());
   unlink("lresults.txt");
   unlink("llr.ini");

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
   fprintf(fp, "PgenOutputFile=%s\n", is_OutFileName.c_str());
   fprintf(fp, "PgenLine=1\n");
   fprintf(fp, "WorkDone=0\n");
   if (is_CpuAffinity.length() > 0)
      fprintf(fp, "Affinity=%s\n", is_CpuAffinity.c_str());
   fclose(fp);

   snprintf(command, 100, "%s %s -q\"%s\" -d", is_ExeName.c_str(), is_ExeArguments.c_str(), is_WorkUnitName.c_str());

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
   char        line[250], scale[200], *ptr, *endOfResidue;
   int32_t     part1, part2;

   id_Seconds = 0;

   if (!GetLineWithTestResult(line, sizeof(line)))
      return TR_CANCELLED;

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

   if (strstr(line, "is a probable prime") || strstr(line, "Fermat PRP") || strstr(line, "Frobenius PRP"))
   {
      ib_IsPRP = true;
      return TR_COMPLETED;
   }

   ptr = strstr(line, "RES64:");
   if (!ptr) ptr = strstr(line, "Res64:");

   if (ptr == NULL)
   {
      if (strstr(line, "small factor"))
      {
         is_Residue = "small_factor";
         return TR_COMPLETED;
      }

      if (strstr(line, "is not prime. (APRCL test)"))
      {
         is_Residue = "APRCL_composite";
         return TR_COMPLETED;
      }
   }

   if (ptr == NULL)
   {
      ip_Log->LogMessage("%s: Could not find RES64 residue lresults.txt.  Is llr broken?", is_Suffix.c_str());
      exit(0);
   }

   ptr += 7;

   endOfResidue = strchr(ptr, ' ');
   if (!endOfResidue) endOfResidue = strchr(ptr, '.');

   if (endOfResidue == NULL)
   {
      ip_Log->LogMessage("%s: Could not find terminator for the residue in lresults.txt.  Is LLR broken?",
                         is_Suffix.c_str());
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

bool  LLRProgram::GetLineWithTestResult(char* line, uint32_t lineLength)
{
   int   tryCount = 0;
   char  fileName[30];
   char  nextLine[500];
   FILE* fp;

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
         return false;
      }
   }

   *line = 0;
   while (!strstr(line, "is a probable prime") && !strstr(line, "is prime") && !strstr(line, "RES64") &&
      !strstr(line, "Res64") && !strstr(line, "Fermat PRP") && !strstr(line, "is not prime") &&
      !strstr(line, "Frobenius PRP") && !strstr(line, "composite") && !strstr(line, "small factor"))
      if (!fgets(line, lineLength, fp))
      {
         ip_Log->LogMessage("%s: Test results not found in file [%s].  Assuming user stopped with ^C",
            is_Suffix.c_str(), fileName);
#ifdef WIN32
         SetQuitting(1);
#else
         raise(SIGINT);
#endif
         fclose(fp);
         return false;
      }

   // If we find "Fermat PRP", then LLR might do another PRP test, so look
   // for that line as the candidate might actually be composite.
   if (strstr(line, "Fermat PRP") == NULL)
   {
      fclose(fp);
      return line;
   }

   if (!fgets(nextLine, sizeof(nextLine), fp))
   {
      fclose(fp);
      return true;
   }

   // If there is another line in the file and it has "Starting Lucas" in it, then look for
   // the results from that test.
   if (strstr(nextLine, "Starting Lucas") == NULL)
   {
      fclose(fp);
      return true;
   }

   *line = 0;
   while (!strstr(line, "is a probable prime") && !strstr(line, "is prime") && !strstr(line, "RES64") &&
      !strstr(line, "Res64") && !strstr(line, "Fermat PRP") && !strstr(line, "is not prime") && 
      !strstr(line, "Frobenius PRP") && !strstr(line, "composite") && !strstr(line, "small factor"))
      if (!fgets(line, lineLength, fp))
      {
         ip_Log->LogMessage("%s: Test results not found in file [%s].  Assuming user stopped with ^C",
            is_Suffix.c_str(), fileName);
#ifdef WIN32
         SetQuitting(1);
#else
         raise(SIGINT);
#endif
         fclose(fp);
         return false;
      }

   fclose(fp);

   return true;
}

void  LLRProgram::DetermineVersion(void)
{
   char  command[200], line[200], *ptr, *ptr2;
   FILE *fp;

   is_ProgramVersion = "";

   snprintf(command, 200, "%s -v > a.out", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of LLR being used.  File not found.\n");
      exit(0);
   }

   while (fgets(line, sizeof(line), fp) != 0)
   {
      // We should find a line like this:
      // LLR Program - Version 4.0.3, using Gwnum Library Version 30.6
      ptr = strstr(line, "Version");
      if (ptr)
      {
         ptr += 8;

         ptr2 = strchr(ptr, ',');
         if (ptr2) *ptr2 = 0;

         StripCRLF(ptr);
         is_ProgramVersion = ptr;
         break;
      }
   }

   fclose(fp);
   unlink("a.out");

   if (is_ProgramVersion.size() == 0) {
      printf("Could not determine version of LLR being used.  Missing version data\n");
      exit(0);
   }
}
