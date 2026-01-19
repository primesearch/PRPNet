#ifndef _GFNDivisorWorkReceiver_

#define _GFNDivisorWorkReceiver_

#include "WorkReceiver.h"

class GFNDivisorWorkReceiver : public WorkReceiver
{
public:
   GFNDivisorWorkReceiver(DBInterface* dbInterface, Socket* theSocket, globals_t* globals,
      string userID, string emailID, string machineID,
      string instanceID, string teamID);


   virtual ~GFNDivisorWorkReceiver();

   void  ProcessMessage(string theMessage);

private:
   int32_t  ReceiveWorkUnit(string theMessage);

   int32_t  ReceiveWorkUnit(int32_t n, int64_t kMin, int64_t kMax, int64_t testID);

   int32_t  ProcessWorkUnit(int32_t n, int64_t kMin, int64_t kMax, int64_t testID);

   bool     AbandonTest(int32_t n, int64_t kMin, int64_t kMax, int64_t testID);

   void     LogResults(int32_t n, int64_t kMin, int64_t kMax, int32_t finds);

   bool     ProcessFind(char* divisor);

   char     ic_Program[50];
   char     ic_ProgramVersion[50];
};

#endif // #ifndef _GFNDivisorWorkReceiver_
