#ifndef _GenericStatsUpdater_

#define _GenericStatsUpdater_

#include "PrimeStatsUpdater.h"

class GenericStatsUpdater : public PrimeStatsUpdater
{
private:
   string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);

   char* GetInsertSqlForCandidateGroupStats(void) {
      return "insert into CandidateGroupStats (candidateGroupId) (select distinct candidateGroupId from Candidate)";
   };

   bool   UpdateCandidateGroupStats(char* candidateGroupId);

   bool   InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                               int64_t theC, int32_t theD, double decimalLength);

   string is_GroupName = "n/a";
};

#endif // #ifndef _GenericStatsUpdater_
