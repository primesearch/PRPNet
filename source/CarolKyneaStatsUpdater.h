#ifndef _CarolKyneaStatsUpdater_

#define _CarolKyneaStatsUpdater_

#include "PrimeStatsUpdater.h"

class CarolKyneaStatsUpdater : public PrimeStatsUpdater
{
public:

private:
   char* GetGroupingColumnForCandidateGroupStats(void) { return "n"; }

   char* GetInsertSqlForCandidateGroupStats(void) { 
      return "insert into CandidateGroupStats (candidateGroupId, b, c) (select distinct candidateGroupId, b, c from Candidate)"; 
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _CarolKyneaStatsUpdater_
