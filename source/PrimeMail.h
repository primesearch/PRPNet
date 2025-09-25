#ifndef _PrimeMail_

#define _PrimeMail_

#include "Mail.h"

class PrimeMail : public Mail
{
public:
   PrimeMail(const globals_t * const globals, const string &serverName, const uint32_t portID);

   void     MailSpecialResults(void);

   void     MailLowWorkNotification(const int32_t daysLeft);

private:
   bool     NotifyUser(const string &toEmailID, const string &candidateName,
                       const int64_t testID, const double decimalLength);

   void     AppendGFNDivisibilityData(const int32_t theB, const int64_t theC,
                                      const int32_t checkedGFNDivisibility, const string &candidateName,
                                      const string &testedNumber);

};

#endif // #ifndef _PrimeMail_

