#include "GFNStatsUpdater.h"
#include "SQLStatement.h"

string  GFNStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   sprintf(temp, "b^%d+1", theN);

   return temp;
}
