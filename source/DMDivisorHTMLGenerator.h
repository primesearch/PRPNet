#ifndef _DMDivisorHTMLGenerator_

#define _DMDivisorHTMLGenerator_

#include "HTMLGenerator.h"
#include "DMDivisorServerHelper.h"

class DMDivisorHTMLGenerator : public HTMLGenerator
{
public:
   DMDivisorHTMLGenerator(globals_t* theGlobals) : HTMLGenerator(theGlobals) {};

   virtual ~DMDivisorHTMLGenerator() {};

protected:
   bool          SendSpecificPage(string thePage);

   void     PendingTests();
   void     DivisorsFound();

   void     SendLinks(void);

   ServerHelper* GetServerHelper(void) { return new DMDivisorServerHelper(ip_DBInterface, ip_Log); }

private:
   void     ServerStats(void);

   void     ConvertToScientificNotation(int32_t n, int64_t valueInt, string& valueStr);
};


#endif // #ifndef _DMDivisorHTMLGenerator_

