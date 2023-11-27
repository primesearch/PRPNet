#include "PRSTProgram.h"

void     PRSTProgram::SendStandardizedName(Socket* theSocket, uint32_t returnWorkUnit)
{
   theSocket->Send(GetStandardizedName().c_str());
}

testresult_t   PRSTProgram::Execute(testtype_t testType)
{
   char           command[100];
   testresult_t   testResult;

   id_Seconds = 0;

   unlink("result.txt");

   if (ii_ServerType == ST_PRIMORIAL || ii_ServerType == ST_FACTORIAL || (testType == TT_PRP && !IsPerformingProthTest()))
      snprintf(command, 100, "%s %s -d -fermat %s", is_ExeName.c_str(), is_ExeArguments.c_str(), is_WorkUnitName.c_str());
   else
      snprintf(command, 100, "%s %s -d \"%s\"", is_ExeName.c_str(), is_ExeArguments.c_str(), is_WorkUnitName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   testResult = ParseTestResults(testType);

   unlink("result.txt");

   return testResult;
}

// Parse the output from LLR and determine if the candidate is PRP or prime
testresult_t   PRSTProgram::ParseTestResults(testtype_t testType)
{
   char        line[250], fileName[30], *ptr, *endOfResidue;
   FILE* fp;
   int         tryCount = 0;

   id_Seconds = 0;

   strcpy(fileName, "result.txt");

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
   while (!strstr(line, "is a probable prime") && !strstr(line, "is prime") 
      && !strstr(line, "RES64") && !strstr(line, "is not prime"))
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
   ptr = strstr(line, "ime : ");
   if (ptr)
   {
      sscanf(ptr, "ime : %lf s", &id_Seconds);
   }

   if (strstr(line, "is prime"))
   {
      ib_IsPrime = true;
      return TR_COMPLETED;
   }

   if (strstr(line, "is a probable prime"))
   {
      ib_IsPRP = true;
      return TR_COMPLETED;
   }

   ptr = strstr(line, "RES64:");

   if (!ptr)
      if (strstr(line, "divisible"))
      {
         is_Residue = "small_factor";
         return TR_COMPLETED;
      }

   if (!ptr)
   {
      ip_Log->LogMessage("%s: Could not find RES64 residue [%s].  Is prst broken?", is_Suffix.c_str(), fileName);
      exit(0);
   }

   ptr += 7;
   endOfResidue = strchr(ptr, ',');

   if (!endOfResidue)
   {
      ip_Log->LogMessage("%s: Could not find terminator for the residue in [%s].  Is prst broken?",
         is_Suffix.c_str(), fileName);
      exit(0);
   }

   *endOfResidue = 0;

   is_Residue = ptr;

   return TR_COMPLETED;
}

void  PRSTProgram::DetermineVersion(void)
{
   char  command[200], line[200], * ptr, * ptr2;
   FILE* fp;

   is_ProgramVersion = "";

   snprintf(command, 200, "%s -v > a.out", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of PRST being used.  File not found.\n");
      exit(0);
   }

   while (fgets(line, sizeof(line), fp) != 0)
   {
      // We should find a line like this:
      // LLR Program - Version 4.0.3, using Gwnum Library Version 30.6
      ptr = strstr(line, "version");
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
      printf("Could not determine version of PRST being used.  Missing version data\n");
      exit(0);
   }
}

bool  PRSTProgram::IsPerformingProthTest(void)
{
   switch (ii_ServerType) {
      case ST_SIERPINSKIRIESEL:
      case ST_CULLENWOODALL:
      case ST_FIXEDBKC:
      case ST_FIXEDBNC:
      case ST_TWIN:
      case ST_TWINANDSOPHIE:
         return (ii_b == 2 && ii_c == 1);

      default:
       return false;

   }
}