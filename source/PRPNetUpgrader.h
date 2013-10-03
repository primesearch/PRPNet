#ifndef _PRPNetUpgrader_

#define _PRPNetUpgrader_

#include "defs.h"
#include "Log.h"
#include "Candidate.h"
#include "DBInterface.h"
#include "server.h"

class PRPNetUpgrader
{
public:
   PRPNetUpgrader(globals_t *theGlobals, DBInterface *dbInterface);

   void  UpgradeFromV2ToV3(void);

private:
   int32_t     ii_ServerType;
   int32_t     ii_NeedsDoubleCheck;
   int32_t     ii_CandidateCount;
   int32_t     ii_CandidateSize;
   Candidate **ii_Candidates;

   Log        *ip_Log;

   DBInterface *ip_DBInterface;

   void      ProcessUserStatsFile(char *userStatsFile);
   void      LoadCandidates(char *saveFileName);
   int32_t   AddCandidate(Candidate *theCandidate);
};

#endif // #ifndef _PRPNetUpgrader_
