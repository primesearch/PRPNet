#ifndef _WagstaffStatsUpdater_

#define _WagstaffStatsUpdater_

#include "PrimeStatsUpdater.h"

class WagstaffStatsUpdater : public PrimeStatsUpdater
{
public:

private:
   char* GetGroupingColumnForCandidateGroupStats(void) { return "n"; }

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId) (select distinct candidateGroupId from Candidate)";
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _WagstaffStatsUpdater_
