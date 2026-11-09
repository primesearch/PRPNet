#ifndef _CyclotomicStatsUpdater_

#define _CyclotomicStatsUpdater_

#include "PrimeStatsUpdater.h"

class CyclotomicStatsUpdater : public PrimeStatsUpdater
{
public:

private:
   char  *GetGroupingColumnForCandidateGroupStats(void) { return "abs(b)"; }

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId, k, n) (select distinct candidateGroupId, k, n from Candidate)";
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _CyclotomicStatsUpdater_
