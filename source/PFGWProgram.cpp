#include "PFGWProgram.h"

void     PFGWProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   ip_FirstGFN = NULL;

   theSocket->Send(GetStandardizedName());
}

testresult_t   PFGWProgram::Execute(testtype_t testType)
{
   char           command[100], sign, * normalPriority;
   char           affinity[20];
   testresult_t   testResult;
   int32_t        aValue;
   FILE          *fp;
   int            tryCount;

   unlink(is_InFileName.c_str());
   unlink("pfgw.ini");

   ip_FirstGFN = NULL;

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

   fprintf(fp, "%s", is_WorkUnitName.c_str());
   fclose(fp);

   aValue = (testType == TT_GFN ? 2 : 0);
   if (ii_ServerType == ST_CYCLOTOMIC)
      sign = 'm';
   else if (ii_ServerType == ST_CAROLKYNEA)
      sign = 'p';
   else
      sign = ((ii_c > 0) ? 'm' : 'p');

   if (ii_Affinity > 0)
      sprintf(affinity, "-A%u", ii_Affinity);
   else
      affinity[0] = 0;

   normalPriority = (char*)(ii_NormalPriority ? "-N" : "");

   do
   {
      ib_TestFailure = false;

      if (aValue > 2)
      {
         ip_Log->LogMessage("%s: PFGW failed on WorkUnit %s for both -a1 and -a2.  Exiting program",
                            is_Suffix.c_str(), is_WorkUnitName.c_str());
         exit(-1);
      }

      unlink(is_OutFileName.c_str());

      if (ii_ServerType == ST_GENERIC)
         DetermineDecimalLength();

      switch (testType)
      {
         case TT_PRP:
            sprintf(command, "%s %s -k -f0 %s -a%d -l%s %s",
                    is_ExeName.c_str(), affinity, normalPriority, aValue, is_OutFileName.c_str(), is_InFileName.c_str());
            break;

         case TT_PRIMALITY:
            // Use -e to increase likelihood that PFGW will get to 33% factorization, which
            // is necessary for a primality proof.  This is an issue with GFNs which tend to
            // have large bases.
            if (ii_ServerType == ST_CYCLOTOMIC)
               sprintf(command, "%s %s -k -f0 %s -a%d -t%c -l%s %s",
                       is_ExeName.c_str(), affinity, normalPriority, aValue, sign, is_OutFileName.c_str(), is_InFileName.c_str());
            else
               sprintf(command, "%s %s -k -f0 %s -a%d -e%d -t%c -l%s %s",
                       is_ExeName.c_str(), affinity, normalPriority, aValue, ii_b, sign, is_OutFileName.c_str(), is_InFileName.c_str());
            break;

         case TT_GFN:
            sprintf(command, "%s %s -k -f0 %s -a%d -gxo -l%s %s", 
                    is_ExeName.c_str(), affinity, normalPriority, aValue, is_OutFileName.c_str(), is_InFileName.c_str());
            break;

         default:
            printf("Unhandled case for test type %d\n", (int32_t) testType);
            exit(0);
      }

      ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

      system(command);

      testResult = ParseTestResults(testType);

      aValue++;
   } while (ib_TestFailure);

   unlink(is_InFileName.c_str());
   unlink(is_OutFileName.c_str());
   unlink("pfgw.ini");
   unlink("pfgw_err.log");

   return testResult;
}

