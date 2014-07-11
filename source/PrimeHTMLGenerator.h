#ifndef _PrimeHTMLGenerator_

#define _PrimeHTMLGenerator_

#include <string>
#include "HTMLGenerator.h"

typedef enum { BY_K, BY_B, BY_N, BY_Y } sss_t;

class PrimeHTMLGenerator : public HTMLGenerator
{
public:
   PrimeHTMLGenerator(globals_t *theGlobals) : HTMLGenerator(theGlobals) {};

   virtual ~PrimeHTMLGenerator() {};

   void     Send(string thePage);

   void     PendingTests(void);

   void     PrimesByUser(void);

   void     PrimesByTeam(void);

   void     GFNDivisorsByUser(void);

   bool     CanCheckForGFNs(void);
   bool     HasTeams(void);

   void     UserStats(void);
   void     UserTeamStats(void);
   void     TeamUserStats(void);
   void     TeamStats(void);
   void     ServerStatsHeader(sss_t summarizedBy);

   virtual void  ServerStats(void) {};

private:
   string   TimeToString(time_t theTime);
   void     HeaderPlusLinks(string pageTitle);
   void     GetDaysLeft(void);
};

#endif // #ifndef _PrimeHTMLGenerator_
