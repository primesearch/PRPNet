#ifndef _GFNDivisorWorkSender_

#define _GFNDivisorWorkSender_

#include <string>
#include "WorkSender.h"

class GFNDivisorWorkSender : public WorkSender
{
public:
   GFNDivisorWorkSender(DBInterface* dbInterface, Socket* theSocket, globals_t* globals,
      string userID, string emailID, string machineID,
      string instanceID, string teamID);

   void     ProcessMessage(string theMessage);

private:
   bool     ib_HasSoftware;

   int32_t     SelectWork(int32_t sendWorkUnits);
   bool        ReserveRange(int32_t n, int64_t kMin, int64_t kMax);
   bool        SendWork(int32_t n, int64_t kMin, int64_t kMax);
};

#endif // #ifdef _GFNDivisorWorkSender_
