#ifndef _SophieGermainHTML_

#define _SophieGermainHTML_

#include "PrimeHTMLGenerator.h"

class SophieGermainHTML : public PrimeHTMLGenerator
{
public:
   SophieGermainHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _SophieGermainHTML_

