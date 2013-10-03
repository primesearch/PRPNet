#ifndef _ServerSocket_

#define _ServerSocket_

#include "defs.h"
#include "Socket.h"
#include "SharedMemoryItem.h"

class ServerSocket : public Socket
{
public:
   ServerSocket(Log *theLog, int32_t portID);

   bool     Open(void);

   // Listen for a message
   bool     AcceptMessage(sock_t &clientSocketID, string &clientAddress);

private:
   int32_t  ii_PortID;
};

#endif // #ifndef _ServerSocket_

