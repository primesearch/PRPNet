#include "CullenWoodallStatsUpdater.h"
#include "SQLStatement.h"

string  CullenWoodallStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   if (theK == 1)
   {
      if (theC < 0)
         sprintf(temp, "%u^n-n", theB);
      else
         sprintf(temp, "%u^n+n", theB);
   }
   else
      sprintf(temp, "n*%u^n%" PRId64"", theB, theC);

   return temp;
}
