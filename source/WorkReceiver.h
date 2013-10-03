#ifndef _WorkReceiver_

#define _WorkReceiver_

#include "defs.h"
#include "server.h"
#include "DBInterface.h"
#include "SQLStatement.h"
#include "Socket.h"
#include "Log.h"
#include "StatsUpdater.h"

#define CT_BAD_TERMINATOR     1
#define CT_WRONG_TERMINATOR   2
#define CT_MISSING_TERMINATOR 3
#define CT_MISSING_DETAIL     4
#define CT_SQL_ERROR          5
#define CT_PARSE_ERROR        6
#define CT_ABANDONED          8
#define CT_SUCCESS            9
#define CT_OBSOLETE           10

class WorkReceiver
{
public:
   WorkReceiver(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                string userID, string emailID, string machineID, 
                string instanceID, string teamID);

   ~WorkReceiver();

   virtual void  ProcessMessage(string theMessage) {};

protected:
   StatsUpdater  *ip_StatsUpdater;
   DBInterface   *ip_DBInterface;
   Socket        *ip_Socket;
   Log           *ip_Log;

   char        is_ClientVersion[10];
   string      is_UserID;
   string      is_EmailID;
   string      is_MachineID;
   string      is_InstanceID;
   string      is_TeamID;
   int32_t     ii_ServerType;
   bool        ib_NeedsDoubleCheck;
   bool        ib_ShowOnWebPage;
   bool        ib_BriefTestLog;

   virtual int32_t  ReceiveWorkUnit(string theMessage) { return 0; };
};

#endif // #ifndef _WorkReceiver_
