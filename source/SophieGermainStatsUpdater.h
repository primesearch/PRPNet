#ifndef _SophieGermainStatsUpdater_

#define _SophieGermainStatsUpdater_

#include "PrimeStatsUpdater.h"

class SophieGermainStatsUpdater : public PrimeStatsUpdater
{
public:

private:
   char* GetGroupingColumnForCandidateGroupStats(void) { return "n"; }

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId, b, n, c) (select distinct candidateGroupId, b, n, c from Candidate)";
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _SophieGermainStatsUpdater_
