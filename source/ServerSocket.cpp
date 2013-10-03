#include "ServerSocket.h"

ServerSocket::ServerSocket(Log *theLog, int32_t portID) : Socket(theLog, "server")
{
   ii_PortID = portID;
}

bool     ServerSocket::Open()
{
   struct sockaddr_in   sin;
   int                  mytrue = 1;

   // Using IPPROTO_IP is equivalent to IPPROTO_TCP since AF_INET and SOCK_STREAM are used
   ii_SocketID = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

#ifdef WIN32
   if (ii_SocketID == INVALID_SOCKET)
      return false;
#else
   if (ii_SocketID < 0)
      return false;
#endif

   // Try to reuse the socket
   if (setsockopt(ii_SocketID, SOL_SOCKET, SO_REUSEADDR, (char*)&mytrue, sizeof(mytrue)) != 0)
   {
      Close();
      ip_Log->LogMessage("Port %d: setsockopt(SO_REUSEADDR) failed", ii_PortID);
      return false;
   }

   // bind the socket to address and port given
   memset(&sin, 0x00, sizeof(sin));
   sin.sin_family       = AF_INET;
   sin.sin_addr.s_addr  = inet_addr("0.0.0.0");
   sin.sin_port         = htons((unsigned short) ii_PortID);

   if (bind(ii_SocketID, (struct sockaddr *) &sin, sizeof(sin)) != 0)
   {
      ip_Log->LogMessage("Port %d: bind on socket failed", ii_PortID);
      return false;
   }

   // set up to receive connections on this socket
   if (listen(ii_SocketID, 999) != 0)
   {
      Close();
      ip_Log->LogMessage("Port %d: listen on socket failed", ii_PortID);
      return false;
   }

   ip_Log->Debug(DEBUG_SOCKET, "Port %d: Listening on socket %d", ii_PortID, ii_SocketID);
   ib_IsOpen = true;

   return true;
}

bool     ServerSocket::AcceptMessage(sock_t &clientSocketID, string &clientAddress)
{
   struct sockaddr_in   sin;
   sock_t               socketID;
#if defined(WIN32)
   int                  sinlen;
#else
   socklen_t            sinlen;
#endif

   sinlen = sizeof(struct sockaddr);
   socketID = accept(ii_SocketID, (struct sockaddr *) &sin, &sinlen);

#ifdef WIN32
   if (socketID == INVALID_SOCKET)
      return false;
#else
   if (socketID < 0)
      return false;
#endif

   ip_Log->Debug(DEBUG_SOCKET, "%d: client connecting from %s", socketID, inet_ntoa(sin.sin_addr));

   ib_IsOpen = true;
   ib_IsReadBuffering = false;
   ib_IsSendBuffering = false;

   clientAddress = inet_ntoa(sin.sin_addr);
   clientSocketID = socketID;

   return true;
}
