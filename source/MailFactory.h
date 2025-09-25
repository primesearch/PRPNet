#ifndef _MailFactory_

#define _MailFactory_

#include "defs.h"
#include "server.h"
#include "Mail.h"

class MailFactory
{
public:
   MailFactory(void) {};

   Mail   *GetInstance(const globals_t * const globals, const string &smtpServer, const int32_t smtpPort) const;
};

#endif // #ifndef _MailFactory_
