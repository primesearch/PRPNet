#ifndef _PrimeWorkReceiver_

#define _PrimeWorkReceiver_

#include "WorkReceiver.h"
#include "CandidateTestResult.h"

class PrimeWorkReceiver : public WorkReceiver
{
public:
   PrimeWorkReceiver(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                     string userID, string emailID, string machineID, 
                     string instanceID, string teamID);

   ~PrimeWorkReceiver();

   void  ProcessMessage(string theMessage);

protected:
   int32_t  ReceiveWorkUnit(string theMessage);

private:
   int32_t     ii_TestResults;
   CandidateTestResult *ip_TestResult[10];

   int32_t     ReceiveWorkUnit(string candidateName, int64_t testID, string clientVersion);

   int32_t     ProcessWorkUnit(string candidateName, int64_t testID, string clientVersion);

   bool        CheckDoubleCheck(string candidateName, string residue);

   bool        AbandonTest(string candidateName, int64_t testID);
};

#endif // #ifndef _PrimeWorkReceiver_
