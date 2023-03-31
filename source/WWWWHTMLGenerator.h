#ifndef _WWWWHTMLGenerator_

#define _WWWWHTMLGenerator_

#include <string>
#include "HTMLGenerator.h"
#include "ServerHelper.h"
#include "SQLStatement.h"

class WWWWHTMLGenerator : public HTMLGenerator
{
public:
   WWWWHTMLGenerator(globals_t *theGlobals);

   virtual ~WWWWHTMLGenerator() {};

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

   bool          SendSpecificPage(string thePage);
   ServerHelper *GetServerHelper(void);

   bool     CheckIfRecordsWereFound(SQLStatement *sqlStatement, string noRecordsFoundMessage, ...);

   void     SendLinks();

private:
   void     ConvertToScientificNotation(int64_t valueInt, string &valueStr);
};

#endif // #ifndef _WWWWHTMLGenerator_
