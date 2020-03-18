
#include <sstream>
#include <stdarg.h>

#include "SQLStatement.h"

#define  PARAM_TYPE_STRING          1
#define  PARAM_TYPE_INTEGER32       2
#define  PARAM_TYPE_INTEGER64       3
#define  PARAM_TYPE_DOUBLE          4

// Constructor
SQLStatement::SQLStatement(Log *theLog, DBInterface *dbInterface, string fmt, ...)
{
   char       *theSQL;
   va_list     args;
   SQLRETURN   sqlReturnCode;
   size_t      foundPos;

   ip_Log = theLog;
   ip_DBInterface = dbInterface;

   SQLAllocHandle(SQL_HANDLE_STMT, ip_DBInterface->GetConnectionHandle(), &ih_StatementHandle);

   ii_BufferSize = 5000;
   is_MessageText = (SQLCHAR *) new char[ii_BufferSize + 1];

   ii_SQLParamIndex = 0;
   ii_FetchedRowCount = 0;
   ii_BoundColumnCount = 0;
   ii_BoundInputCount = 0;
   ii_LengthIndicator = 0;

   theSQL = new char[10000];

   va_start(args, fmt);
   vsprintf(theSQL, fmt.c_str(), args);
   va_end(args);

   is_SQLStatement = theSQL;

   foundPos = is_SQLStatement.rfind("$null_func$");
   while (foundPos != string::npos)
   {
      if (ip_DBInterface->GetDBVendor() == DB_MYSQL)
         is_SQLStatement.replace(foundPos, 11, "ifnull");
      if (ip_DBInterface->GetDBVendor() == DB_POSTGRESQL)
         is_SQLStatement.replace(foundPos, 11, "coalesce");

      foundPos = is_SQLStatement.rfind("$null_func$");
   }

   sqlReturnCode = SQLPrepare(ih_StatementHandle, (SQLCHAR *) is_SQLStatement.c_str(), SQL_NTS);

   if (sqlReturnCode != SQL_SUCCESS)
   {
      if (!GetSQLErrorAndLog(sqlReturnCode))
         exit(0);
   }

   ib_OpenCursor = false;
   delete [] theSQL;
}

// Destructor
SQLStatement::~SQLStatement(void)
{
   int32_t  ii;

   CloseCursor();

   SQLFreeHandle(SQL_HANDLE_STMT, ih_StatementHandle);

   for (ii=0; ii<ii_BoundInputCount; ii++)
   {
      if (ip_SQLParam[ii].sqlType == SQL_C_CHAR)
         delete [] ip_SQLParam[ii].charValue;
   }

   delete [] is_MessageText;
}

void     SQLStatement::CloseCursor(void)
{
   ib_OpenCursor = false;

   SQLFreeStmt(ih_StatementHandle, SQL_CLOSE);
}

//****************************************************************************************************//
// Execute an INSERT/UPDATE/DELETE statement that has bound variables
//****************************************************************************************************//
bool     SQLStatement::Execute(void)
{
   int32_t     canContinue = true;
   SQLRETURN   sqlReturnCode;

   ii_RowsAffected = 0;

   ip_Log->Debug(DEBUG_DATABASE, "%d: Executing statement: <%s>", ip_DBInterface->GetClientID(), ExpandStatement());

   sqlReturnCode = SQLExecute(ih_StatementHandle);

   if (sqlReturnCode != SQL_SUCCESS)
      canContinue = GetSQLErrorAndLog(sqlReturnCode);
   else
   {
      SQLRowCount(ih_StatementHandle, &ii_RowsAffected);
      ip_Log->Debug(DEBUG_DATABASE, "%d: %d rows affected", ip_DBInterface->GetClientID(), ii_RowsAffected);
   }

   CloseCursor();

   if (!canContinue)
      return false;

   return true;
}

