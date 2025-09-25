#ifndef _WWWWMail_

#define _WWWWMail_

#include "Mail.h"

class WWWWMail : public Mail
{
public:
   WWWWMail(const globals_t * const globals, const string &serverName, const uint32_t portID);

   void     MailSpecialResults(void);

   void     MailLowWorkNotification(const int32_t daysLeft);

private:
   bool     NotifyUser(const string &toEmailID, const int64_t prime, const int32_t remainder, const int32_t quotient,
                       const int32_t duplicate, const string &machineID, const string &instanceID, const string &searchingProgram);
};

#endif // #ifndef _WWWWMail_
