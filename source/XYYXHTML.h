#ifndef _XYYXHTML_

#define _XYYXHTML_

#include "PrimeHTMLGenerator.h"

class XYYXHTML : public PrimeHTMLGenerator
{
public:
   XYYXHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};


#endif // #ifndef _XYYXHTML_

