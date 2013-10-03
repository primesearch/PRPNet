#ifndef _MailSocket_

#define _MailSocket_

#include "defs.h"
#include "Socket.h"

class MailSocket : public Socket
{
public:
   MailSocket(Log *theLog, string serverName, int32_t portID, string emailID);
   
   bool     Open(void);

   void     AppendNewLine(void);

private:
   string   is_ServerName;
   int32_t  ii_PortID;

   string   is_EmailID;
};

#endif // #ifndef _MailSocket_

