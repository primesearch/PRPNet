#ifndef _HTMLGeneratorFactory_

#define _HTMLGeneratorFactory_

#include "defs.h"
#include "HTMLGenerator.h"
#include "DBInterface.h"

class HTMLGeneratorFactory
{
public:
   static HTMLGenerator   *GetHTMLGenerator(globals_t *globals);
};

#endif // #ifndef _HTMLGeneratorFactory_
