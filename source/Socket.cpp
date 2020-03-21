#include "Socket.h"

#ifndef WIN32
#include <netinet/tcp.h>
#endif

#define TEMP_BUFFER_SIZE    4000
#define SEND_BUFFER_SIZE    4000
#define READ_BUFFER_SIZE   10000

Socket::Socket(Log *theLog, string socketDescription)
{
   ip_Log = theLog;

   ib_IsOpen = false;
   ib_IsMail = false;
   ib_IsReadBuffering = false;
   ib_IsSendBuffering = false;

   ip_ThreadWaiter = NULL;

   ii_ReadBufferSize = READ_BUFFER_SIZE;
   ii_SendMessageSize = 0;

   is_TempSendBuffer = new char[TEMP_BUFFER_SIZE + 10];
   is_SendBuffer = new char[SEND_BUFFER_SIZE + 10];
   is_TempReadBuffer = new char[ii_ReadBufferSize + 10];
   is_ReadBuffer = new char[ii_ReadBufferSize + 10];

   is_SendBuffer[0] = 0;
   is_ReadBuffer[0] = 0;

   ib_AutoNewLine = true;

   is_SocketDescription = socketDescription;
}

Socket::~Socket()
{
   if (ip_ThreadWaiter)
      delete ip_ThreadWaiter;

   if (ib_IsOpen)
      Close();

   delete [] is_TempSendBuffer;
   delete [] is_TempReadBuffer;
   delete [] is_SendBuffer;
   delete [] is_ReadBuffer;
}

SharedMemoryItem  *Socket::GetThreadWaiter(void)
{
   char   id[10];

   sprintf(id, "TW %d", (int32_t) ii_SocketID);

   if (!ip_ThreadWaiter)
      ip_ThreadWaiter = new SharedMemoryItem(id);

   return ip_ThreadWaiter;
}

void  Socket::Close()
{
   // Don't give the message again, but force the socket closed
   // just in case it is open.
   if (ib_IsOpen)
      ip_Log->Debug(DEBUG_SOCKET, "%s: closing socket %d", is_SocketDescription.c_str(), ii_SocketID);

   closesocket(ii_SocketID);

   ClearBuffer();
   ib_IsOpen = 0;
}

void  Socket::ClearBuffer()
{
   ib_IsReadBuffering = false;
   ib_IsSendBuffering = false;
   is_ReadBuffer[0] = 0;
   is_SendBuffer[0] = 0;

   ii_SendMessageSize = 0;
}

int32_t  Socket::GetAddress(string serverName)
{
   uint32_t        addr;
   struct hostent *hp;
   int             host_alias;
   int             i;

   if ((int) (addr = inet_addr(serverName.c_str())) == -1)
   {
      hp = gethostbyname(serverName.c_str());
      if (!hp)
      {
         perror("gethostbyname");
#ifdef WIN32
         printf("gethostbyname(%s) generated error %d\n", serverName.c_str(), WSAGetLastError());
#else
         printf("gethostbyname(%s) generated error %d\n", serverName.c_str(), errno);
#endif
         return 0;
      }

      host_alias = 0;
      for (i=0; hp->h_addr_list[i]; i++)
         if (i > 0)
            host_alias = rand() % i;

      memcpy((void *) &addr, (void *) hp->h_addr_list[host_alias], sizeof(addr));
   }

   return addr;
}

// PPRNet uses \n as a delimiter within buffered messages.  Search for the \n,
// send the string leading up to it, then move the pointer to the character
// after the \n.
char    *Socket::GetNextMessageFromReadBuffer(void)
{
   char *pos;

   strcpy(is_TempReadBuffer, is_ReadBuffer);

   // Each part of the message is terminated with a \n
   pos = strchr(is_ReadBuffer, '\n');

   if (!pos)
   {
      // If the \n is missing, then log a message.  It is probable that returning
      // an incomplete message here will cause problems elsewhere, so clear
      // the buffer, return nothing, and let the caller handle the problem.
      ip_Log->Debug(DEBUG_SOCKET, "%s: Error:  missing end-of-line [%s]", is_SocketDescription.c_str(), is_ReadBuffer);
      ClearBuffer();
      return NULL;
   }

   is_TempReadBuffer[pos - is_ReadBuffer] = 0;

   ip_Log->Debug(DEBUG_SOCKET, "parsed %s\n", is_TempReadBuffer);

   // Point to the 0 that we just put into is_TempReadBuffer
   pos = &is_TempReadBuffer[pos - is_ReadBuffer];

   // Point to the next character, then copy back to is_ReadBuffer
   pos++;
   if (*pos)
   {
      strcpy(is_ReadBuffer, pos);
      ip_Log->Debug(DEBUG_SOCKET, "next %s\n", is_ReadBuffer);
}
   else
      ClearBuffer();

   ip_Log->Debug(DEBUG_SOCKET, "%s: received [%s]", is_SocketDescription.c_str(), is_TempReadBuffer);

   return is_TempReadBuffer;
}

