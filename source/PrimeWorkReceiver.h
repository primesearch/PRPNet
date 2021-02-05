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


   virtual ~PrimeWorkReceiver();

   void  ProcessMessage(string theMessage);

protected:
   int32_t  ReceiveWorkUnit(string theMessage);

private:
   int32_t     ii_TestResults;
   bool        ib_OneKPerInstance;
   
   CandidateTestResult *ip_TestResult[10];

   int32_t     ReceiveWorkUnit(string candidateName, int64_t testID, int32_t genericDecimalLength, string clientVersion);

   int32_t     ProcessWorkUnit(string candidateName, int64_t testID, int32_t genericDecimalLength, string clientVersion);

   bool        CheckDoubleCheck(string candidateName, string residue);

   bool        AbandonTest(string candidateName, int64_t testID);

   bool        UpdateGroupStats(string candidateName, result_t mainTestResult);
  
   bool        BadProgramVersion(string version);
};

#endif // #ifndef _PrimeWorkReceiver_
