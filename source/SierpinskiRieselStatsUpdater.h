#ifndef _SierpinskiRieselStatsUpdater_

#define _SierpinskiRieselStatsUpdater_

#include "PrimeStatsUpdater.h"

class SierpinskiRieselStatsUpdater : public PrimeStatsUpdater
{
public:

public:
   bool  SetSierspinkiRieselPrimeN(char *candidateGroupId, int32_t theN);

private:
   char* GetGroupingColumnForCandidateGroupStats(void) { return "n"; }

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId, k, b, c, d) (select distinct candidateGroupId, k, b, c, d from Candidate)";
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _SierpinskiRieselStatsUpdater_
