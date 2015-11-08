#ifndef _GenericHTML_

#define _GenericHTML_

#include "PrimeHTMLGenerator.h"

class GenericHTML : public PrimeHTMLGenerator
{
public:
   GenericHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
   void     ServerStatsHeader(sss_t summarizedBy);
};

#endif // #ifndef _GenericHTML_