testresult_t   PFGWProgram::ParseTestResults(testtype_t testType)
{
   char        line[250], fileName[30], *ptr, *pos, *endOfResidue = 0;
   FILE       *fp;
   testresult_t rCode;
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

   *line = 0;
   while (!strstr(line, "PRP") && !strstr(line, "is prime") && !strstr(line, "RES64") &&
          !strstr(line, "composite") && !strstr(line, "GFN testing completed"))
   {
      // Must have triggered an FFT rounding error
      if (strstr(line, "ERROR"))
      {
         fclose(fp);
         ib_TestFailure = true;
         return TR_COMPLETED;
      }

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
   }

   if (testType == TT_GFN)
   {
      // Close and reopen
      fclose(fp);
      fp = fopen(fileName, "r");
      *line = 0;
      rCode = TR_CANCELLED;

      do
      {
         if (strstr(line, "GFN testing completed"))
            rCode = TR_COMPLETED;

         pos = strstr(line, "is a Factor of");
         if (pos)
         {
            ptr = strchr(line, '!');
            *ptr = 0;

            ip_Log->LogMessage("%s: %s", is_Suffix.c_str(), line);

            pos[14] = 0;
            AddGFNToList(&pos[15]);
         }
      } while (fgets(line, sizeof(line), fp));

      fclose(fp);
      
      if (rCode == TR_CANCELLED)
         ip_FirstGFN = NULL;
      return rCode;
   }

   fclose(fp);

   // The time is typically in the format "(t.tttts+y.yyyys)".  We care
   // about the first portion within the parenthesis
   ptr = strstr(line, "s+");
   if (ptr)
   {
      *ptr = 0;
      while (*ptr != '(' && ptr > line)
         ptr--;

      if (ptr > line)
         sscanf(ptr, "(%lf", &id_Seconds);
   }

   if (testType == TT_PRIMALITY)
   {
      if (!strstr(line, "is prime"))
      {
         ip_Log->LogMessage("%s: Candidate failed primality test.", is_Suffix.c_str());
         return TR_COMPLETED;
      }

      ib_IsPrime = true;
      return TR_COMPLETED;
   }

   if (strstr(line, "PRP"))
   {
      ib_IsPRP = true;
      return TR_COMPLETED;
   }

   ptr = strstr(line, "RES64:");
   if (ptr)
   {
      ptr += 7;
      if (*ptr == '[')
      {
         ptr++;
         endOfResidue = strchr(ptr, ']');
      }
      else
         endOfResidue = strchr(ptr, ' ');
   }
   else
   {
      ptr = strstr(line, "composite");
      if (ptr)
        ptr = strstr(ptr, "[");

      if (ptr)
      {
        ptr++;
        endOfResidue = strstr(ptr, "]");
      }
   }

   if (!ptr || !endOfResidue)
   {
      ip_Log->LogMessage("%s: Could not find the residue in output [%s].  Is PFGW broken?",
                         is_Suffix.c_str(), fileName);
      exit(0);
   }

   *endOfResidue = 0;

   is_Residue = ptr;

   return TR_COMPLETED;
}

void  PFGWProgram::AddGFNToList(string divisor)
{
   gfn_t   *gfnPtr, *listPtr;

   gfnPtr = (gfn_t *) malloc(sizeof(gfn_t));
   gfnPtr->m_NextGFN = 0;
   gfnPtr->s_Divisor = divisor;

   if (!ip_FirstGFN)
   {
      ip_FirstGFN = gfnPtr;
      return;
   }

   listPtr = ip_FirstGFN;

   while (listPtr->m_NextGFN)
      listPtr = (gfn_t *) listPtr->m_NextGFN;

   listPtr->m_NextGFN = gfnPtr;
}

void  PFGWProgram::DetermineVersion(void)
{
   char  command[200], line[200], *ptr1, *ptr2;
   FILE *fp;

   sprintf(command, "%s -V 2> a.out", is_ExeName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

   fp = fopen("a.out", "r");
   if (!fp)
   {
      printf("Could not determine version of PFGW being used.  File not found.\n");
      exit(0);
   }

   if (fgets(line, sizeof(line), fp) == 0)
   {
      fclose(fp);
      unlink("a.out");

      printf("Could not determine version of PFGW being used.  File empty.\n");
      exit(0);
   }

   fclose(fp);
   unlink("a.out");

   // The first line should look like this:
   // PFGW Version 3.3.6.20101007.Win_Stable [GWNUM 25.14]
   ptr1 = strstr(line, "Version");
   if (!ptr1)
   {
      printf("Could not determine version of PFGW being used.  Missing version data\n");
      exit(0);
   }

   ptr2 = strstr(ptr1+8, " ");
   if (!ptr2)
   {
      printf("Could not determine version of PFGW being used.  Missing version data\n");
      exit(0);
   }

   *ptr2 = 0;

   is_ProgramVersion = ptr1+8;
}

void  PFGWProgram::DetermineDecimalLength(void)
{
   char        command[200];
   char        line[10000], fileName[30];
   FILE       *fp;
   int         tryCount = 0, bytes;

   sprintf(command, "%s -k -od -f0 -l%s %s",
         is_ExeName.c_str(), is_OutFileName.c_str(), is_InFileName.c_str());

   ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

   system(command);

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
         return;
      }
   }

   ii_DecimalLength = 0;
   while ((bytes = fread(line, 1, (int32_t)  sizeof(line), fp)) > 0)
      ii_DecimalLength += bytes;
   
   fclose(fp);

   // Exclude the candidate name + the ": " after it in the output
   if (ii_DecimalLength > 0)
      ii_DecimalLength -= (int32_t) (is_WorkUnitName.length() + 2);

}
