#ifndef _XYYXStatsUpdater_

#define _XYYXStatsUpdater_

#include "StatsUpdater.h"

class XYYXStatsUpdater : public StatsUpdater
{
private:
   bool  RollupGroupStats(bool deleteInsert);

   bool  UpdateGroupStats(string candidateName);

   bool  UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC);

   bool  InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                               int32_t theC, int32_t theD, double decimalLength);
};

#endif // #ifndef _XYYXStatsUpdater_
