#ifndef _HTMLGenerator_

#define _HTMLGenerator_

#include <string>
#include "defs.h"
#include "server.h"
#include "DBInterface.h"
#include "ServerHelper.h"
#include "Socket.h"
#include "SQLStatement.h"
#include "Log.h"

#define  TD_CHAR(x) ip_Socket->Send("<td>%s</td>", (x))
#define  TD_TIME(h, m) ip_Socket->Send("<td class=\"time\">%d:%02d</td>", h, m)
#define  TD_32BIT(x) ip_Socket->Send("<td class=\"number\">%d</td>", (x))
#define  TD_IF_DC(x) if (ib_NeedsDoubleCheck) ip_Socket->Send("<td class=\"number\">%d</td>", (x))
#define  TD_64BIT(x) ip_Socket->Send("<td class=\"number\">%" PRId64"</td>", (x))
#define  TD_FLOAT(x) ip_Socket->Send("<td class=\"number\">%.0lf</td>", (x))

#define  TH_CLMN_HDR(x) ip_Socket->Send("<th scope=\"col\">%s</th>", x)
#define  TH_CH_IF_DC(x) if (ib_NeedsDoubleCheck) ip_Socket->Send("<th scope=\"col\">%s</th>", x)
#define  TH_ROW_HDR(x) ip_Socket->Send("<th scope=\"row\">%s</th>", x)

#define  PLURAL_ENDING(count) ((count) == 1) ? "" : "s"
#define  PLURAL_COPULA(count) ((count) == 1) ? "is" : "are"

class HTMLGenerator
{
public:
   HTMLGenerator(globals_t *theGlobals);

   virtual ~HTMLGenerator() {};

   void     SetDBInterface(DBInterface *dbInterface) { ip_DBInterface = dbInterface; };
   void     SetLog(Log *theLog) { ip_Log = theLog; };
   void     SetSocket(Socket *theSocket) { ip_Socket = theSocket; };

   bool     HasTeams(void);

   void                  Send(string thePage);

   virtual void          ServerStats(void) {};
   virtual void          UserStats(void) {};
   virtual void          UserTeamStats(void) {};
   virtual void          TeamUserStats(void) {};
   virtual void          TeamStats(void) {};

protected:
   int32_t        ii_ServerType;
   int32_t        ib_NeedsDoubleCheck;
   string         is_HTMLTitle;
   string         is_ProjectTitle;
   string         is_CSSLink;
   string         is_SortLink;
   string         is_AllPrimesSortSequence;
   DBInterface   *ip_DBInterface;
   Log           *ip_Log;
   Socket        *ip_Socket;

   bool           ib_UseLocalTime;
   bool           ib_ServerStatsSummaryOnly;
   bool           ib_ShowTeamsOnHtml;

   int32_t        ii_DelayCount;
   delay_t       *ip_Delay;

   virtual bool   SendSpecificPage(string thePage) { return false; };
   void           GetDaysLeft(void);
   bool           CheckIfRecordsWereFound(SQLStatement *sqlStatement, string noRecordsFoundMessage);
   string         TimeToString(time_t theTime);
   void           HeaderPlusLinks(string pageTitle);
   virtual void   SendLinks() {};

   virtual ServerHelper *GetServerHelper(void) { return NULL; };

private:
   void           SendHeader(string pageTitle);
};

#endif // #ifndef _HTMLGenerator_
