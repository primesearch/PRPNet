#ifndef _FixedBKCHTML_

#define _FixedBKCHTML_

#include "PrimeHTMLGenerator.h"

class FixedBKCHTML : public PrimeHTMLGenerator
{
public:
   FixedBKCHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _FixedBKCHTML_

