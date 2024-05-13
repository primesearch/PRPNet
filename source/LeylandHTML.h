#ifndef _LeylandHTML_

#define _LeylandHTML_

#include "PrimeHTMLGenerator.h"

class LeylandHTML : public PrimeHTMLGenerator
{
public:
   LeylandHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};


#endif // #ifndef _LeylandHTML_

