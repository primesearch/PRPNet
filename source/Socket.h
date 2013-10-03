#ifndef _Socket_

#define _Socket_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include "defs.h"
#include "Log.h"
#include "SharedMemoryItem.h"

#ifdef WIN32
   #include <winsock.h>
   #include <io.h>
   #define GetSocketError() GetLastError()
   #define BLOCKING_ERROR WSAEWOULDBLOCK
   #define sock_t SOCKET
#endif

#ifdef UNIX
   #include <unistd.h>
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
   #include <fcntl.h>
   #include <sys/time.h>
   #define closesocket(s) close(s)
   #define GetSocketError() errno
   #define BLOCKING_ERROR EAGAIN
   #define sock_t int32_t
#endif

class Socket
{
public:
   Socket(Log *theLog, string socketDescription);

   ~Socket(void);

   // Open a socket.  If necessary override in classees that inherit
   bool     Open(void) { return false; };

   // Close a socket connection to the server
   void     Close(void);

   // Return a flag indicating if the socket is open
   bool     GetIsOpen(void) { return ib_IsOpen; };

   // Send a string of data to the server
   bool     Send(string fmt, ...);

   // Send the buffered messages
   bool     SendBuffer(void);

   // Start buffering messages
   void     StartBuffering(void) { ib_IsSendBuffering = true; ii_SendMessageSize = 0; };

   // Receive information from the server via the socket
   char    *Receive(int32_t maxWaitSeconds = 10);

   // Clear the communication buffer
   void     ClearBuffer(void);

   int32_t  GetSocketID(void) { return (int32_t) ii_SocketID; };

   void     SetAutoNewLine(bool autoNewLine) { ib_AutoNewLine = autoNewLine; };

   void     SetSocketDescription(string socketDescription) { is_SocketDescription = socketDescription; };

   void     SendNewLine(void);

   SharedMemoryItem  *GetThreadWaiter(void);

protected:
   // Get the IP address for the server name
   int32_t  GetAddress(string serverName);

   // Receive information from the server via the socket
   char    *GetNextMessageFromReadBuffer(void);

   // Send the buffered messages
   bool     SendBuffer(char *theBuffer);

   // A pointer to the Log
   Log     *ip_Log;

   // Whether or not the socket is open
   bool     ib_IsOpen;
   bool     ib_IsMail;

   bool     ib_AutoNewLine;

   // The socket for sending and receiving messages
   sock_t   ii_SocketID;
   string   is_SocketDescription;

   // The socket will buffer all messages until told to send them together
   bool     ib_IsSendBuffering;
   bool     ib_IsReadBuffering;

   // The buffer used for sending
   char    *is_SendBuffer;
   int32_t  ii_SendMessageSize;

   // The buffer used for reading
   char    *is_ReadBuffer;
   int32_t  ii_ReadBufferSize;

   // These are temporary buffers used by Send() and Receive()
   char    *is_TempSendBuffer;
   char    *is_TempReadBuffer;

   // This allows multiple threads to use the same socket
   SharedMemoryItem *ip_ThreadWaiter;
};

#endif // #ifndef _Socket_

