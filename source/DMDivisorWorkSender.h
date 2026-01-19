#ifndef _DMDivisorWorkSender_

#define _DMDivisorWorkSender_

#include <string>
#include "WorkSender.h"

class DMDivisorWorkSender : public WorkSender
{
public:
   DMDivisorWorkSender(DBInterface* dbInterface, Socket* theSocket, globals_t* globals,
      string userID, string emailID, string machineID,
      string instanceID, string teamID);

   void     ProcessMessage(string theMessage);

private:
   bool     ib_HasSoftware;

   int32_t     SelectWork(int32_t sendWorkUnits);
   bool        ReserveRange(int32_t n, int64_t kMin, int64_t kMax);
   bool        SendWork(int32_t n, int64_t kMin, int64_t kMax);
};

#endif // #ifdef _DMDivisorWorkSender_
