#ifndef _FixedBKCStatsUpdater_

#define _FixedBKCStatsUpdater_

#include "PrimeStatsUpdater.h"

class FixedBKCStatsUpdater : public PrimeStatsUpdater
{
public:

private:
   char* GetGroupingColumnForCandidateGroupStats(void) { return "n"; }

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId, k, b, c) (select distinct candidateGroupId, k, b, c from Candidate)";
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _FixedBKCStatsUpdater_
