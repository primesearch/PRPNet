#ifndef _Log_

#define _Log_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include "defs.h"
#include "SharedMemoryItem.h"

#define DEBUG_NONE	   0
#define DEBUG_ALL	      1
#define DEBUG_SOCKET	   2
#define DEBUG_WORK      3
#define DEBUG_DATABASE  4

class Log
{
public:
   Log(int32_t logLimit, string logFileName, int32_t debugLevel, bool echoToConsole);

   ~Log();

   // Log a message to the log file 
   void  LogMessage(string fmt, ...);

   // Log a message to the log file 
   void  TestMessage(string fmt, ...);

   // Log a message to the log file based upon the debug level
   void  Debug(int32_t debugLevel, string fmt, ...);

   void  SetDebugLevel(int32_t debugLevel) { ip_FileLock->SetValueNoLock(debugLevel); };

   void  SetEchoTest(bool echoTest) { ib_EchoTest = echoTest; };

   void  SetLogLimit(int32_t logLimit) { ii_LogLimit = logLimit; };

   bool  GetUseLocalTime(void) { return ib_UseLocalTime; };

   void  SetUseLocalTime(bool useLocalTime) { ib_UseLocalTime = useLocalTime; };

   void  AppendTimeZone(bool appendTimeZone) { ib_AppendTimeZone = appendTimeZone; };

private:
   // Write a message to the log file and echo it to stdout 
   void     Write(string fileName, const char *logMessage, int32_t toConsole, int32_t respectLimit);

   // Return the time in a formatted string
   char    *Time();

   // Does messages get echoed to console
   bool     ib_EchoToConsole;

   // Does test sent/received messages get echoed to console
   bool     ib_EchoTest;

   bool     ib_UseLocalTime;
   bool     ib_AppendTimeZone;

   // Size lLimit for the log file
   int32_t  ii_LogLimit;

   // Log file name
   string   is_LogFileName;

   // Temporary buffer for logging messages
   char    *is_TempBuffer;

   SharedMemoryItem *ip_FileLock;
};

#endif // #ifndef _Log_

