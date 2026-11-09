#include "CarolKyneaStatsUpdater.h"
#include "SQLStatement.h"

string  CarolKyneaStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   sprintf(temp, "(%u^n%+" PRId64")^2-2", theB, theC);

   return temp;
}
