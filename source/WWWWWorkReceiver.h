#ifndef _WWWWWorkReceiver_

#define _WWWWWorkReceiver_

#include "WorkReceiver.h"
class WWWWWorkReceiver : public WorkReceiver
{
public:
   WWWWWorkReceiver(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                    string userID, string emailID, string machineID, 
                    string instanceID, string teamID);


   virtual ~WWWWWorkReceiver();
 
   void  ProcessMessage(string theMessage);

private:
   int32_t  ReceiveWorkUnit(string theMessage);

   int32_t  ReceiveWorkUnit(int64_t lowerLimit, int64_t upperLimit, int64_t testID);

   int32_t  ProcessWorkUnit(int64_t lowerLimit, int64_t upperLimit, int64_t testID);

   bool     CheckDoubleCheck(int64_t lowerLimit, int64_t upperLimit,
                             int64_t primesTested, string checksum);

   bool     AbandonTest(int64_t lowerLimit, int64_t upperLimit, int64_t testID);

   void     LogResults(int64_t lowerLimit, int64_t upperLimit, int32_t finds, int32_t nearFinds);

   bool     ProcessFind(int64_t lowerLimit, int64_t upperLimit, int64_t testID,
                        int64_t prime, int32_t remainder, int32_t quotient);

   bool     BadProgramVersion(string version);

   char     ic_Program[50];
   char     ic_ProgramVersion[50];
};

#endif // #ifndef _WWWWWorkReceiver_
