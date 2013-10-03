#ifndef _SierpinskiRieselHTML_

#define _SierpinskiRieselHTML_

#include "PrimeHTMLGenerator.h"

class SierpinskiRieselHTML : public PrimeHTMLGenerator
{
public:
   SierpinskiRieselHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _SierpinskiRieselHTML_

