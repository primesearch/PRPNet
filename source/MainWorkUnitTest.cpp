#include "MainWorkUnitTest.h"

MainWorkUnitTest::MainWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                                   workunit_t *wu, TestingProgramFactory *testingProgramFactory)
            : PrimeWorkUnitTest(theLog, serverType, workSuffix, wu, testingProgramFactory)
{
   char  tempName[200];

   il_k = wu->l_k;
   ii_b = wu->i_b;
   ii_n = wu->i_n;
   ii_c = wu->i_c;

   switch (serverType)
   {
      case ST_PRIMORIAL:
        sprintf(tempName, "%d#%+d", ii_b, ii_c);
        break;

      case ST_FACTORIAL:
        sprintf(tempName, "%d!%+d", ii_b, ii_c);
        break;

      case ST_MULTIFACTORIAL:
        sprintf(tempName, "%d!%d%+d", ii_b, ii_n, ii_c);
        break;

      case ST_GFN:
        sprintf(tempName, "%d^%d%+d", ii_b, ii_n, ii_c);
        break;

      case ST_XYYX:
         sprintf(tempName, "%d^%d%c%d^%d", ii_b, ii_n, ((ii_c == 1) ? '+' : '-'), ii_n, ii_b);
         break;

      case ST_GENERIC:
      case ST_CYCLOTOMIC:
         sprintf(tempName, "%s", wu->s_Name);
         break;

      case ST_CAROLKYNEA:
        sprintf(tempName, "(%d^%d%+d)^2-2", ii_b, ii_n, ii_c);
        break;

      case ST_WAGSTAFF:
        sprintf(tempName, "(2^%d+1)/3", ii_n);
        break;

      default:
        sprintf(tempName, "%" PRId64"*%d^%d%+d", il_k, ii_b, ii_n, ii_c);
        break;
   }

   is_ChildName = tempName;
}

MainWorkUnitTest::~MainWorkUnitTest()
{
}

void     MainWorkUnitTest::LogMessage(WorkUnitTest *masterWorkUnit)
{
   if (GetWorkUnitTestResult() == R_PRIME)
      ip_Log->LogMessage("%s: %s is prime", is_WorkSuffix.c_str(), is_ChildName.c_str());
   else if (GetWorkUnitTestResult() == R_PRP)
      ip_Log->LogMessage("%s: %s is PRP", is_WorkSuffix.c_str(), is_ChildName.c_str());
   else
      ip_Log->LogMessage("%s: %s is not prime.  Residue %s",
                         is_WorkSuffix.c_str(), is_ChildName.c_str(), is_Residue.c_str());
}
