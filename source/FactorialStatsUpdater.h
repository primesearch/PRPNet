#ifndef _FactorialStatsUpdater_

#define _FactorialStatsUpdater_

#include "PrimeStatsUpdater.h"

class FactorialStatsUpdater : public PrimeStatsUpdater
{
public:

private:
   char* GetGroupingColumnForCandidateGroupStats(void) { return "n"; }

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId, c) (select distinct candidateGroupId, c from Candidate)";
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _FactorialStatsUpdater_
