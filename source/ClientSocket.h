#ifndef _WorkSocket_

#define _WorkSocket_

#include "Socket.h"

class ClientSocket : public Socket
{
public:
   ClientSocket(Log *theLog, string serverName, int32_t portID, string socketDescription);

   bool     Open(string emailID, string userID, string machineID, string instanceID, string teamID);

   string   GetServerName(void) { return is_ServerName; };

   string   GetServerVersion(void) { return is_ServerVersion; };

   int32_t  GetServerType(void) { return ii_ServerType; };

   int32_t  GetPortID(void) { return ii_PortID; };

private:
   string   is_ServerName;
   int32_t  ii_PortID;

   string   is_ServerVersion;
   int32_t  ii_ServerType;
};

#endif // #ifndef _WorkSocket_

