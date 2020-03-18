#ifndef _HTMLGenerator_

#define _HTMLGenerator_

#include <string>
#include "defs.h"
#include "server.h"
#include "DBInterface.h"
#include "Socket.h"
#include "Log.h"

#define  TD_CHAR(x) ip_Socket->Send("<td>%s", (x))
#define  TD_TIME(h, m) ip_Socket->Send("<td align=center>%d:%02d", h, m)
#define  TD_32BIT(x) ip_Socket->Send("<td align=right>%d", (x))
#define  TD_IF_DC(x) if (ib_NeedsDoubleCheck) ip_Socket->Send("<td align=right>%d", (x))
#define  TD_64BIT(x) ip_Socket->Send("<td align=right>%" PRId64"", (x))
#define  TD_FLOAT(x) ip_Socket->Send("<td align=right>%.0lf", (x))

#define  TH_32BIT(x) ip_Socket->Send("<th align=right>%d", (x))
#define  TH_IF_DC(x) if (ib_NeedsDoubleCheck) ip_Socket->Send("<th align=right>%d", (x))
#define  TH_64BIT(x) ip_Socket->Send("<th align=right>%" PRId64"", (x))

#define  TH_CLMN_HDR(x) ip_Socket->Send("<th class=headertext>%s", x)
#define  TH_CH_IF_DC(x) if (ib_NeedsDoubleCheck) ip_Socket->Send("<th class=headertext>%s", x)

class HTMLGenerator
{
public:
   HTMLGenerator(globals_t *theGlobals);

   virtual ~HTMLGenerator() {};

   void     SetDBInterface(DBInterface *dbInterface) { ip_DBInterface = dbInterface; };
   void     SetLog(Log *theLog) { ip_Log = theLog; };
   void     SetSocket(Socket *theSocket) { ip_Socket = theSocket; };

   virtual void     Send(string thePage) {};

   virtual void  ServerStats(void) {};
   virtual void  UserStats(void) {};
   virtual void  UserTeamStats(void) {};
   virtual void  TeamUserStats(void) {};
   virtual void  TeamStats(void) {};

protected:
   int32_t        ii_ServerType;
   int32_t        ib_NeedsDoubleCheck;
   string         is_HTMLTitle;
   string         is_ProjectTitle;
   string         is_CSSLink;
   string         is_SortLink;
   DBInterface   *ip_DBInterface;
   Log           *ip_Log;
   Socket        *ip_Socket;

   bool           ib_UseLocalTime;
   bool           ib_ServerStatsSummaryOnly;

   int32_t        ii_DelayCount;
   delay_t       *ip_Delay;
};

#endif // #ifndef _HTMLGenerator_
