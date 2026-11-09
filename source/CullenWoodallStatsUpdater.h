#ifndef _CullenWoodallStatsUpdater_

#define _CullenWoodallStatsUpdater_

#include "PrimeStatsUpdater.h"

class CullenWoodallStatsUpdater : public PrimeStatsUpdater
{
public:

private:
   char  *GetGroupingColumnForCandidateGroupStats(void) { return "n"; }

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId, b) (select distinct candidateGroupId, b from Candidate)";
   };

   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);
};

#endif // #ifndef _CullenWoodallStatsUpdater_
