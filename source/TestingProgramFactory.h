#ifndef _TestingProgramFactory_

#define _TestingProgramFactory_

#include "Log.h"
#include "Socket.h"

#include "LLRProgram.h"
#include "PFGWProgram.h"
#include "PhrotProgram.h"
#include "GeneferProgram.h"
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

   TestingProgram *GetPRPTestingProgram(int32_t serverType, int32_t theB);
   TestingProgram *GetPRPTestingProgram(void);
   LLRProgram     *GetLLRProgram(void)     { return ip_LLRProgram;     };
   PFGWProgram    *GetPFGWProgram(void)    { return ip_PFGWProgram;    };
   PhrotProgram   *GetPhrotProgram(void)   { return ip_PhrotProgram;   };
   GeneferProgram *GetGeneferProgram(void) { return ip_GeneferProgram; };
   WWWWProgram    *GetWWWWProgram(void)    { return ip_WWWWProgram;    };

private:
   Log            *ip_Log;

   LLRProgram     *ip_LLRProgram;
   PFGWProgram    *ip_PFGWProgram;
   PhrotProgram   *ip_PhrotProgram;
   GeneferProgram *ip_GeneferProgram;
   WWWWProgram    *ip_WWWWProgram;
};

#endif // #ifndef _TestingProgramFactory_
