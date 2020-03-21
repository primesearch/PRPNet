#include "Log.h"

Log::Log(int32_t logLimit, string logFileName, int32_t debugLevel, bool echoToConsole)
{
   ip_FileLock = new SharedMemoryItem(logFileName);

   ii_LogLimit = logLimit;

   is_LogFileName = logFileName;
   SetDebugLevel(debugLevel);

   ib_EchoToConsole = echoToConsole;
   ib_EchoTest = true;
   ib_UseLocalTime = true;
   ib_AppendTimeZone = true;

   is_TempBuffer = new char[10000];
}

Log::~Log()
{
   delete ip_FileLock;
   delete [] is_TempBuffer;
}

void  Log::LogMessage(string fmt, ...)
{
   va_list  args;

   va_start(args, fmt);
   vsprintf(is_TempBuffer, fmt.c_str(), args);
   va_end(args);

   ip_FileLock->Lock();

   if (ii_LogLimit != -1)
      Write(is_LogFileName, is_TempBuffer, ib_EchoToConsole, true);

   ip_FileLock->Release();
}

void  Log::TestMessage(string fmt, ...)
{
   va_list  args;

   va_start(args, fmt);
   vsprintf(is_TempBuffer, fmt.c_str(), args);
   va_end(args);
   
   ip_FileLock->Lock();

   if (ii_LogLimit != -1)
      Write(is_LogFileName, is_TempBuffer, ib_EchoTest, true);

   ip_FileLock->Release();
}

void  Log::Debug(int32_t debugLevel, string fmt, ...)
{
   va_list  args;
   int32_t  internalDebugLevel = (int32_t) ip_FileLock->GetValueNoLock();

   // Check if we are debugging
   if (internalDebugLevel == 0)
      return;

   if (internalDebugLevel == DEBUG_ALL || (internalDebugLevel == debugLevel))
   {
      ip_FileLock->Lock();

      va_start(args, fmt);
      vsprintf(is_TempBuffer, fmt.c_str(), args);
      va_end(args);

      Write(is_LogFileName, is_TempBuffer, true, true);

      ip_FileLock->Release();
   }
}

void  Log::Write(string fileName, const char *logMessage, int32_t toConsole, int32_t respectLimit)
{
   FILE       *fp;

   struct stat buf;
   char        newFileName[400];
   char        oldFileName[400];

   if ((fp = fopen(fileName.c_str(), "at")) == NULL)
   {
      // couldn't open for appending
      if (stat(fileName.c_str(), &buf) == -1)
      {
         // File doesn't exist
         if ((fp = fopen(fileName.c_str(), "wt")) == NULL)
         {
            fprintf(stderr, "Couldn't create file [%s].\n", fileName.c_str());
            return;
         }
      }
      else 
      {
         fprintf(stderr, "Couldn't append to file [%s].\n", fileName.c_str());
         return;
      }
   }

   if (toConsole)
   {
      // If the line begins with a '\n' then make sure the time
      // in inserted after the \n and not before
      if (*logMessage == '\n')
         fprintf(stderr, "\n[%s] %s\n", Time(), &logMessage[1]);
      else
         fprintf(stderr, "[%s] %s\n", Time(), logMessage);
      fflush(stderr);
   }

   // If the line begins with a '\n' then make sure the time
   // in inserted after the \n and not before
   if (*logMessage == '\n')
      fprintf(fp, "\n[%s] %s\n", Time(), &logMessage[1]);
   else
      fprintf(fp, "[%s] %s\n", Time(), logMessage);

   fclose(fp);

   if (respectLimit && ii_LogLimit>0)
   {
      // See if the file has grown too big...
      if (stat(fileName.c_str(), &buf) != -1)
      {
         if (buf.st_size > ii_LogLimit)
         {
            strcpy(newFileName, fileName.c_str());
            strcpy(oldFileName, fileName.c_str());
            strcat(oldFileName, ".old");
            // delete filename.old
            unlink(oldFileName);
            // rename filename to filename.old
            rename((const char *) newFileName, (const char *) oldFileName);
         }
      }
   }
}

char *Log::Time()
{
   time_t timenow;
   struct tm *ltm;
   static char timeString[200];
   char   timeZone[200];
   char  *ptr1, *ptr2;

   while (true)
   {
     timenow = time(NULL);

     if (ib_UseLocalTime)
		ltm = localtime(&timenow);
     else
		ltm = gmtime(&timenow);

     if (!ltm)
        continue;

     if (ib_UseLocalTime)
     {
        // This could return a string like "Central Standard Time" or "CST"
        strftime(timeZone, sizeof(timeZone), "%Z", ltm);

        // If there are embedded spaces, this will convert
        // "Central Standard Time" to "CST"
        ptr1 = timeZone;
	     ptr1++;
        ptr2 = ptr1;
        while (true)
        {
           // Find the space
           while (*ptr2 && *ptr2 != ' ')
	           ptr2++;

           if (!*ptr2 || iscntrl(*ptr2)) 
              break;

           // Skip the space
           ptr2++;
           if (!*ptr2 || iscntrl(*ptr2)) 
              break;

           *ptr1 = *ptr2;
           ptr1++;
           *ptr1 = 0;
        }
     }
     else
        strcpy(timeZone, "GMT");

     strftime(timeString, sizeof(timeString), "%Y-%m-%d %X", ltm);
     if (ib_AppendTimeZone)
     {
        strcat(timeString, " ");
        strcat(timeString, timeZone);
     }

     break;
   }

   return timeString;
}
