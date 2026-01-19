#include "TestingProgramFactory.h"

TestingProgramFactory::TestingProgramFactory(Log *theLog, string configFile)
{
   int32_t gpuAffinity = -1, normalPriority = 0;
   FILE   *fp;
   char    line[2001];
   string  cpuAffinity = "";

   ip_LLRProgram = nullptr;
   ip_PhrotProgram = nullptr;
   ip_PFGWProgram = nullptr;
   ip_GeneferProgram = nullptr;
   ip_CycloProgram = nullptr;
   ip_WWWWProgram = nullptr;
   ip_PRSTProgram = nullptr;
   ip_DMDivisorProgram = nullptr;
   ip_GFNDivisorProgram = nullptr;

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
      else if (!memcmp(line, "cycloexe=", 9) && strlen(line) > 9)
         ip_CycloProgram = new CycloProgram(theLog, line+9);
      else if (!memcmp(line, "geneferexe=", 11) && strlen(line) > 11)
      {
         if (!ip_GeneferProgram)
            ip_GeneferProgram = new GeneferProgram(theLog, line+11);
         
         ip_GeneferProgram->AddProgram(line+11);
      }
      else if (!memcmp(line, "dmdsieveexe=", 12) && strlen(line) > 12)
         ip_DMDivisorProgram = new DMDivisorProgram(theLog, line + 12);
      else if (!memcmp(line, "gfndsieveexe=", 13) && strlen(line) > 13)
         ip_GFNDivisorProgram = new GFNDivisorProgram(theLog, line + 13);
      else if (!memcmp(line, "normalpriority=", 15) && strlen(line) > 15)
         normalPriority = atoi(line+15);
      else if (!memcmp(line, "cpuaffinity=", 12) && strlen(line) > 12)
         cpuAffinity = line+12;
      else if (!memcmp(line, "gpuaffinity=", 12) && strlen(line) > 12)
         gpuAffinity = atol(line+12);
   }

   if (ip_LLRProgram)      ip_LLRProgram->SetCpuAffinity(cpuAffinity);
   if (ip_GeneferProgram)  ip_GeneferProgram->SetGpuAffinity(gpuAffinity);
   if (ip_PFGWProgram)     ip_PFGWProgram->SetCpuAffinity(cpuAffinity);
   if (ip_PFGWProgram)     ip_PFGWProgram->SetNormalPriority(normalPriority);
   if (ip_PRSTProgram)     ip_PRSTProgram->SetCpuAffinity(cpuAffinity);
}

TestingProgramFactory::~TestingProgramFactory()
{
   if (ip_LLRProgram)        delete ip_LLRProgram;
   if (ip_PhrotProgram)      delete ip_PhrotProgram;
   if (ip_PFGWProgram)       delete ip_PFGWProgram;
   if (ip_PRSTProgram)       delete ip_PRSTProgram;
   if (ip_GeneferProgram)    delete ip_GeneferProgram;
   if (ip_CycloProgram)      delete ip_CycloProgram;
   if (ip_WWWWProgram)       delete ip_WWWWProgram;
   if (ip_DMDivisorProgram)  delete ip_DMDivisorProgram;
   if (ip_GFNDivisorProgram) delete ip_GFNDivisorProgram;
}

