#include "PFGWProgram.h"

void     PFGWProgram::SendStandardizedName(Socket *theSocket, uint32_t returnWorkUnit)
{
   ip_FirstGFN = NULL;

   theSocket->Send(GetStandardizedName().c_str());
}

testresult_t   PFGWProgram::Execute(testtype_t testType)
{
   char           command[200], sign, normalPriority[20], fftSize[20];
   char           affinity[20], newBase[20], primalityValue[20];
   char           pfgwArguments[200];
   testresult_t   testResult;
   FILE          *fp;
   int32_t        tryCount, a;

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

   if (ii_ServerType == ST_CYCLOTOMIC)
      sign = 'm';
   else if (ii_ServerType == ST_CAROLKYNEA)
      sign = 'p';
   else
      sign = ((il_c > 0) ? 'm' : 'p');

   primalityValue[0] = 0;
   newBase[0] = 0;
   affinity[0] = 0;
   normalPriority[0] = 0;

   if (testType == TT_PRIMALITY)
      snprintf(primalityValue, sizeof(primalityValue), "-e%d -t%c", ii_b, sign);

   if (is_CpuAffinity.length() > 0)
      snprintf(affinity, sizeof(affinity), "-A%s", is_CpuAffinity.c_str());

   // If k = 1 and b mod 3 = 0, then choose a base 5 for the PRP test as the default is base 3
   // and the PRP test is likely to yield bad results.
   if (il_k == 1 && ii_b % 3 == 0)
      if (ii_ServerType == ST_SIERPINSKIRIESEL || ii_ServerType == ST_FIXEDBKC || ii_ServerType == ST_FIXEDBNC)
         strcat(newBase, "-b5");

   if (ii_NormalPriority)
      strcat(normalPriority, "-N");

   // Start with a larger FFT size for GFNs.
   a = (testType == TT_GFN ? 2 : 0);

   do
   {
      fftSize[0] = 0;

      if (a > 0)
         snprintf(fftSize, sizeof(fftSize), "-a%u", a);

      // -k   terse output
      // -f0  prevents trail factoring
      // -a<> changes the FFT size which helps avoid FFT failures
      // -b<> changes the base
      // -A<> sets CPU affinity
      // -N   normal priority (by default, pfgw runs at low priority)
      // -e<> increases likelihood that PFGW will get to 33% factorization for primarily tests.  This
      //      is primarity an issue with GFNs and not other forms as GFNs have large bases.
      // -t<> ensures the correct type of primality test is run
      snprintf(pfgwArguments, sizeof(pfgwArguments), "-k -f0 %s %s %s %s %s", fftSize, newBase, affinity, normalPriority, primalityValue);

      ib_TestFailure = false;

      if (a > 2)
      {
         ip_Log->LogMessage("%s: PFGW failed on WorkUnit %s for multiple FFT sizes.  Exiting program",
                            is_Suffix.c_str(), is_WorkUnitName.c_str());
         exit(-1);
      }

      unlink(is_OutFileName.c_str());

      if (ii_ServerType == ST_GENERIC)
         DetermineDecimalLength();

      snprintf(command, 200, "%s %s %s -l%s %s", is_ExeName.c_str(), is_ExeArguments.c_str(), pfgwArguments, is_OutFileName.c_str(), is_InFileName.c_str());

      ip_Log->Debug(DEBUG_WORK, "Command line: %s", command);

      system(command);

      testResult = ParseTestResults(testType);

      // Increase FFT size and try again.
      a++;
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

   snprintf(command, 200, "%s -V 2> a.out", is_ExeName.c_str());

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

   snprintf(command, 200, "%s -k -od -f0 -l%s %s",
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
   while ((bytes = (int) fread(line, 1, (int32_t)  sizeof(line), fp)) > 0)
      ii_DecimalLength += bytes;
   
   fclose(fp);

   // Exclude the candidate name + the ": " after it in the output
   if (ii_DecimalLength > 0)
      ii_DecimalLength -= (int32_t) (is_WorkUnitName.length() + 2);

}
