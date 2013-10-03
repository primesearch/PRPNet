#ifndef _WWWWWorkSender_

#define _WWWWWorkSender_

#include <string>
#include "WorkSender.h"

class WWWWWorkSender : public WorkSender
{
public:
   WWWWWorkSender(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                  string userID, string emailID, string machineID, 
                  string instanceID, string teamID);

   void     ProcessMessage(string theMessage);

private:
   bool     ib_HasSoftware;
   bool     ib_CanDoWieferich;
   bool     ib_CanDoWilson;
   bool     ib_CanDoWallSunSun;
   bool     ib_CanDoWolstenholme;
   int32_t  ii_SpecialThreshhold;

   int32_t     SelectWork(int32_t sendWorkUnits);
   bool        CheckDoubleCheck(int64_t lowerLimit, int64_t upperLimit);
   bool        ReserveRange(int64_t lowerLimit, int64_t upperLimit);
   bool        SendWork(int64_t lowerLimit, int64_t upperLimit);
};

#endif // #ifdef _WWWWWorkSender_
