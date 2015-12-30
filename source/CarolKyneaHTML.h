#ifndef _CarolKyneaHTML_

#define _CarolKyneaHTML_

#include "PrimeHTMLGenerator.h"

class CarolKyneaHTML : public PrimeHTMLGenerator
{
public:
   CarolKyneaHTML(globals_t *theGlobals) : PrimeHTMLGenerator(theGlobals) {};

private:
   void     ServerStats(void);
};

#endif // #ifndef _CarolKyneaHTML_

