#ifndef _PrimeWorkSender_

#define _PrimeWorkSender_

#include <string>
#include "WorkSender.h"

class PrimeWorkSender : public WorkSender
{
public:
   PrimeWorkSender(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                   string userID, string emailID, string machineID, 
                   string instanceID, string teamID);

   ~PrimeWorkSender();

   void     ProcessMessage(string theMessage);

private:
   bool     ib_UseLLROverPFGW;
   bool     ib_OneKPerInstance;
   string   is_OrderBy;
   int32_t  ii_DelayCount;
   delay_t *ip_Delay;

   bool     ib_HasLLR;
   bool     ib_HasPhrot;
   bool     ib_HasPFGW;
   bool     ib_HasCyclo;
   bool     ib_HasAnyGenefer;
   bool     ib_HasGeneferGPU;
   bool     ib_HasGenefx64;
   bool     ib_HasGenefer;
   bool     ib_HasGenefer80bit;
   
   int32_t     SendWorkToClient(int32_t sendWorkUnits, bool doubleCheckOnly, bool oneKPerInstance);
   int32_t     SelectDoubleCheckCandidates(int32_t sendWorkUnits, double minLength, double maxLength, int64_t olderThanTime);
   int32_t     SelectOneKPerClientCandidates(int32_t sendWorkUnits, bool firstPass);
   int32_t     SelectGFNCandidates(int32_t sendWorkUnits);
   int32_t     SelectCandidates(int32_t sendWorkUnits);
   bool        CheckGenefer(string candidateName);
   bool        CheckDoubleCheck(string candidateName, double decimalLength, int64_t lastUpdateTime);
   bool        ReserveCandidate(string candidateName);
   bool        SendWork(string candidateName, int64_t theK, int32_t theB, int32_t theN, int32_t theC);
   bool        UpdateGroupStats(string candidateName);
};

#endif // #ifdef _PrimeWorkSender_
