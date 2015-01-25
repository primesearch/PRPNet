#include "ClientSocket.h"

ClientSocket::ClientSocket(Log *theLog, string serverName, int32_t portID, string socketDescription)
               : Socket(theLog, socketDescription)
{
   is_ServerName = serverName;
   ii_PortID = portID;
}

bool     ClientSocket::Open(string emailID, string userID, string machineID, string instanceID, string teamID)
{
   struct sockaddr_in   sin;
   uint32_t             addr;
   char                *readBuf;
   char                 serverVersion[50];

   if (ib_IsOpen)
      return true;

   addr = GetAddress(is_ServerName);
   if (!addr)
      return false;

   // Using IPPROTO_IP is equivalent to IPPROTO_TCP since AF_INET and SOCK_STREAM are used
   ii_SocketID = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

#ifdef WIN32
   if (ii_SocketID == INVALID_SOCKET)
      return false;
#else
   if (ii_SocketID < 0)
      return false;
#endif

   memset(&sin, 0x00, sizeof(sin));
   sin.sin_family       = AF_INET;
   sin.sin_addr.s_addr  = addr;
   sin.sin_port         = htons((unsigned short) ii_PortID);

   if (connect(ii_SocketID, (struct sockaddr *) &sin, sizeof(sin)) != 0)
   {
      ip_Log->LogMessage("%s:%d connect to socket failed with error %d", is_ServerName.c_str(), ii_PortID, GetSocketError());
      Close();
      return false;
   }

   ib_IsOpen = true;

   Send("FROM %s %s %s %s %s %s", PRPNET_VERSION, emailID.c_str(), machineID.c_str(), userID.c_str(), teamID.c_str(), instanceID.c_str());

   // Verify that we have connected to the server
   readBuf = Receive();

   if (!readBuf || memcmp(readBuf, "Connected to server", 19))
   {
      if (readBuf && !memcmp(readBuf, "ERROR", 5))
         ip_Log->LogMessage(readBuf);
      else
         ip_Log->LogMessage("Could not verify connection to %s.  Will try again later.", is_ServerName.c_str());
      Close();
      return false;
   }

   if (sscanf(readBuf, "Connected to server %s %d", serverVersion, &ii_ServerType) != 2)
   {
      ip_Log->LogMessage("Could not determine server type for %s.  Will try again later.", is_ServerName.c_str());
      return false;
   }
   
   is_ServerVersion = serverVersion;

   return true;
}
