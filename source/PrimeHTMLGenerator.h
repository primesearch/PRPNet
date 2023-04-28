#ifndef _PrimeHTMLGenerator_

#define _PrimeHTMLGenerator_

#include <string>
#include "HTMLGenerator.h"
#include "ServerHelper.h"
#include "SQLStatement.h"

typedef enum { BY_K, BY_B, BY_N, BY_Y } sss_t;

class PrimeHTMLGenerator : public HTMLGenerator
{
public:
   PrimeHTMLGenerator(globals_t *theGlobals) : HTMLGenerator(theGlobals) {};

   virtual ~PrimeHTMLGenerator() {};

   void     PendingTests(void);

   void     PrimesByUser(void);

   void     PrimesByTeam(void);

   void     AllPrimes(void);

   void     GFNDivisorsByUser(void);

   bool     CanCheckForGFNs(void);

   void     UserStats(void);
   void     UserTeamStats(void);
   void     TeamUserStats(void);
   void     TeamStats(void);
   void     ServerStatsHeader(sss_t summarizedBy);

   virtual void  ServerStats(void) {};

protected:

   bool          SendSpecificPage(string thePage);
   ServerHelper *GetServerHelper(void);

   void     SendLinks();
};

#endif // #ifndef _PrimeHTMLGenerator_
