#ifndef _HyperCullenWoodallHTML_

#define _HyperCullenWoodallHTML_

#include "PrimeHTMLGenerator.h"

class HyperCullenWoodallHTML : public PrimeHTMLGenerator
{
public:
   HyperCullenWoodallHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};


#endif // #ifndef _HyperCullenWoodallHTML_

