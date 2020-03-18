#include "SophieGermainWorkUnitTest.h"

SophieGermainWorkUnitTest::SophieGermainWorkUnitTest(Log *theLog, int32_t serverType, string workSuffix,
                                   workunit_t *wu, TestingProgramFactory *testingProgramFactory, 
                                   sgtype_t sgType)
         : PrimeWorkUnitTest(theLog, serverType, workSuffix, wu, testingProgramFactory)
{
   char  tempName[200];
   il_k = wu->l_k;
   ii_b = wu->i_b;

   if (sgType == SG_NM1)
      ii_n = wu->i_n + 1;
   else
      ii_n = wu->i_n - 1;

   ii_c = wu->i_c;
   isg_Type = sgType;

   if (serverType == ST_PRIMORIAL || serverType == ST_FACTORIAL ||
      serverType == ST_MULTIFACTORIAL || serverType == ST_XYYX || serverType == ST_GENERIC)
   {
      printf("Cannot create Sophie-Germain workunit for servertype %d.  Exiting\n", serverType);
      exit(0);
   }

   sprintf(tempName, "%" PRId64"*%d^%d%+d", il_k, ii_b, ii_n, ii_c);
   is_ChildName = tempName;
}

SophieGermainWorkUnitTest::~SophieGermainWorkUnitTest()
{
}

void     SophieGermainWorkUnitTest::LogMessage(WorkUnitTest *parentWorkUnit)
{
   result_t parentTestResult;
   char     form[10];

   if (isg_Type == SG_NM1)
      strcpy(form, "n-1");
   else
      strcpy(form, "n+1");

   parentTestResult = parentWorkUnit->GetWorkUnitTestResult();

   switch (parentTestResult)
   {
      case R_PRIME:
         switch (iwut_Result)
         {
            case R_PRIME:
               ip_Log->LogMessage("%s: %s/%s is a Prime Sophie Germain Pair (%s)!  Both numbers have been proven prime",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str(), form);
               break;
            case R_PRP:
               ip_Log->LogMessage("%s: %s/%+d is a PRP Sophie Germain Pair (%s)!  %s is prime.  %s requires a primality test",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str(),
                                  form, is_ParentName.c_str(), is_ChildName.c_str());

            default:
               ip_Log->LogMessage("%s: %s is Prime, but Sophie Germain (%s) %s is composite!",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), form, is_ChildName.c_str());
               break;
         }
         return;

      case R_PRP:
         switch (iwut_Result)
         {
            case R_PRP:
               ip_Log->LogMessage("%s: %s/%s is a PRP Sophie Germain Pair (%s)!  Both numbers require a primality test",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str(), form);
               break;
            case R_PRIME:
               ip_Log->LogMessage("%s: %s/%+d is a PRP Sophie Germain Pair (%s)!  %s is prime.  %s requires a primality test",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), is_ChildName.c_str(), form, is_ChildName.c_str(), is_ParentName.c_str());

            default:
               ip_Log->LogMessage("%s: %s is PRP, but Sophie Germain (%s) %s is composite (%s)!",
                                  is_WorkSuffix.c_str(), is_ParentName.c_str(), form, is_ChildName.c_str());
               break;
         }
         return;

      default:
         return;
   }
}
