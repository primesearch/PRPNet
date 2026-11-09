#ifndef _GFNStatsUpdater_

#define _GFNStatsUpdater_

#include "StatsUpdater.h"

class GFNStatsUpdater : public StatsUpdater
{
public:
   bool RequiresCandidateGroupId(void) { return true; }

private:
   char* GetGroupingColumnForCandidateGroupStats(void) { return "b"; }

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _GFNStatsUpdater_
