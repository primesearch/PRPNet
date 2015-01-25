#ifndef _CyclotomicHTML_

#define _CyclotomicHTML_

#include "PrimeHTMLGenerator.h"

class CyclotomicHTML : public PrimeHTMLGenerator
{
public:
   CyclotomicHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _CyclotomicHTML_

