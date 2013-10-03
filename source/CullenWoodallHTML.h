#ifndef _CullenWoodallHTML_

#define _CullenWoodallHTML_

#include "PrimeHTMLGenerator.h"

class CullenWoodallHTML : public PrimeHTMLGenerator
{
public:
   CullenWoodallHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _CullenWoodallHTML_

