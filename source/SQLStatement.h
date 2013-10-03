#ifndef  _SQLStatement_H_
#define  _SQLStatement_H_

#ifdef WIN32
   #include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include "defs.h"
#include "Log.h"
#include "DBInterface.h"
#define  MAX_PARAMETERS    400

typedef struct
{
   SQLSMALLINT    sqlType;
   char          *charValue;
   uint32_t       ntsLength;
   int32_t        int32Value;
   int64_t        int64Value;
   double         doubleValue;
} sql_param_t;

class SQLStatement
{
public:
   SQLStatement(Log *theLog, DBInterface *dbInterface, string fmt, ...);

   ~SQLStatement(void);

   // Execute a prepated statement
   bool           Execute(void);

   // Open a cursor and fetch a row for a prepared statement
   bool           FetchRow(int32_t fetchAndClose);

   void           CloseCursor(void);

   int32_t        GetRowsAffected(void) { return (int32_t) ii_RowsAffected; };

   int32_t        GetFetchedRowCount(void) { return ii_FetchedRowCount; };

   // Add data fields to be filled in by FetchRow
   void           BindSelectedColumn(char     *ntsParameter, int32_t ntsLength);
   void           BindSelectedColumn(int32_t  *int32Parameter);
   void           BindSelectedColumn(int64_t  *int64Parameter);
   void           BindSelectedColumn(double   *doubleParameter);

   // Bind parameters and values to SQL statements.
   // These must always be called there are input parameters to the SQL statement.
   void           BindInputParameter(string   stringParameter, int32_t maxStringLength, bool hasValue = true);
   void           BindInputParameter(char    *ntsParameter, int32_t maxNtsLength, bool hasValue = true);
   void           BindInputParameter(int32_t  int32Parameter, bool isNull = false);
   void           BindInputParameter(int64_t  int64Parameter, bool isNull = false);
   void           BindInputParameter(double   doubleParameter, bool isNull = false);

   // Set data values for the corresponding parameters on SQL statements.
   // If an SQL statement with bound parameters is executed multiple times, call these
   // functions to update the values bound to the statements.
   void           SetInputParameterValue(string   stringParameter, bool firstParameter = false);
   void           SetInputParameterValue(char    *ntsParameter,    bool firstParameter = false);
   void           SetInputParameterValue(int32_t  int32Parameter,  bool firstParameter = false);
   void           SetInputParameterValue(int64_t  int64Parameter,  bool firstParameter = false);
   void           SetInputParameterValue(double   doubleParameter, bool firstParameter = false);

private:
   DBInterface   *ip_DBInterface;

   SQLHSTMT       ih_StatementHandle;
   SQLSMALLINT    ii_BufferSize;
   SQLCHAR       *is_MessageText;

   Log           *ip_Log;

   const char    *ExpandStatement(void);
   bool           GetSQLErrorAndLog(SQLRETURN returnCode);

   string         is_SQLStatement;
   string         is_ExpandedStatement;
   int32_t        ii_FetchedRowCount;
   int32_t        ii_BoundColumnCount;
   int32_t        ii_BoundInputCount;
   int32_t        ii_SQLParamIndex;
   SQLLEN         ii_LengthIndicator;

   bool           ib_OpenCursor;
   SQLLEN         ii_RowsAffected;

   sql_param_t    ip_SQLParam[MAX_PARAMETERS];
};

#endif
