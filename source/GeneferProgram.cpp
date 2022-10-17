#include <ctype.h>
#include "GeneferProgram.h"

void     GeneferProgram::AddProgram(string programName)
{
   int32_t  index;

   if (ii_ProgramCount >= MAX_GENEFERS)
      return;

   if (!programName.length())
      return;

   is_ProgramList[ii_ProgramCount] = programName;

   ii_ProgramCount++;

   for (index=0; index<MAX_GENEFERS; index++)
      ii_RunIndex[index] = 99;
}

void     GeneferProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   int32_t  index;

   if (returnWorkUnit)
   {
      for (index=0; index<ii_ProgramCount; index++)
         theSocket->Send("Genefer ROE: %s", is_StandardizedName[index].c_str());
   }
   else
   {
      for (index=0; index<ii_ProgramCount; index++)
         theSocket->Send(is_StandardizedName[index]);
   }
}

testresult_t   GeneferProgram::Execute(testtype_t testType)
{
   char           command[200];
   testresult_t   testResult;
   int32_t        index;
   FILE          *fp;
   int            tryCount;

   // We do not remove the genefer.ckpt file as it can be used
   // to continue a previous test that was not completed
   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());

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

   fprintf(fp, "ABC $a^$b+1\n");
   fprintf(fp, "%u %u\n", ii_b, ii_n);

   fclose(fp);

   testResult = TR_CANCELLED;

   for (index=0; index<MAX_GENEFERS; index++)
   {
      unlink("genefer.log");
      unlink("genefer.ini");

      ib_TestFailure = true;

      if (ii_RunIndex[index] == 99) continue;

      is_InternalProgramName = is_ProgramList[ii_RunIndex[index]];

      if (ii_Affinity >= 0 && index == 0)
         sprintf(command, "%s -d %d %s", is_InternalProgramName.c_str(), ii_Affinity, is_InFileName.c_str());
      else
         sprintf(command, "%s %s", is_InternalProgramName.c_str(), is_InFileName.c_str());

      ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

      system(command);

      testResult = ParseTestResults(testType);

      // Exit the loop if the user cancelled
      if (testResult == TR_CANCELLED)
         break;

      if (ib_DeleteCheckpoint)
         unlink("genefer.ckpt");

      // Exit the loop upon successful completion of a test.
      if (!ib_TestFailure)
         break;
   }

   // We do not remove the genefer.ckpt file as it can be used
   // to continue a previous test that was not completed
   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());
   unlink("genefer.log");
   unlink("genefer.ini");

   return testResult;
}

// Parse the output from phrot and determine if the candidate is PRP
testresult_t   GeneferProgram::ParseTestResults(testtype_t testType)
{
   char        line[250], line2[250], fileName[30], *ptr, *endOfResidue;
   int32_t     hours, minutes, seconds;
   FILE       *fp;
   int         tryCount;
  
   ib_DeleteCheckpoint = true;
   id_Seconds = 0;

   strcpy(fileName, "genefer.log");

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

   ptr = 0;

   while (!ptr)
   {
      if (!fgets(line, sizeof(line), fp))
      {
         ip_Log->LogMessage("%s: No data in file [%s].  Is genefer broken?", is_Suffix.c_str(), fileName);
         exit(0);
      }

      if (strstr(line, "Start test"))
         continue;

      *line2 = 0;
      fgets(line2, sizeof(line2), fp);

      fclose(fp);

      if (strstr(line, "version cannot restart"))
      {
         is_Residue = "roundofferr";
         ib_TestFailure = true;
         ib_DeleteCheckpoint = false;
         return TR_COMPLETED;
      }

      if (strstr(line, "maxErr exceeded"))
      {
         is_Residue = "roundofferr";
         ib_TestFailure = true;
         return TR_COMPLETED;
      }

      if (strstr(line, "is a probable prime"))
      {
         ib_IsPRP = true;
         ib_TestFailure = false;
         return TR_COMPLETED;
      }

      ptr = strstr(line, "RES=");
      if (!ptr)
      {
         ip_Log->LogMessage("%s: Could not find RES residue [%s].  Is genefer broken?", is_Suffix.c_str(), fileName);
         exit(0);
      }
   }

   ptr += 4;
   endOfResidue = strchr(ptr, ')');

   if (!endOfResidue)
   {
      ip_Log->LogMessage("%s: Could not find terminator for the residue in [%s].  Is genefer broken?",
                         is_Suffix.c_str(), fileName);
      exit(0);
   }

   *endOfResidue = 0;

   ib_TestFailure = false;
   is_Residue = ptr;

   // The time is typically in the format "(time = hh:mm:ss)".
   ptr = strstr(endOfResidue+1, "time =");
   if (!ptr) ptr = strstr(line2, "time =");

   if (ptr)
   {
      if (sscanf(ptr, "time = %d:%d:%d", &hours, &minutes, &seconds) == 3)
         id_Seconds = hours * 3600 + minutes * 60 + seconds;
   }

   return TR_COMPLETED;
}

