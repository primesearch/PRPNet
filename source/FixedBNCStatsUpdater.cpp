#include "FixedBNCStatsUpdater.h"
#include "SQLStatement.h"

string  FixedBNCStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   if (theD > 1 && theK > 1)
      sprintf(temp, "(%" PRIu64"*%d^n%+" PRId64")/%d", theK, theB, theC, theD);
   else if (theD > 1)
      sprintf(temp, "(%d^n%+" PRId64")/%d", theB, theC, theD);
   else if (theK > 1)
      sprintf(temp, "%" PRIu64"*%d^n%+" PRId64"", theK, theB, theC);
   else
      sprintf(temp, "%d^n%+" PRId64"", theB, theC);

   return temp;
}
