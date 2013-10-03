#ifndef _FixedBNCHTML_

#define _FixedBNCHTML_

#include "PrimeHTMLGenerator.h"

class FixedBNCHTML : public PrimeHTMLGenerator
{
public:
   FixedBNCHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _FixedBNCHTML_

