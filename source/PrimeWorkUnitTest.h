#ifndef _PrimeWorkUnitTest_

#define _PrimeWorkUnitTest_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "WorkUnitTest.h"
#include "TestingProgramFactory.h"
#include "workunit.h"
#include "gfndivisor.h"

class WorkUnitTest;

class PrimeWorkUnitTest : public WorkUnitTest
{
public:
   PrimeWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                     workunit_t *wu, TestingProgramFactory *testingProgramFactory);

   ~PrimeWorkUnitTest();

   string   GetParentName(void) { return is_ParentName; };
   string   GetName(void)       { return is_ChildName;  };
   string   GetResidue(void)    { return is_Residue;    };
   string   GetProver(void)     { return is_Prover;     };
   bool     DidSearchForGFNDivisors(void) { return ib_SearchedForGFNDivisors; };
   gfndivisor_t   *GetGFNList(void)    { return ip_FirstGFN;    };

   bool     TestWorkUnit(WorkUnitTest *masterWorkUnit);
   void     SendResults(Socket *theSocket);
   void     Save(FILE *saveFile);
   void     Load(FILE *saveFile, string lineIn, string prefix);

protected:
   virtual void   LogMessage(WorkUnitTest *parentWorkUnit) {};
   virtual string GetResultPrefix(void) { return ""; };

   string   is_ParentName;
   string   is_ChildName;

   string   is_Residue;
   string   is_Prover;
   string   is_ProverVersion;

   // This represents the number being tested
   uint64_t il_k;
   uint32_t ii_b;
   uint32_t ii_n;
   int64_t  il_c;
   uint32_t ii_d;

   bool     ib_HadTestFailure;
   bool     ib_SearchedForGFNDivisors;
   gfndivisor_t   *ip_FirstGFN;

private:
   testresult_t   DoPRPTest(void);
   testresult_t   DoPrimalityTest(void);
   testresult_t   CheckForGFNDivisibility(void);
};

#endif // #ifndef _PrimeWorkUnitTest_

