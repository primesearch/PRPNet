/* DBInterface Class for PRPNet. - (C) Mark Rodenkirch, February 2011
*/

#include <sstream>
#include <stdarg.h>

#include "DBInterface.h"

#define  PARAM_TYPE_STRING          1
#define  PARAM_TYPE_INTEGER32       2
#define  PARAM_TYPE_INTEGER64       3
#define  PARAM_TYPE_DOUBLE          4

// Constructor
DBInterface::DBInterface(Log *theLog, string iniFileName)
{
   ip_Log = theLog;

   is_DriverName.clear();
   is_DSN.clear();
   is_ServerIP.clear();
   ii_ServerPort = 0;
   is_DatabaseName.clear();
   is_UserName.clear();
   is_Password.clear();

   ProcessIniFile(iniFileName);

   ip_SQLEnvironmentHandle = 0;
   ip_SQLConnectionHandle = 0;
   ip_SQLStatementHandle = 0;

   ib_initialized = false;
   ib_connected = false;
   ii_ClientID = 0;

   ii_SQLBufferSize = 5000;
   ic_SQLMessageText = (SQLCHAR *) new char[ii_SQLBufferSize + 1];
}

// Destructor
DBInterface::~DBInterface(void)
{
   if (ib_connected)
      Disconnect();

   delete ic_SQLMessageText;
}

// Initialize the variables needed so that this application can connect to a
// database through an ODBC connection
int32_t  DBInterface::Initialize(void)
{
   int   rCode;

   if (ib_initialized)
      return ib_initialized;

   // Allocate and intialize memory for the environment handle
   rCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &ip_SQLEnvironmentHandle);

   if (rCode != SQL_SUCCESS)
      return ib_initialized;

   // Set the ODBC version
   rCode = SQLSetEnvAttr(ip_SQLEnvironmentHandle, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);

   if (rCode != SQL_SUCCESS)
      return ib_initialized;

   // Allocate and intialize memory for the connection handle
   rCode = SQLAllocHandle(SQL_HANDLE_DBC, ip_SQLEnvironmentHandle, &ip_SQLConnectionHandle);

   if (rCode != SQL_SUCCESS)
      return ib_initialized;

   ib_initialized = true;

   return ib_initialized;
}

void     DBInterface::Disconnect(void)
{
   if (ib_connected)
   {
      SQLFreeHandle(SQL_HANDLE_STMT, ip_SQLStatementHandle);
      SQLDisconnect(ip_SQLConnectionHandle);

      ib_connected = false;
      ip_Log->Debug(DEBUG_DATABASE, "%d: disconnected from database", ii_ClientID);
   }

   if (ib_initialized)
   {
      SQLFreeHandle(SQL_HANDLE_DBC, ip_SQLConnectionHandle);
      SQLFreeHandle(SQL_HANDLE_ENV, ip_SQLEnvironmentHandle);

      ib_initialized = false;
   }
}

