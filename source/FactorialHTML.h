#ifndef _FactorialHTML_

#define _FactorialHTML_

#include "PrimeHTMLGenerator.h"

class FactorialHTML : public PrimeHTMLGenerator
{
public:
   FactorialHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _FactorialHTML_

