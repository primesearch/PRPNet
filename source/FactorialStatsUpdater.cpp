#include "FactorialStatsUpdater.h"
#include "SQLStatement.h"

string  FactorialStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   sprintf(temp, "n!%+" PRId64"", theC);

   return temp;
}