// Connect to the specified database
int32_t  DBInterface::Connect(int32_t clientID)
{
   int         rCode;
   char        temp[512];
   char        dbmsName[100];
   SQLSMALLINT size;
   SQLCHAR     sqlState[6];
   SQLINTEGER  nativeCode;
   SQLCHAR     msgText[1001];
   SQLSMALLINT retMsgLen;
   string      connectString;
   stringstream   port;
   int   dbConnectTimeout = 20;

   if (!ib_initialized)
      Initialize();

   if (ib_connected)
      return ib_connected;

   ii_ClientID = clientID;

   port << ii_ServerPort;

   int caCode;

   caCode = SQLSetConnectAttr(ip_SQLConnectionHandle, SQL_ATTR_CONNECTION_TIMEOUT, (SQLPOINTER) &dbConnectTimeout, 0);
   if (caCode != SQL_SUCCESS) GetSQLErrorAndLog(caCode);
   
   if (is_DSN.length())
   {
      ip_Log->Debug(DEBUG_DATABASE, "%d: ODBC Connection via a DSN.  DSN=%s, User=%s, Password=%s",
                    ii_ClientID, is_DSN.c_str(), is_UserName.c_str(), is_Password.c_str());

      rCode = SQLConnect(ip_SQLConnectionHandle,
                         (SQLCHAR *) is_DSN.c_str(), SQL_NTS,
                         (SQLCHAR *) is_UserName.c_str(), SQL_NTS,
                         (SQLCHAR *) is_Password.c_str(), SQL_NTS);
   }
   else
   {
      connectString  = "Driver={" + is_DriverName + "};";
      connectString += "Server=" + is_ServerIP + ";";
      connectString += "Port=" + port.str() + ";";
      connectString += "Database=" + is_DatabaseName + ";";
      connectString += "User=" + is_UserName + ";";
      connectString += "UID=" + is_UserName + ";";
      if (is_Password.length() > 0)
      {
         connectString += "Password=" + is_Password + ";";
         connectString += "PWD=" + is_Password + ";";
      }
      connectString += "Option=3;";

      ip_Log->Debug(DEBUG_DATABASE, "%d: ODBC Connection via a driver: <%s>", ii_ClientID, connectString.c_str());

      rCode = SQLDriverConnect(ip_SQLConnectionHandle, NULL, (SQLCHAR *) connectString.c_str(),
                               (SQLSMALLINT) connectString.length(),
                               (unsigned char *) temp, 511, &size, SQL_DRIVER_NOPROMPT);
   }

   if (rCode == SQL_SUCCESS || rCode == SQL_SUCCESS_WITH_INFO)
   {
      caCode = SQLSetConnectAttr(ip_SQLConnectionHandle, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER) SQL_AUTOCOMMIT_OFF, SQL_NTS);
      if (caCode != SQL_SUCCESS) GetSQLErrorAndLog(caCode);

      caCode = SQLSetConnectAttr(ip_SQLConnectionHandle, SQL_ATTR_TXN_ISOLATION, (SQLPOINTER) SQL_TXN_REPEATABLE_READ, SQL_NTS);
      if (caCode != SQL_SUCCESS) GetSQLErrorAndLog(caCode);

      SQLAllocHandle(SQL_HANDLE_STMT, ip_SQLConnectionHandle, &ip_SQLStatementHandle);

      ib_connected = true;

      ii_Vendor = DB_UNKNOWN;
      strcpy(dbmsName, "mystery");

      rCode = SQLGetInfo(ip_SQLConnectionHandle, SQL_DBMS_NAME, dbmsName, sizeof(dbmsName), &size);
      if (rCode == SQL_SUCCESS)
      {
         if (!strcmp(dbmsName, "MySQL"))
            ii_Vendor = DB_MYSQL;
         if (!strcmp(dbmsName, "PostgreSQL"))
            ii_Vendor = DB_POSTGRESQL;
      }

      if (ii_Vendor == DB_UNKNOWN)
      {
         ip_Log->LogMessage("PRPNet does not support %s databases", dbmsName);
         exit(0);
      }

      ip_Log->Debug(DEBUG_DATABASE, "%d: successfully connected to a %s database", ii_ClientID, dbmsName);
   }
   else
   {
      SQLGetDiagRec(SQL_HANDLE_DBC, ip_SQLConnectionHandle, 1, sqlState, &nativeCode, msgText, 1000, &retMsgLen);

      ip_Log->LogMessage("Connect to database failed: %s, native code=%d", msgText, nativeCode);
   }

   return ib_connected;
}

// Parse the configuration file to determine how we are to connect to the database
void     DBInterface::ProcessIniFile(string iniFileName)
{
   FILE *fp;
   char  line[201];

   if ((fp = fopen(iniFileName.c_str(), "r")) == NULL)
   {
      printf("Database ini file '%s' was not found or could not be opened", iniFileName.c_str());
      return;
   }

   while (fgets(line, 200, fp) != NULL)
   {
      StripCRLF(line);

      if (!memcmp(line, "driver=", 7))
         is_DriverName = line+7;
      else if (!memcmp(line, "dsn=", 4))
         is_DSN = line+4;
      else if (!memcmp(line, "server=", 7))
         is_ServerIP = line+7;
      else if (!memcmp(line, "port=", 5))
         ii_ServerPort = atol(line+5);
      else if (!memcmp(line, "database=", 9))
         is_DatabaseName = line+9;
      else if (!memcmp(line, "user=", 5))
         is_UserName = line+5;
      else if (!memcmp(line, "password=", 9))
         is_Password = line+9;
   }

   fclose(fp);
}