//****************************************************************************************************//
// Fetch records from a cursor
//****************************************************************************************************//
bool     SQLStatement::FetchRow(int32_t fetchAndClose)
{
   int32_t     canContinue = true;
   SQLRETURN   sqlReturnCode;

   if (!ib_OpenCursor)
   {
      ip_Log->Debug(DEBUG_DATABASE, "%d: Executing statement: <%s>", ip_DBInterface->GetClientID(), ExpandStatement());

      sqlReturnCode = SQLExecute(ih_StatementHandle);

      if (sqlReturnCode != SQL_SUCCESS)
         canContinue = GetSQLErrorAndLog(sqlReturnCode);

      ii_SQLParamIndex = 0;
      ii_FetchedRowCount = 0;

      if (!canContinue)
         return false;

      ib_OpenCursor = true;
   }

   sqlReturnCode = SQLFetch(ih_StatementHandle);

   // Close the cursor automatically if there is no more
   // data or the routine is instructed to close immediately
   if (sqlReturnCode == 100 || fetchAndClose)
      CloseCursor();

   if (sqlReturnCode == 100)
   {
      ip_Log->Debug(DEBUG_DATABASE, "%d: No more records found", ip_DBInterface->GetClientID());
      return false;
   }

   ii_FetchedRowCount++;
   ip_Log->Debug(DEBUG_DATABASE, "%d: %d records fetched", ip_DBInterface->GetClientID(), ii_FetchedRowCount);

   if (sqlReturnCode != SQL_SUCCESS)
      canContinue = GetSQLErrorAndLog(sqlReturnCode);

   if (!canContinue)
      return false;

   return true;
}

//****************************************************************************************************//
// Replace the "?" in the SQL statement with the value bound to it.  This will help with tracing.
//****************************************************************************************************//
const char  *SQLStatement::ExpandStatement(void)
{
   int      parameterIndex;
   size_t   parameterPosition;
   char    *substitutedValue, temp[50];

   is_ExpandedStatement = is_SQLStatement;
   parameterIndex = 0;
   parameterPosition = is_ExpandedStatement.find("?");
   while (parameterPosition != string::npos)
   {
      if (parameterIndex == ii_SQLParamIndex)
      {
         ip_Log->LogMessage("Fatal error.  Not enough bound variables for <%s>", is_SQLStatement.c_str());
         exit(0);
      }

      switch (ip_SQLParam[parameterIndex].sqlType)
      {
         case SQL_C_CHAR:
            sprintf(temp, "\'%s\'", ip_SQLParam[parameterIndex].charValue);
            substitutedValue = temp;
            break;
         case SQL_C_LONG:
            sprintf(temp, "%d", ip_SQLParam[parameterIndex].int32Value);
            substitutedValue = temp;
            break;
         case SQL_C_SBIGINT:
            sprintf(temp, "%" PRId64"", ip_SQLParam[parameterIndex].int64Value);
            substitutedValue = temp;
            break;
         case SQL_C_DOUBLE:
            sprintf(temp, "%lf", ip_SQLParam[parameterIndex].doubleValue);
            substitutedValue = temp;
            break;
         default:
            ip_Log->LogMessage("Fatal error.  Unhandled SQL Type %u", ip_SQLParam[parameterIndex].sqlType);
            exit(0);
      }

      is_ExpandedStatement.replace(parameterPosition, 1, substitutedValue);
      parameterPosition = is_ExpandedStatement.find("?");
      parameterIndex++;
   }

   if (parameterIndex != ii_SQLParamIndex)
      ip_Log->LogMessage("Warning.  Too many bound variables for <%s>", is_SQLStatement.c_str());

   return is_ExpandedStatement.c_str();
}

//****************************************************************************************************//
// Various routines to bind selected columns and their target fields
//****************************************************************************************************//
void     SQLStatement::BindSelectedColumn(char     *ntsParameter, int32_t ntsLength)
{
   ii_BoundColumnCount++;

   SQLBindCol(ih_StatementHandle,
              ii_BoundColumnCount,
              SQL_C_CHAR,
              ntsParameter,
              ntsLength - 1,
             &ii_LengthIndicator);
}

void     SQLStatement::BindSelectedColumn(int32_t  *int32Parameter)
{
   ii_BoundColumnCount++;

   SQLBindCol(ih_StatementHandle,
              ii_BoundColumnCount,
              SQL_INTEGER,
              int32Parameter,
              sizeof(int32_t),
             &ii_LengthIndicator);
}

