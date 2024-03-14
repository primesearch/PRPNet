#ifndef _SophieGermainStatsUpdater_

#define _SophieGermainStatsUpdater_

#include "PrimeStatsUpdater.h"

class SophieGermainStatsUpdater : public PrimeStatsUpdater
{
private:
   bool  RollupGroupStats(bool deleteInsert);

   bool  UpdateGroupStats(string candidateName);

   bool  UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC);

   bool  InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                               int32_t theC, int32_t theD, double decimalLength);
};

#endif // #ifndef _SophieGermainStatsUpdater_