bool     TestingProgramFactory::ValidatePrograms(void)
{
   if (ip_LLRProgram && !ip_LLRProgram->ValidateExe())
   {
      delete ip_LLRProgram;
      ip_LLRProgram = nullptr;
   }

   if (ip_PRSTProgram && !ip_PRSTProgram->ValidateExe())
   {
      delete ip_PRSTProgram;
      ip_PRSTProgram = nullptr;
   }

   if (ip_PhrotProgram && !ip_PhrotProgram->ValidateExe())
   {
      delete ip_PhrotProgram;
      ip_PhrotProgram = nullptr;
   }

   if (ip_PFGWProgram && !ip_PFGWProgram->ValidateExe())
   {
      delete ip_PFGWProgram;
      ip_PFGWProgram = nullptr;
   }

   if (ip_GeneferProgram && !ip_GeneferProgram->ValidateExe())
   {
      delete ip_GeneferProgram;
      ip_GeneferProgram = nullptr;
   }
   
   if (ip_CycloProgram && !ip_CycloProgram->ValidateExe())
   {
      delete ip_CycloProgram;
      ip_CycloProgram = nullptr;
   }

   if (ip_WWWWProgram && !ip_WWWWProgram->ValidateExe())
   {
      delete ip_WWWWProgram;
      ip_WWWWProgram = nullptr;
   }

   if (ip_GFNDivisorProgram && !ip_GFNDivisorProgram->ValidateExe())
   {
      delete ip_GFNDivisorProgram;
      ip_GFNDivisorProgram = nullptr;
   }

   if (ip_DMDivisorProgram && !ip_DMDivisorProgram->ValidateExe())
   {
      delete ip_DMDivisorProgram;
      ip_DMDivisorProgram = nullptr;
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
   if (ip_LLRProgram)         ip_LLRProgram->SendStandardizedName(theSocket, false);
   if (ip_PRSTProgram)        ip_PRSTProgram->SendStandardizedName(theSocket, false);
   if (ip_LLRProgram)         ip_LLRProgram->SendStandardizedName(theSocket, false);
   if (ip_PFGWProgram)        ip_PFGWProgram->SendStandardizedName(theSocket, false);
   if (ip_GeneferProgram)     ip_GeneferProgram->SendStandardizedName(theSocket, false);
   if (ip_CycloProgram)       ip_CycloProgram->SendStandardizedName(theSocket, false);
   if (ip_WWWWProgram)        ip_WWWWProgram->SendStandardizedName(theSocket, false);
   if (ip_GFNDivisorProgram)  ip_GFNDivisorProgram->SendStandardizedName(theSocket, false);
   if (ip_DMDivisorProgram)   ip_DMDivisorProgram->SendStandardizedName(theSocket, false);
}

void     TestingProgramFactory::SetNumber(int32_t serverType, string suffix, string workUnitName,
                                          uint64_t theK, uint32_t theB, uint32_t theN, int64_t theC, uint32_t theD)
{
   if (ip_LLRProgram)      ip_LLRProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_PRSTProgram)     ip_PRSTProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_PhrotProgram)    ip_PhrotProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_PFGWProgram)     ip_PFGWProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_GeneferProgram)  ip_GeneferProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
   if (ip_CycloProgram)    ip_CycloProgram->SetNumber(serverType, suffix, workUnitName, theK, theB, theN, theC, theD);
}

// Return the most appropriate PRP testing program given the server type and base
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

   if (serverType == ST_LEYLAND || serverType == ST_LIFCHITZ || serverType == ST_HYPERCW)
      return ip_PFGWProgram;
   
   if (serverType == ST_CAROLKYNEA || serverType == ST_MULTIFACTORIAL || serverType == ST_GENERIC)
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
         // For GRUs, use PFGW if the base is divisible by 3 to avoid false positives
         // as LLR uses base 3 for the PRP test and yields false positives.
         if (theK == 1 && theB % 3 == 0)
            if (ip_PFGWProgram)  return ip_PFGWProgram;

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

// Return the most appropriate primality testing program given the server type and base
TestingProgram *TestingProgramFactory::GetPrimalityTestingProgram(int32_t serverType, uint32_t theB, uint32_t theN, int64_t theC, uint32_t theD)
{
   // For GFN, factorials and primorials, don't do primality tests for larger n because
   // they can take days and PFGW does not checkpoint during the primality test.  If PFGW
   // is modified to checkpoint during primality tests, then this could be removed.
   if (serverType == ST_GFN && theN > 200000)
      return 0;

   if (serverType == ST_FACTORIAL && theB > 500000)
      return 0;

   if (serverType == ST_PRIMORIAL && theB > 500000)
      return 0;

   switch (serverType) {
   // For Wagstaff, pfgw cannot prove primality
      case ST_WAGSTAFF:
   // For XYYX, pfgw cannot prove primality
      case ST_LEYLAND:
      case ST_LIFCHITZ:
   // Don't know if pfgw can prove primality, so don't try
      case ST_GENERIC:
         return 0;
      case ST_FIXEDBKC:
      case ST_FIXEDBNC:
         if (theD != 1)
            return 0;

         if (theC > 1 || theC < -1)
            return 0;

         if (theD > 1) {
            if (ip_PFGWProgram != NULL)  return ip_PFGWProgram;
         } else {
            if (ip_PRSTProgram != NULL)  return ip_PRSTProgram;
            if (ip_PFGWProgram != NULL)  return ip_PFGWProgram;
         }
         break;
      case ST_SIERPINSKIRIESEL:
      case ST_CULLENWOODALL:
      case ST_TWIN:
      case ST_TWINANDSOPHIE:
         if (theD != 1)
            return 0;

         if (ip_PRSTProgram != NULL)  return ip_PRSTProgram;
         if (ip_PFGWProgram != NULL)  return ip_PFGWProgram;
         break;
      default:
         if (theC > 1 || theC < -1)
            return 0;
         if (ip_PFGWProgram != NULL)
            return ip_PFGWProgram;
   }

   return 0;
}

