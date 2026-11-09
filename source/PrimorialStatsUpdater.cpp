#include "PrimorialStatsUpdater.h"
#include "SQLStatement.h"

string  PrimorialStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   sprintf(temp, "n#%u%+" PRId64"", theB, theC);

   return temp;
}
