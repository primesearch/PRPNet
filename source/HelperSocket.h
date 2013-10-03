#ifndef _HelperSocket_

#define _HelperSocket_

#include "defs.h"
#include "Socket.h"

class HelperSocket : public Socket
{
public:
   HelperSocket(Log *theLog, sock_t socketID, string fromAddress, string socketDescription);

   string   GetFromAddress(void) { return is_FromAddress; };

private:
   string   is_FromAddress;

};

#endif // #ifndef _HelperSocket_

