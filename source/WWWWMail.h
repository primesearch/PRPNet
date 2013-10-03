#ifndef _WWWWMail_

#define _WWWWMail_

#include "Mail.h"

class WWWWMail : public Mail
{
public:
   WWWWMail(globals_t *globals, string serverName, uint32_t portID);

   void     MailSpecialResults(void);

   void     MailLowWorkNotification(int32_t daysLeft);

private:
   bool     NotifyUser(string toEmailID, int64_t prime, int32_t remainder, int32_t quotient,
                       int32_t duplicate, string machineID, string instanceID, string searchingProgram);
};

#endif // #ifndef _WWWWMail_
