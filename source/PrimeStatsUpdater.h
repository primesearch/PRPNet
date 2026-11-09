#ifndef _PrimeStatsUpdater_

#define _PrimeStatsUpdater_

#include "StatsUpdater.h"

class PrimeStatsUpdater : public StatsUpdater
{
public:
   bool     InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD, double decimalLength);

   bool     UpdateStats(string   userID,
                        string   teamID,
                        string   candidateName,
                        double   decimalLength,
                        int32_t  gfnDivisors,
                        int32_t  numberOfTests,
                        int32_t  numberOfPRPs,
                        int32_t  numberOfPrimes);

   bool     RollupCandidateGroupStats(bool deleteInsert);

   bool     SetCandidateGroupId(string candidateName, int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD);

protected:
   bool     RollupUserStats(void);
   bool     RollupUserTeamStats(void);
   bool     RollupTeamStats(void);

   virtual char* GetGroupingColumnForCandidateGroupStats(void) { return "n"; };

   int32_t  GetCompletedThru(char* candidateGroupId, int32_t countInGroup, int32_t countUntested, int32_t countDoubleChecked);

   int32_t  GetLeadingEdge(char* candidateGroupId, int32_t maxInGroup, int32_t completedThru);

   bool     UpdateCandidateGroupStats(char* candidateGroupId);

   bool     UpdateUserStats(string   userID,
                            double   decimalLength,
                            int32_t  gfnDivisors,
                            int32_t  numberOfTests,
                            int32_t  numberOfPRPs,
                            int32_t  numberOfPrimes);

   bool     UpdateTeamStats(string   teamID,
                            double   decimalLength,
                            int32_t  gfnDivisors,
                            int32_t  numberOfTests,
                            int32_t  numberOfPRPs,
                            int32_t  numberOfPrimes);

   bool     UpdateUserTeamStats(string   userID,
                                string   teamID,
                                double   decimalLength,
                                int32_t  gfnDivisors,
                                int32_t  numberOfTests,
                                int32_t  numberOfPRPs,
                                int32_t  numberOfPrimes);

private:
   virtual char* GetInsertSqlForCandidateGroupStats(void) { return ""; };

   virtual string GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD) { return ""; };
};

#endif // #ifndef _PrimeStatsUpdater_
