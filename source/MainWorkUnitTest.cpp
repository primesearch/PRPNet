#include "MainWorkUnitTest.h"

MainWorkUnitTest::MainWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                                   workunit_t *wu, TestingProgramFactory *testingProgramFactory)
            : PrimeWorkUnitTest(theLog, serverType, workSuffix, wu, testingProgramFactory)
{
   char  tempName[200];

   il_k = wu->l_k;
   ii_b = wu->i_b;
   ii_n = wu->i_n;
   il_c = wu->l_c;
   ii_d = wu->i_d;

   switch (serverType)
   {
      case ST_PRIMORIAL:
         snprintf(tempName, sizeof(tempName), "%d#%+" PRId64"", ii_b, il_c);
         break;

      case ST_FACTORIAL:
         snprintf(tempName, sizeof(tempName), "%d!%+" PRId64"", ii_b, il_c);
         break;

      case ST_MULTIFACTORIAL:
         snprintf(tempName, sizeof(tempName), "%d!%d%+" PRId64"", ii_b, ii_n, il_c);
         break;

      case ST_GFN:
         snprintf(tempName, sizeof(tempName), "%d^%d%+" PRId64"", ii_b, ii_n, il_c);
         break;

      case ST_LEYLAND:
         snprintf(tempName, sizeof(tempName), "%d^%d%c%d^%d", ii_b, ii_n, ((il_c == 1) ? '+' : '-'), ii_n, ii_b);
         break;

      case ST_LIFCHITZ:
         snprintf(tempName, sizeof(tempName), "%d^%d%c%d^%d", ii_b, ii_b, ((il_c == 1) ? '+' : '-'), ii_n, ii_n);
         break;

      case ST_GENERIC:
      case ST_CYCLOTOMIC:
         snprintf(tempName, sizeof(tempName), "%s", wu->s_Name);
         break;

      case ST_CAROLKYNEA:
         snprintf(tempName, sizeof(tempName), "(%d^%d%+" PRId64")^2-2", ii_b, ii_n, il_c);
         break;

      case ST_WAGSTAFF:
         snprintf(tempName, sizeof(tempName), "(2^%d+1)/3", ii_n);
         break;

      default:
        if (il_k > 1 && ii_d > 1)
           snprintf(tempName, sizeof(tempName), "(%" PRIu64"*%d^%d%+" PRId64")/%d", il_k, ii_b, ii_n, il_c, ii_d);
        else if (ii_d > 1)
           snprintf(tempName, sizeof(tempName), "(%d^%d%+" PRId64")/%d", ii_b, ii_n, il_c, ii_d);
        else
           snprintf(tempName, sizeof(tempName), "%" PRIu64"*%d^%d%+" PRId64"", il_k, ii_b, ii_n, il_c);
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
