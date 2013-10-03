#include "HelperSocket.h"

HelperSocket::HelperSocket(Log *theLog, sock_t socketID, string fromAddress, string description) : Socket(theLog, description)
{
   ib_IsOpen = true;
   ii_SocketID = socketID;
   is_FromAddress = fromAddress;
}
