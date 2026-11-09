#include "FixedBKCStatsUpdater.h"
#include "SQLStatement.h"

string  FixedBKCStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   if (theD > 1)
      sprintf(temp, "(k*%d^%d%+" PRId64")/%d", theB, theN, theC, theD);
   else
      sprintf(temp, "k*%d^%d%+" PRId64"", theB, theN, theC);

   return temp;
}
