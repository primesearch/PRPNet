/* DBInterface Class header for PRPNet. - (C) Mark Rodenkirch, February 2011
*/

#ifndef  _DBInterface_
#define  _DBInterface_

#ifdef WIN32
   #include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include "defs.h"
#include "Log.h"

#define  MAX_PARAMETERS    400

#define  DB_UNKNOWN        0
#define  DB_MYSQL          1
#define  DB_POSTGRESQL     2

class DBInterface
{
public:
   DBInterface(Log *theLog, string iniFileName);

   ~DBInterface(void);

   void           ProcessIniFile(string iniFileName);
   int32_t        Initialize(void);
   int32_t        IsConnected(void) { return ib_connected; };
   void           Disconnect(void);
   int32_t        Connect(int32_t clientID);

   int32_t        GetClientID(void) { return ii_ClientID; };
   void           Commit(void);
   void           Rollback(void);

   SQLHDBC        GetConnectionHandle(void) { return ip_SQLConnectionHandle; };

   int32_t        GetDBVendor(void) { return ii_Vendor; };

private:
   SQLHENV        ip_SQLEnvironmentHandle;
   SQLHDBC        ip_SQLConnectionHandle;
   SQLHSTMT       ip_SQLStatementHandle;
   SQLSMALLINT    ii_SQLBufferSize;
   SQLCHAR       *ic_SQLMessageText;

   int32_t        ib_initialized;
   int32_t        ib_connected;
   int32_t        ii_ClientID;
   int32_t        ii_Vendor;

   Log           *ip_Log;

   string         is_DriverName;
   string         is_DSN;
   string         is_ServerIP;
   int32_t        ii_ServerPort;
   string         is_DatabaseName;
   string         is_UserName;
   string         is_Password;

   int32_t        GetSQLErrorAndLog(SQLRETURN returnCode);
};

#endif
