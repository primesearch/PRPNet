#include "LeylandStatsUpdater.h"
#include "SQLStatement.h"

string  LeylandStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   if (theC == 1)
      sprintf(temp, "%d^y+y^%d", theB, theB);
   else
      sprintf(temp, "%d^y-y^%d", theB, theB);

   return temp;
}