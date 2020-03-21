#ifndef _TestingProgramFactory_

#define _TestingProgramFactory_

#include "Log.h"
#include "Socket.h"

#include "LLRProgram.h"
#include "PFGWProgram.h"
#include "PhrotProgram.h"
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
                      int64_t theK, int32_t theB, int32_t theN, int32_t theC);
   
   TestingProgram *GetPRPTestingProgram(void);
   TestingProgram *GetPRPTestingProgram(int32_t serverType,
                                        int64_t theK, int32_t theB, int32_t theN);

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
   WWWWProgram    *ip_WWWWProgram;
};

#endif // #ifndef _TestingProgramFactory_
