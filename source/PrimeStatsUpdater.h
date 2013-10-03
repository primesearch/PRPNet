#ifndef _PrimeStatsUpdater_

#define _PrimeStatsUpdater_

#include "StatsUpdater.h"

class PrimeStatsUpdater : public StatsUpdater
{
public:
   virtual bool   RollupGroupStats(bool deleteInsert) { return false; };

   virtual bool   UpdateGroupStats(string candidateName) { return false; };

   virtual bool   UpdateGroupStats(int64_t theK, int32_t theB, int32_t theN, int32_t theC) { return false; };

   virtual bool   InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                  int32_t theC, double decimalLength) { return false; };

   bool     UpdateStats(string   userID,
                        string   teamID,
                        string   candidateName,
                        double   decimalLength,
                        result_t testResult,
                        int32_t  gfnDivisors);

protected:
   bool     RollupUserStats(void);
   bool     RollupUserTeamStats(void);
   bool     RollupTeamStats(void);

   bool     UpdateUserStats(string   userID,
                            double   decimalLength,
                            result_t testResult,
                            int32_t  gfnDivisors);

   bool     UpdateTeamStats(string   teamID,
                            double   decimalLength,
                            result_t testResult,
                            int32_t  gfnDivisors);

   bool     UpdateUserTeamStats(string   userID,
                                string   teamID,
                                double   decimalLength,
                                result_t testResult,
                                int32_t  gfnDivisors);

};

#endif // #ifndef _PrimeStatsUpdater_
