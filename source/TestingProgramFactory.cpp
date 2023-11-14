#include "TestingProgramFactory.h"

TestingProgramFactory::TestingProgramFactory(Log *theLog, string configFile)
{
   int32_t cpuAffinity = -1, gpuAffinity = -1, normalPriority = 0;
   FILE   *fp;
   char    line[2001];

   ip_LLRProgram = NULL;
   ip_PhrotProgram = NULL;
   ip_PFGWProgram = NULL;
   ip_GeneferProgram = NULL;
   ip_CycloProgram = NULL;
   ip_WWWWProgram = NULL;
   ip_PRSTProgram = NULL;

   fp = fopen(configFile.c_str(), "r");

   // Extract the email ID and count the number of servers on this pass
   while (fgets(line, 2000, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "llrexe=", 7) && strlen(line) > 7)
         ip_LLRProgram = new LLRProgram(theLog, line+7);
      else if (!memcmp(line, "phrotexe=", 9) && strlen(line) > 9)
         ip_PhrotProgram = new PhrotProgram(theLog, line+9);
      else if (!memcmp(line, "prstexe=", 8) && strlen(line) > 8)
         ip_PRSTProgram = new PRSTProgram(theLog, line + 8);
      else if (!memcmp(line, "pfgwexe=", 8) && strlen(line) > 8)
         ip_PFGWProgram = new PFGWProgram(theLog, line+8);
      else if (!memcmp(line, "wwwwexe=", 8) && strlen(line) > 8)
         ip_WWWWProgram = new WWWWProgram(theLog, line+8);
      else if (!memcmp(line, "cycloexe=", 8) && strlen(line) > 8)
         ip_CycloProgram = new CycloProgram(theLog, line+9);
      else if (!memcmp(line, "geneferexe=", 11) && strlen(line) > 11)
      {
         if (!ip_GeneferProgram)
            ip_GeneferProgram = new GeneferProgram(theLog, line+11);
         
         ip_GeneferProgram->AddProgram(line+11);
      }
      else if (!memcmp(line, "normalpriority=", 15) && strlen(line) > 15)
         normalPriority = atoi(line+15);
      else if (!memcmp(line, "cpuaffinity=", 12) && strlen(line) > 12)
         cpuAffinity = atol(line+12);
      else if (!memcmp(line, "gpuaffinity=", 12) && strlen(line) > 12)
         gpuAffinity = atol(line+12);
   }

   if (ip_LLRProgram)      ip_LLRProgram->SetAffinity(cpuAffinity);
   if (ip_GeneferProgram)  ip_GeneferProgram->SetAffinity(gpuAffinity);
   if (ip_PFGWProgram)     ip_PFGWProgram->SetAffinity(cpuAffinity);
   if (ip_PFGWProgram)     ip_PFGWProgram->SetNormalPriority(normalPriority);
   if (ip_PRSTProgram)     ip_PRSTProgram->SetAffinity(cpuAffinity);
}

TestingProgramFactory::~TestingProgramFactory()
{
   if (ip_LLRProgram)     delete ip_LLRProgram;
   if (ip_PhrotProgram)   delete ip_PhrotProgram;
   if (ip_PFGWProgram)    delete ip_PFGWProgram;
   if (ip_PRSTProgram)    delete ip_PRSTProgram;
   if (ip_GeneferProgram) delete ip_GeneferProgram;
   if (ip_CycloProgram)   delete ip_CycloProgram;
   if (ip_WWWWProgram)    delete ip_WWWWProgram;
}

