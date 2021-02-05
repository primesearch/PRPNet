#ifndef _SierpinskiRieselStatsUpdater_

#define _SierpinskiRieselStatsUpdater_

#include "PrimeStatsUpdater.h"

class SierpinskiRieselStatsUpdater : public PrimeStatsUpdater
{
public:
   bool  SetSierspinkiRieselPrimeN(int64_t theK, int32_t theB, int32_t theN, int32_t theC);

private:
   bool  SetHasSierspinkiRieselPrime(int64_t theK, int32_t theB, int32_t theC, bool &foundOne);

   bool  RollupGroupStats(bool deleteInsert);

   bool  UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC);

   bool  InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                         int32_t theC, double decimalLength);
};

#endif // #ifndef _SierpinskiRieselStatsUpdater_