void     SQLStatement::BindSelectedColumn(int64_t  *int64Parameter)
{
   ii_BoundColumnCount++;

   SQLBindCol(ih_StatementHandle,
              ii_BoundColumnCount,
              SQL_C_SBIGINT,
              int64Parameter,
              sizeof(int64_t),
             &ii_LengthIndicator);
}

void     SQLStatement::BindSelectedColumn(double   *doubleParameter)
{
   ii_BoundColumnCount++;

   SQLBindCol(ih_StatementHandle,
              ii_BoundColumnCount,
              SQL_C_DOUBLE,
              doubleParameter,
              sizeof(double),
             &ii_LengthIndicator);
}

//****************************************************************************************************//
// Various routines to bind SQL parameters to data
//****************************************************************************************************//
void     SQLStatement::BindInputParameter(string stringParameter, int32_t maxStringLength, bool hasValue)
{
   BindInputParameter((char *) stringParameter.c_str(), maxStringLength, hasValue);
}

void     SQLStatement::BindInputParameter(char     *ntsParameter, int32_t maxNtsLength, bool hasValue)
{
   SQLRETURN   sqlReturnCode;
   int32_t     length;
#ifdef WIN32
   SQLLEN      nullValue = SQL_NULL_DATA;
#else
   long int    nullValue = SQL_NULL_DATA;
#endif

   if (!hasValue) *ntsParameter = 0;

   length = (int32_t) (maxNtsLength ? maxNtsLength : strlen(ntsParameter));

   ip_SQLParam[ii_SQLParamIndex].sqlType = SQL_C_CHAR;
   ip_SQLParam[ii_SQLParamIndex].charValue = new char[length+1];
   ip_SQLParam[ii_SQLParamIndex].ntsLength = length+1;

   sqlReturnCode = SQLBindParameter(ih_StatementHandle,
                                    ++ii_BoundInputCount,
                                    SQL_PARAM_INPUT,
                                    SQL_C_CHAR,
                                    SQL_VARCHAR,
                                    length,
                                    0,
                                    ((length == 0) ? 0 : ip_SQLParam[ii_SQLParamIndex].charValue),
                                    length,
                                    ((length == 0) ? &nullValue : 0));

   if (sqlReturnCode != SQL_SUCCESS)
      GetSQLErrorAndLog(sqlReturnCode);

   SetInputParameterValue(ntsParameter);
}

void     SQLStatement::BindInputParameter(int32_t int32Parameter, bool isNull)
{
   SQLRETURN   sqlReturnCode;
#ifdef WIN32
   SQLLEN      nullValue = SQL_NULL_DATA;
#else
   long int    nullValue = SQL_NULL_DATA;
#endif

   ip_SQLParam[ii_SQLParamIndex].sqlType = SQL_C_LONG;
   ip_SQLParam[ii_SQLParamIndex].int32Value = int32Parameter;

   sqlReturnCode = SQLBindParameter(ih_StatementHandle,
                                    ++ii_BoundInputCount,
                                    SQL_PARAM_INPUT,
                                    SQL_C_LONG,
                                    SQL_INTEGER,
                                    0,
                                    0,
                                    (isNull ? 0 : &ip_SQLParam[ii_SQLParamIndex].int32Value),
                                    0,
                                    (isNull ? &nullValue : 0));

   if (sqlReturnCode != SQL_SUCCESS)
      GetSQLErrorAndLog(sqlReturnCode);

   SetInputParameterValue(int32Parameter);
}

void     SQLStatement::BindInputParameter(int64_t int64Parameter, bool isNull)
{
   SQLRETURN   sqlReturnCode;
#ifdef WIN32
   SQLLEN      nullValue = SQL_NULL_DATA;
#else
   long int    nullValue = SQL_NULL_DATA;
#endif

   ip_SQLParam[ii_SQLParamIndex].sqlType = SQL_C_SBIGINT;
   ip_SQLParam[ii_SQLParamIndex].int64Value = int64Parameter;

   sqlReturnCode = SQLBindParameter(ih_StatementHandle,
                                    ++ii_BoundInputCount,
                                    SQL_PARAM_INPUT,
                                    SQL_C_SBIGINT,
                                    SQL_BIGINT,
                                    0,
                                    0,
                                    (isNull ? 0 : &ip_SQLParam[ii_SQLParamIndex].int64Value),
                                    0,
                                    (isNull ? &nullValue : 0));

   if (sqlReturnCode != SQL_SUCCESS)
      GetSQLErrorAndLog(sqlReturnCode);

   SetInputParameterValue(int64Parameter);
}

