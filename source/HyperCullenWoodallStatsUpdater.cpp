#include "HyperCullenWoodallStatsUpdater.h"
#include "SQLStatement.h"

string  HyperCullenWoodallStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   sprintf(temp, "%u^n*n^%u%+" PRId64"" , theB, theB, theC);

   return temp;
}