void     DBInterface::Commit(void)
{
   SQLRETURN   sqlReturnCode;

   // For some reason SQLEndTran returns a 0, but doesn't really commit the transaction.
   // We'll do it the "hard way" because we know it works.
//#ifndef WIN32
   sqlReturnCode = SQLEndTran(SQL_HANDLE_ENV, ip_SQLEnvironmentHandle, SQL_COMMIT);
//#else
//   sqlReturnCode = SQLExecDirect(ip_SQLStatementHandle, (SQLCHAR *) "COMMIT;", SQL_NTS);
//#endif

   if (sqlReturnCode != SQL_SUCCESS)
      GetSQLErrorAndLog(sqlReturnCode);
}

void     DBInterface::Rollback(void)
{
   SQLRETURN   sqlReturnCode;

   // For some reason SQLEndTran returns a 0, but doesn't really rollback the transaction.
   // We'll do it the "hard way" because we know it works.
//#ifndef WIN32
   sqlReturnCode = SQLEndTran(SQL_HANDLE_ENV, ip_SQLEnvironmentHandle, SQL_ROLLBACK);
//#else
//   sqlReturnCode = SQLExecDirect(ip_SQLStatementHandle, (SQLCHAR *) "ROLLBACK;", SQL_NTS);
//#endif

   if (sqlReturnCode != SQL_SUCCESS)
      GetSQLErrorAndLog(sqlReturnCode);
}

int32_t  DBInterface::GetSQLErrorAndLog(SQLRETURN returnCode)
{
   const char       *errorType;
   const char       *severityLevel;
   int32_t     canContinue;
   SQLSMALLINT returnLength;
   SQLINTEGER  sqlNativeErrorCode;
   SQLCHAR     sqlState[6];
   SQLRETURN   sqlReturnCode;

// SQLRETURN SQLGetDiagRec(SQLSMALLINT    HandleType,
//                         SQLHANDLE      Handle,
//                         SQLSMALLINT    RecNumber,
//                         SQLCHAR       *SQLState,
//                         SQLINTEGER    *NativeErrorPtr,
//                         SQLCHAR       *MessageText,
//                         SQLSMALLINT    BufferLength,
//                         SQLSMALLINT   *TextLengthPtr);

   ic_SQLMessageText[0] = 0;

   canContinue = false;

   switch (returnCode)
   {
      case SQL_SUCCESS_WITH_INFO:
         severityLevel = "WARNING";
         errorType = "SQL_SUCCESS_WITH_INFO";
         canContinue = true;
         break;

      case SQL_ERROR:
         severityLevel = "ERROR";
         errorType = "SQL_ERROR";

         sqlReturnCode = SQLGetDiagRec(SQL_HANDLE_DBC,
                                       ip_SQLConnectionHandle,
                                       1,
                                       sqlState,
                                       &sqlNativeErrorCode,
                                       ic_SQLMessageText,
                                       ii_SQLBufferSize,
                                       &returnLength);
         break;

      case SQL_NEED_DATA:
         severityLevel = "ERROR";
         errorType = "SQL_NEED_DATA";
         break;

      case SQL_INVALID_HANDLE:
         severityLevel = "ERROR";
         errorType = "SQL_INVALID_HANDLE";
         break;

      default:
         severityLevel = "ERROR";
         errorType = "SQL_default";
         break;
   }

   ip_Log->LogMessage("%d: ODBC Information: %s: %s", ii_ClientID, errorType, ic_SQLMessageText);

   return canContinue;
}
