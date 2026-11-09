#include "CyclotomicStatsUpdater.h"
#include "SQLStatement.h"

string  CyclotomicStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   if (theN == 1)
      sprintf(temp, "Phi(%" PRIu64", b", theK);
   else
      sprintf(temp, "Phi(%" PRIu64", b^%u", theK, theN);

   return temp;
}
