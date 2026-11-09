#include "WagstaffStatsUpdater.h"
#include "SQLStatement.h"

string  WagstaffStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   sprintf(temp, "(2^n+1)/3");

   return temp;
}
