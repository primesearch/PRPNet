#ifndef _WWWWHTMLGenerator_

#define _WWWWHTMLGenerator_

#include <string>
#include "HTMLGenerator.h"

class WWWWHTMLGenerator : public HTMLGenerator
{
public:
   WWWWHTMLGenerator(globals_t *theGlobals) : HTMLGenerator(theGlobals) {};

   virtual ~WWWWHTMLGenerator() {};

   void     Send(string thePage);

   void     PendingTests(void);
   void     FindsByUser(void);
   void     FindsByTeam(void);
   void     ServerStats(void);
   void     UserStats(void);
   void     UserTeamStats(void);
   void     TeamUserStats(void);
   void     TeamStats(void);

protected:

   char     ic_SearchType[20];

private:
   char    *TimeToString(time_t theTime);
   void     ConvertToScientificNotation(int64_t valueInt, string &valueStr);
   void     HeaderPlusLinks(string pageTitle);
   void     GetDaysLeft(void);
};

#endif // #ifndef _WWWWHTMLGenerator_
