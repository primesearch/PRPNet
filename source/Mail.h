#ifndef _Mail_

#define _Mail_

#include "defs.h"
#include "server.h"
#include "Log.h"
#include "MailSocket.h"
#include "DBInterface.h"

class Mail
{
public:
   Mail(globals_t *globals, string serverName, uint32_t portID);

   virtual ~Mail();

   void     SetDBInterface(DBInterface *dbInterface) { ip_DBInterface = dbInterface; };

   virtual void MailSpecialResults(void) {};

   virtual void MailLowWorkNotification(int32_t daysLeft) {};

protected:
   uint32_t GetLine(void);

   bool     SendHeader(string toEmailID, string theSubject);

   bool     SendHeader(string header, string text, uint32_t expectedRC);

   bool     PrepareMessage(string toEmailID);

   void     encodeBase64(string inString, string &outString);

   bool     NewMessage(string toEmailID, string subject, ...);

   void     AppendLine(int32_t newLines, string fmt, ...);

   bool     SendMessage(void);

   Log     *ip_Log;
   DBInterface *ip_DBInterface;

   MailSocket  *ip_Socket;

   string   is_HostName;
   string   is_FromEmailID;
   string   is_FromPassword;

   uint32_t ii_DestIDCount;
   string   is_DestID[10];
   string   is_ProjectName;

   int32_t  ii_ServerType;
   string   ii_ServerName;
   int32_t  ii_ServerPortID;

   // This is set to 1 if a socket could be opened with the SMTP server
   bool     ib_Enabled;

   bool     ib_SMTPAuthentication;

   bool     ib_SMTPVerification;
};

#endif // #ifndef _Mail_

