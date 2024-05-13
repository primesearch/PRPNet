#ifndef _LifchitzHTML_

#define _LifchitzHTML_

#include "PrimeHTMLGenerator.h"

class LifchitzHTML : public PrimeHTMLGenerator
{
public:
   LifchitzHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};


#endif // #ifndef _LifchitzHTML_

