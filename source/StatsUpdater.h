#ifndef _StatsUpdater_

#define _StatsUpdater_

#include "defs.h"
#include "DBInterface.h"
#include "SQLStatement.h"
#include "Log.h"

class StatsUpdater
{
public:
   inline  StatsUpdater() { ip_CandidateLoader = NULL; };

   virtual ~StatsUpdater() { if (ip_CandidateLoader) delete ip_CandidateLoader; };

   void     SetDBInterface(DBInterface *dbInterface) { ip_DBInterface = dbInterface; };
   void     SetLog(Log *theLog) { ip_Log = theLog; };
   void     SetNeedsDoubleCheck(int32_t needsDoubleCheck) { ib_NeedsDoubleCheck = needsDoubleCheck; };

   bool     RollupAllStats(void);

   virtual bool   RollupGroupStats(bool deleteInsert) { return false; };

   virtual bool   InsertCandidate(string candidateName, int64_t theK, int32_t theB, int32_t theN,
                                  int32_t theC, double decimalLength) { return false; };

protected:
   int32_t       ib_NeedsDoubleCheck;
   DBInterface  *ip_DBInterface;
   SQLStatement *ip_CandidateLoader;
   Log          *ip_Log;

   virtual bool   RollupUserStats(void) { return false; };
   virtual bool   RollupUserTeamStats(void) { return false; };
   virtual bool   RollupTeamStats(void) { return false; };
};

#endif // #ifndef _StatsUpdater_
