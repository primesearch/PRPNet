#ifndef _GFNDivisorHTMLGenerator_

#define _GFNDivisorHTMLGenerator_

#include "HTMLGenerator.h"
#include "GFNDivisorServerHelper.h"

class GFNDivisorHTMLGenerator : public HTMLGenerator
{
public:
   GFNDivisorHTMLGenerator(globals_t* theGlobals) : HTMLGenerator(theGlobals) {};

   virtual ~GFNDivisorHTMLGenerator() {};

protected:
   bool          SendSpecificPage(string thePage);

   void     PendingTests();
   void     DivisorsFound();

   void     SendLinks(void);

   ServerHelper* GetServerHelper(void) { return new GFNDivisorServerHelper(ip_DBInterface, ip_Log); }

private:
   void     ServerStats(void);

   void     ConvertToScientificNotation(int64_t valueInt, string& valueStr);
};


#endif // #ifndef _GFNDivisorHTMLGenerator_

