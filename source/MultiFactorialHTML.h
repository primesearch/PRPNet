#ifndef _MultiFactorialHTML_

#define _MultiFactorialHTML_

#include "PrimeHTMLGenerator.h"

class MultiFactorialHTML : public PrimeHTMLGenerator
{
public:
   MultiFactorialHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _MultiFactorialHTML_

