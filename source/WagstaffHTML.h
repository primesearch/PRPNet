#ifndef _WagstaffHTML_

#define _WagstaffHTML_

#include "PrimeHTMLGenerator.h"

class WagstaffHTML : public PrimeHTMLGenerator
{
public:
   WagstaffHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _WagstaffHTML_

