#ifndef _CullenWoodallStatsUpdater_

#define _CullenWoodallStatsUpdater_

#include "PrimeStatsUpdater.h"

class CullenWoodallStatsUpdater : public PrimeStatsUpdater
{
private:
   bool  RollupGroupStats(bool deleteInsert);

   bool  UpdateGroupStats(string candidateName);

   bool  UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC);

   bool  InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                          int32_t theC, double decimalLength);
};

#endif // #ifndef _CullenWoodallStatsUpdater_
