#ifndef _CyclotomicStatsUpdater_

#define _CyclotomicStatsUpdater_

#include "PrimeStatsUpdater.h"

class CyclotomicStatsUpdater : public PrimeStatsUpdater
{
private:
   bool  RollupGroupStats(bool deleteInsert);

   bool  UpdateGroupStats(string candidateName);

   bool  UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int64_t theC);

   bool  InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                               int64_t theC, int32_t theD, double decimalLength);
};

#endif // #ifndef _CyclotomicStatsUpdater_