uint32_t GeneferProgram::ValidateExe(void)
{
   struct stat buf;
   int32_t  index;
   uint32_t lower;
   char     command[150], *pos;
   FILE    *fPtr;

   for (index=0; index<ii_ProgramCount; index++)
   {
      if (stat(is_ProgramList[index].c_str(), &buf) == -1)
      {
         printf("Could not find executable '%s'.\n", is_ProgramList[index].c_str());
         return false;
      }

      sprintf(command, "%s -v > a.out", is_ProgramList[index].c_str());

      ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

      system(command);

      fPtr = fopen("a.out", "r");
      if (!fPtr)
      {
         printf("Unable to determine version from '%s'\n", is_ProgramList[index].c_str());
         return false;
      }

      if (!fgets(command, sizeof(command), fPtr))
         command[0] = 0;

      fclose(fPtr);
      unlink("a.out");

      for (lower=0; lower<strlen(command); lower++)
         command[lower] = (char) tolower(command[lower]);

      pos = strchr(command, ' ');
      if (pos == 0 || pos - command > 20)
      {
         printf("Unable to determine version from '%s'\n", is_ProgramList[index].c_str());
         return false;
      }

      *pos = 0;
      if (!strcmp(command, GENEFER))
         ii_RunIndex[2] = index;
      else if (!strcmp(command, GENEFER_x64))
         ii_RunIndex[1] = index;
      else if (!strcmp(command, GENEFER_cuda) || !strcmp(command, GENEFER_OpenCL))
         ii_RunIndex[0] = index;
      else if (!strcmp(command, GENEFER_80bit))
         ii_RunIndex[3] = index;
      else
      {
         printf("Genefer version of %s from '%s' is not supported\n", command, is_ProgramList[index].c_str());
         return false;
      }

      is_StandardizedName[index] = command;

      // Here we can reject known buggy versions from the client-side
      DetermineVersion();
      if (is_ProgramVersion.find("3.2.0beta-0") != string::npos)
      {
         printf("Genefer Version '%s' is not supported\n", is_ProgramVersion.c_str());
         return false;
      }
   }

   return true;
}

void  GeneferProgram::DetermineVersion(void)
{
   char command[200], line[200];
   char version[20];
   FILE *fp;

   sprintf(command, "%s -v > a.out", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of Genefer being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("a.out");

      printf("Could not determine version of Genefer being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("a.out");

   // The first line should look like this:
   // "<appname> <version> <arch>" e.g.
   // "genefer 3.1.2-0 (Apple x86 32-bit Default)"

   if (sscanf(line,"%*s %20s", version) != 1)
   {
      printf("Could not determine version of Genefer being used.  Missing version data\n");
      exit(0);
   }
   is_ProgramVersion = version;
}