// Continue reading from the socket until there is no more data or until
// we timeout waiting for data
char    *Socket::Receive(int32_t maxWaitSeconds)
{
   struct timeval tv;
   fd_set   readfds, errorfds;
   time_t   theTime;
   int32_t  rc, noSelectCount;
   bool     quit;
   bool     clientDisconnected;
   int32_t  totalBytesReceived;
   char    *ptr;

   if (!ib_IsOpen)
      return NULL;

   if (ib_IsReadBuffering)
      return GetNextMessageFromReadBuffer();

   theTime = time(NULL);

   totalBytesReceived = 0;
   is_ReadBuffer[0] = 0;
   is_TempSendBuffer[0] = 0;
   noSelectCount = 0;

   tv.tv_sec = 0;
   tv.tv_usec = 1;

   clientDisconnected = quit = false;
   while (!quit)
   {
      noSelectCount = 0;
      do
      {
         FD_ZERO(&readfds);
         FD_ZERO(&errorfds);
         FD_SET(ii_SocketID, &readfds);
         errorfds = readfds;

         rc = select(64, &readfds, 0, &errorfds, &tv);

         if (rc > 0)
            break;

         if (rc < 1)
         {
            // If we have waited 200 ms without receiving more data,
            // presume that all of it has been received.
            if (++noSelectCount == 10 && totalBytesReceived)
               quit = true;

            // If we have received nothing for the max time then give up.
            if (difftime(time(NULL), theTime) >= maxWaitSeconds)
               quit = true;
         }

         // select() does not relinquish control while it is waiting for a
         // message on the socket.  Sleep() will relinquish control, so we'll
         // just sleep if nothing was selected before trying again rather than
         // waiting for select() to get data from the socket.
         Sleep(5);
      } while (rc < 1 && !quit);

      if (quit)
         break;

      if (FD_ISSET(ii_SocketID, &errorfds))
      {
         ip_Log->Debug(DEBUG_SOCKET, "%s: error while receiving", is_SocketDescription.c_str());
         break;
      }

      if (FD_ISSET(ii_SocketID, &readfds))
      {
         memset(is_TempReadBuffer, 0, TEMP_BUFFER_SIZE);
         rc = recv(ii_SocketID, is_TempReadBuffer, TEMP_BUFFER_SIZE, 0);

         if (rc == 0)
         {
            // This appears to happen only when the client disconnects
            clientDisconnected = true;
            quit = true;
         }
         else if (rc < 0)
         {
            if (GetSocketError() == BLOCKING_ERROR)
               Sleep(50);
            else
            {
               ip_Log->Debug(DEBUG_SOCKET, "%s: error %d while receiving", is_SocketDescription.c_str(), GetSocketError());
               quit = true;
            }
         }
         else
         {
            if (rc + totalBytesReceived > ii_ReadBufferSize)
            {
               ii_ReadBufferSize += READ_BUFFER_SIZE;
               ptr = new char[ii_ReadBufferSize];

               strcpy(ptr, is_ReadBuffer);
               delete [] is_ReadBuffer;
               is_ReadBuffer = ptr;

               ip_Log->Debug(DEBUG_SOCKET, "%s: receive buffer size increased to %d bytes", is_SocketDescription.c_str(), ii_ReadBufferSize);
            }

            is_TempReadBuffer[rc] = 0;
            totalBytesReceived += rc;
            strcat(is_ReadBuffer, is_TempReadBuffer);
            is_ReadBuffer[totalBytesReceived] = 0;

            ip_Log->Debug(DEBUG_SOCKET, "read %s\n", is_ReadBuffer);

            theTime = time(NULL);
         }
      }
   }

   if (!totalBytesReceived)
   {
      if (!clientDisconnected)
         ip_Log->LogMessage("%s: nothing was received on socket after %d seconds",
                            is_SocketDescription.c_str(), maxWaitSeconds);
      return NULL;
   }

   ib_IsReadBuffering = true;
   return GetNextMessageFromReadBuffer();
}

// Format a string and either send it across the socket or buffer it
bool     Socket::Send(string fmt, ...)
{
   bool     didSend;
   int32_t  messageSize;
   va_list  args;

   va_start(args, fmt);
   vsprintf(is_TempSendBuffer, fmt.c_str(), args);
   va_end(args);

   ip_Log->Debug(DEBUG_SOCKET, "%s: sending [%s]", is_SocketDescription.c_str(), is_TempSendBuffer);

   if (ib_AutoNewLine)
      SendNewLine();

   if (ib_IsSendBuffering)
   {
      didSend = true;
      messageSize = (int32_t) strlen(is_TempSendBuffer);

      if (ii_SendMessageSize + messageSize > SEND_BUFFER_SIZE)
         didSend = SendBuffer(is_SendBuffer);

      ii_SendMessageSize += messageSize;

      strcat(is_SendBuffer, is_TempSendBuffer);

      return didSend;
   }

   return SendBuffer(is_TempSendBuffer);
}

void     Socket::SendNewLine(void)
{
   if (ib_IsMail && ib_IsSendBuffering)
   {
      strcat(is_SendBuffer, "\r\n");
      return;
   }

   if (ib_IsMail)
      strcat(is_TempSendBuffer, "\r");

   strcat(is_TempSendBuffer, "\n");
}

// Send whatever is remaining in our buffer
bool     Socket::SendBuffer(void)
{                            
   ib_IsSendBuffering = false;

   return SendBuffer(is_SendBuffer);
}

// Send a buffer of data across the socket
bool     Socket::SendBuffer(char *buffer)
{
   char    *ptr;
   bool     haveError = false;
   int32_t  bytesSent;

   if (!ib_IsOpen)
      return false;

   if (!strlen(buffer))
      return true;

   ptr = buffer;
   while ((ptr != buffer+strlen(buffer)) && !haveError)
   {
      bytesSent = send(ii_SocketID, ptr, (int32_t) strlen(ptr), 0);
      if (bytesSent == -1)
      {
         if (GetSocketError() == BLOCKING_ERROR)
            Sleep(50);
         else
            haveError = true;
      }
      else
         ptr += bytesSent;
   }

   *buffer = 0;
   ii_SendMessageSize = 0;

   return !haveError;
}
