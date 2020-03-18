#include "TwinWorkUnitTest.h"

TwinWorkUnitTest::TwinWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                                   workunit_t *wu, TestingProgramFactory *testingProgramFactory) :
                  PrimeWorkUnitTest(theLog, serverType, workSuffix, wu, testingProgramFactory)
{
   char  tempName[200];

   il_k = wu->l_k;
   ii_b = wu->i_b;
   ii_n = wu->i_n;
   ii_c = -wu->i_c;

   if (serverType == ST_PRIMORIAL || serverType == ST_FACTORIAL || 
      serverType == ST_MULTIFACTORIAL || serverType == ST_XYYX || serverType == ST_GENERIC)
   {
      printf("Cannot create Twin workunit for servertype %d.  Exiting\n", serverType);
      exit(0);
   }

   sprintf(tempName, "%" PRId64"*%d^%d%+d", il_k, ii_b, ii_n, ii_c);
   is_ChildName = tempName;
}

TwinWorkUnitTest::~TwinWorkUnitTest()
{
}

void     TwinWorkUnitTest::LogMessage(WorkUnitTest *parentWorkUnit)
{
   result_t parentTestResult;

   parentTestResult = parentWorkUnit->GetWorkUnitTestResult();

   switch (parentTestResult)
   {
      case R_PRIME:
         switch (iwut_Result)
         {
            case R_PRIME:
               ip_Log->LogMessage("%s: %s/%s is a Prime Twin Pair!  Both numbers have been proven prime",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str());
               break;
            case R_PRP:
               ip_Log->LogMessage("%s: %s/%+d is a PRP Twin Pair!  %s is prime.  %s requires a primality test",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str(),
                                  is_ParentName.c_str(), is_ChildName.c_str());

            default:
               ip_Log->LogMessage("%s: %s is Prime, but %s is composite!",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str());
               break;
         }
         return;

      case R_PRP:
         switch (iwut_Result)
         {
            case R_PRP:
               ip_Log->LogMessage("%s: %s/%s is a PRP Twin Pair!  Both numbers require a primality test",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str());
               break;
            case R_PRIME:
               ip_Log->LogMessage("%s: %s/%+d is a PRP Twin Pair!  %s is prime.  %s requires a primality test",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str(),
                                  is_ParentName.c_str(), is_ChildName.c_str());

            default:
               ip_Log->LogMessage("%s: %s is PRP, but %s is composite!",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str());;
               break;
         }
         return;

      default:
         return;
   }
}
