#ifndef _PrimorialHTML_

#define _PrimorialHTML_

#include "PrimeHTMLGenerator.h"

class PrimorialHTML : public PrimeHTMLGenerator
{
public:
   PrimorialHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _PrimorialHTML_

