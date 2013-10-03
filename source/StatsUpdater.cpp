#include "StatsUpdater.h"
#include "SQLStatement.h"

bool  StatsUpdater::RollupAllStats(void)
{
   if (!RollupUserStats()) return false;
   if (!RollupTeamStats()) return false;
   if (!RollupUserTeamStats()) return false;

   return true;
}
