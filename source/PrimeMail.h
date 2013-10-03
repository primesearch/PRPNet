#ifndef _PrimeMail_

#define _PrimeMail_

#include "Mail.h"

class PrimeMail : public Mail
{
public:
   PrimeMail(globals_t *globals, string serverName, uint32_t portID);

   void     MailSpecialResults(void);

   void     MailLowWorkNotification(int32_t daysLeft);

private:
   bool     NotifyUser(string toEmailID, string candidateName, 
                       int64_t testID, double decimalLength);

   void     AppendGFNDivisibilityData(int32_t theB, int32_t theC,
                                      int32_t checkedGFNDivisibility, string candidateName,
                                      string testedNumber);

};

#endif // #ifndef _PrimeMail_

