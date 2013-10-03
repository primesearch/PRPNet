#ifndef _MailFactory_

#define _MailFactory_

#include "defs.h"
#include "server.h"
#include "Mail.h"

class MailFactory
{
public:
   MailFactory(void) {};

   Mail   *GetInstance(globals_t *globals, string smtpServer, int32_t smtpPort);
};

#endif // #ifndef _MailFactory_
