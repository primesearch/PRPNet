#ifndef _TestingProgramFactory_

#define _TestingProgramFactory_

#include "Log.h"
#include "Socket.h"

#include "LLRProgram.h"
#include "PFGWProgram.h"
#include "PhrotProgram.h"
#include "PRSTProgram.h"
#include "GeneferProgram.h"
#include "CycloProgram.h"
#include "WWWWProgram.h"

class TestingProgramFactory
{
public:
   TestingProgramFactory(Log *theLog, string configFile);

   ~TestingProgramFactory();

   bool     ValidatePrograms(void);
   void     SendPrograms(Socket *theSocket);

   void     SetNumber(int32_t serverType, string suffix, string workUnitName,
                      uint64_t theK, uint32_t theB, uint32_t theN, int32_t theC, uint32_t theD);
   
   TestingProgram *GetPRPTestingProgram(int32_t serverType,
                                        uint64_t theK, uint32_t theB, uint32_t theN, uint32_t theD);

   TestingProgram *GetPrimalityTestingProgram(int32_t serverType,
                                              uint32_t theB, uint32_t theN, int32_t theC, uint32_t theD);

   LLRProgram     *GetLLRProgram(void)     { return ip_LLRProgram;     };
   PFGWProgram    *GetPFGWProgram(void)    { return ip_PFGWProgram;    };
   PhrotProgram   *GetPhrotProgram(void)   { return ip_PhrotProgram;   };
   GeneferProgram *GetGeneferProgram(void) { return ip_GeneferProgram; };
   CycloProgram   *GetCycloProgram(void)   { return ip_CycloProgram;   };
   WWWWProgram    *GetWWWWProgram(void)    { return ip_WWWWProgram;    };

private:

   LLRProgram     *ip_LLRProgram;
   PFGWProgram    *ip_PFGWProgram;
   PhrotProgram   *ip_PhrotProgram;
   GeneferProgram *ip_GeneferProgram;
   CycloProgram   *ip_CycloProgram;
   PRSTProgram    *ip_PRSTProgram;
   WWWWProgram    *ip_WWWWProgram;
};

#endif // #ifndef _TestingProgramFactory_
