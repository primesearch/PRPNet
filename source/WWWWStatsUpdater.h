#ifndef _WWWWStatsUpdater_

#define _WWWWStatsUpdater_

#include "StatsUpdater.h"

class WWWWStatsUpdater : public StatsUpdater
{
public:
   bool     UpdateStats(string   userID,
                        string   teamID,
                        int32_t  finds,
                        int32_t  nearFinds);

private:
   bool     RollupUserStats(void);
   bool     RollupUserTeamStats(void);
   bool     RollupTeamStats(void);

   bool     RollupGroupStats(bool deleteInsert);

   bool     UpdateGroupStats(void);

   bool     UpdateUserStats(string   userID,
                           int32_t  finds,
                           int32_t  nearFinds);

   bool     UpdateUserTeamStats(string   userID,
                                string   teamID,
                                int32_t  finds,
                                int32_t  nearFinds);

   bool     UpdateTeamStats(string   teamID,
                            int32_t  finds,
                            int32_t  nearFinds);
};

#endif // #ifndef _WWWWStatsUpdater_