void     SQLStatement::BindInputParameter(double doubleParameter, bool isNull)
{
   SQLRETURN   sqlReturnCode;
#ifdef WIN32
   SQLLEN      nullValue = SQL_NULL_DATA;
#else
   long int    nullValue = SQL_NULL_DATA;
#endif

   ip_SQLParam[ii_SQLParamIndex].sqlType = SQL_C_DOUBLE;
   ip_SQLParam[ii_SQLParamIndex].doubleValue = doubleParameter;

   sqlReturnCode = SQLBindParameter(ih_StatementHandle,
                                    ++ii_BoundInputCount,
                                    SQL_PARAM_INPUT,
                                    SQL_C_DOUBLE,
                                    SQL_DOUBLE,
                                    0,
                                    0,
                                    (isNull ? 0 : &ip_SQLParam[ii_SQLParamIndex].doubleValue),
                                    0,
                                    (isNull ? &nullValue : 0));

   if (sqlReturnCode != SQL_SUCCESS)
      GetSQLErrorAndLog(sqlReturnCode);

   SetInputParameterValue(doubleParameter);
}

//****************************************************************************************************//
// Various routines to set the parameter values for bound data
//****************************************************************************************************//
void     SQLStatement::SetInputParameterValue(string stringParameter, bool firstParameter)
{
   SetInputParameterValue((char *) stringParameter.c_str(), firstParameter);
}

void     SQLStatement::SetInputParameterValue(char *ntsParameter, bool firstParameter)
{
   if (firstParameter) ii_SQLParamIndex = 0;

   if (strlen(ntsParameter) > ip_SQLParam[ii_SQLParamIndex].ntsLength - 1)
   {
      ip_Log->LogMessage("%d: SQL Parameter too long for statement %s",
                         ip_DBInterface->GetClientID(), is_SQLStatement.c_str());
      exit(0);
   }

   strcpy(ip_SQLParam[ii_SQLParamIndex].charValue, ntsParameter);
   ii_SQLParamIndex++;
}

void     SQLStatement::SetInputParameterValue(int32_t int32Parameter, bool firstParameter)
{
   if (firstParameter) ii_SQLParamIndex = 0;

   ip_SQLParam[ii_SQLParamIndex].int32Value = int32Parameter;
   ii_SQLParamIndex++;
}

void     SQLStatement::SetInputParameterValue(int64_t int64Parameter, bool firstParameter)
{
   if (firstParameter) ii_SQLParamIndex = 0;

   ip_SQLParam[ii_SQLParamIndex].int64Value = int64Parameter;
   ii_SQLParamIndex++;
}

void     SQLStatement::SetInputParameterValue(double doubleParameter, bool firstParameter)
{
   if (firstParameter) ii_SQLParamIndex = 0;

   ip_SQLParam[ii_SQLParamIndex].doubleValue = doubleParameter;
   ii_SQLParamIndex++;
}

//****************************************************************************************************//
// Translate the SQL return code to a message and determine if it is fatal.
//****************************************************************************************************//
bool     SQLStatement::GetSQLErrorAndLog(SQLRETURN returnCode)
{
   const char       *errorType;
   const char       *severityLevel;
   bool        canContinue;
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

   is_MessageText[0] = 0;

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

         sqlReturnCode = SQLGetDiagRec(SQL_HANDLE_STMT,
                                       ih_StatementHandle,
                                       1,
                                       sqlState,
                                       &sqlNativeErrorCode,
                                       is_MessageText,
                                       ii_BufferSize,
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

   ip_Log->LogMessage("%d: ODBC Information: %s: %s", ip_DBInterface->GetClientID(), errorType, is_MessageText);
   ip_Log->LogMessage("%d: ODBC Information: SQL Statement: %s", ip_DBInterface->GetClientID(), ExpandStatement());

   return canContinue;
}

