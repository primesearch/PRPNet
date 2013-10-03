#include "MailSocket.h"

MailSocket::MailSocket(Log *theLog, string serverName, int32_t portID, string emailID) : Socket(theLog, "mail")
{
   is_ServerName = serverName;
   is_EmailID = emailID;

   ii_PortID = portID;
   ib_IsMail = true;
}

bool     MailSocket::Open()
{
   struct sockaddr_in   sin;
   uint32_t             addr;

   if (ib_IsOpen)
      return true;

   addr = GetAddress(is_ServerName.c_str());
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
      Close();
      ip_Log->LogMessage("%s:%ld connect to socket failed with error %d",
                         is_ServerName.c_str(), ii_PortID, GetSocketError());
      return false;
   }

   ib_IsOpen = true;

   return true;
}
