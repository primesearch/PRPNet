#ifndef _GFNHTML_

#define _GFNHTML_

#include "PrimeHTMLGenerator.h"

class GFNHTML : public PrimeHTMLGenerator
{
public:
   GFNHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};


#endif // #ifndef _GFNHTML_

