#ifndef _FixedBKCStatsUpdater_

#define _FixedBKCStatsUpdater_

#include "PrimeStatsUpdater.h"

class FixedBKCStatsUpdater : public PrimeStatsUpdater
{
private:
   bool  RollupGroupStats(bool deleteInsert);

   bool  UpdateGroupStats(string candidateName);

   bool  UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);

   bool  InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                               int64_t theC, int32_t theD, double decimalLength);
};

#endif // #ifndef _FixedBKCStatsUpdater_