bool     TestingProgramFactory::ValidatePrograms(void)
{
   if (ip_LLRProgram && !ip_LLRProgram->ValidateExe())
   {
      delete ip_LLRProgram;
      ip_LLRProgram = 0;
   }

   if (ip_PRSTProgram && !ip_PRSTProgram->ValidateExe())
   {
      delete ip_PRSTProgram;
      ip_PRSTProgram = 0;
   }

   if (ip_PhrotProgram && !ip_PhrotProgram->ValidateExe())
   {
      delete ip_PhrotProgram;
      ip_PhrotProgram = 0;
   }

   if (ip_PFGWProgram && !ip_PFGWProgram->ValidateExe())
   {
      delete ip_PFGWProgram;
      ip_PFGWProgram = 0;
   }

   if (ip_GeneferProgram && !ip_GeneferProgram->ValidateExe())
   {
      delete ip_GeneferProgram;
      ip_GeneferProgram = 0;
   }
   
   if (ip_CycloProgram && !ip_CycloProgram->ValidateExe())
   {
      delete ip_CycloProgram;
      ip_CycloProgram = 0;
   }

   if (ip_WWWWProgram && !ip_WWWWProgram->ValidateExe())
   {
      delete ip_WWWWProgram;
      ip_WWWWProgram = 0;
   }

   if (!ip_LLRProgram && !ip_PhrotProgram && !ip_PFGWProgram && !ip_GeneferProgram && !ip_PRSTProgram && !ip_CycloProgram && !ip_WWWWProgram)
   {
      printf("At least one valid exe (llrexe, phrotexe, pfgwexe, prstexe, geneferexe, cycloexe, wwwwexe) must be specified.\n");
      return false;
   }

   return true;
}

void     TestingProgramFactory::SendPrograms(Socket *theSocket)
{
   if (ip_LLRProgram)      ip_LLRProgram->SendStandardizedName(theSocket, false);
   if (ip_PRSTProgram)     ip_PRSTProgram->SendStandardizedName(theSocket, false);
   if (ip_LLRProgram)      ip_LLRProgram->SendStandardizedName(theSocket, false);
   if (ip_PFGWProgram)     ip_PFGWProgram->SendStandardizedName(theSocket, false);
   if (ip_GeneferProgram)  ip_GeneferProgram->SendStandardizedName(theSocket, false);
   if (ip_CycloProgram)    ip_CycloProgram->SendStandardizedName(theSocket, false);
   if (ip_WWWWProgram)     ip_WWWWProgram->SendStandardizedName(theSocket, false);
}

void     TestingProgramFactory::SetNumber(int32_t serverType, string suffix, string workUnitName,
                                          uint64_t theK, uint32_t theB, uint32_t theN, int32_t theC, uint32_t theD)
{
   if (ip_LLRProgram)      ip_LLRProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_PRSTProgram)     ip_PRSTProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_PhrotProgram)    ip_PhrotProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_PFGWProgram)     ip_PFGWProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_GeneferProgram)  ip_GeneferProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_CycloProgram)    ip_CycloProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
}

// Return the most appropriate testing program given the server type and base
TestingProgram *TestingProgramFactory::GetPRPTestingProgram(int32_t serverType, uint64_t theK, uint32_t theB, uint32_t theN, uint32_t theD)
{
   bool     powerOf2 = false;
   
   if (serverType == ST_WAGSTAFF)
   {
      if (ip_LLRProgram) return ip_LLRProgram;

      return ip_PFGWProgram;
   }

   if (serverType == ST_GFN)
   {
      if (ip_GeneferProgram) return ip_GeneferProgram;

      return ip_PFGWProgram;
   }

   if ((serverType == ST_PRIMORIAL || serverType == ST_FACTORIAL))
   {
      if (ip_PRSTProgram) return ip_PRSTProgram;

      return ip_PFGWProgram;
   }

   if (serverType == ST_XYYX || serverType == ST_GENERIC || serverType == ST_CAROLKYNEA || serverType == ST_MULTIFACTORIAL)
      return ip_PFGWProgram;
   
   if (serverType == ST_CYCLOTOMIC)
   {
      while (theN > 2 && !(theN&1)) theN >>= 1;
      powerOf2 = (theN == 2);

      if (powerOf2 && ((theK == 3 && theB < 0) || (theK == 6 && theB > 0)) && ip_CycloProgram)
         return ip_CycloProgram;
       
      return ip_PFGWProgram;
   }

   if (serverType == ST_FIXEDBKC || serverType == ST_FIXEDBNC)
   {
      if (theD > 1)
      {
         if (ip_LLRProgram)   return ip_LLRProgram;
         if (ip_PFGWProgram)  return ip_PFGWProgram;
      }
   }

   // We assume that these programs can support whatever fell thru the cracks.
   if (ip_PRSTProgram)  return ip_PRSTProgram;
   if (ip_LLRProgram)   return ip_LLRProgram;
   if (ip_PFGWProgram)  return ip_PFGWProgram;
   if (ip_PhrotProgram) return ip_PhrotProgram;

   return 0;
}

