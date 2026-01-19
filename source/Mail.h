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
   Mail(const globals_t * const globals, const string &serverName, const uint32_t portID);

   virtual ~Mail();

   void     SetDBInterface(DBInterface *dbInterface) { ip_DBInterface = dbInterface; };

   virtual void MailSpecialResults(void) = 0;

   virtual void MailLowWorkNotification(const int32_t daysLeft) = 0;

protected:
   bool     NewMessage(const string &toEmailID, const string subject, ...);

   void     AppendLine(int32_t newLines, const string line, ...);

   bool     SendMessage(void);

   Log     *ip_Log;
   DBInterface *ip_DBInterface;

   MailSocket  *ip_Socket;

   const string     is_FromEmailID;
   const string     is_ProjectName;
   const int32_t    ii_ServerType;

private:
   uint32_t GetLine(void);

   bool     SendHeader(const string &toEmailID, const string &theSubject);

   bool     SendHeader(const string &header, const string &text, const uint32_t expectedRC);

   bool     PrepareMessage(const string &toEmailID);

   static void encodeBase64(const string &inString, string &outString);

   const string     is_HostName;
   const string     is_FromPassword;

   uint32_t         ii_DestIDCount;
   string           is_DestID[10];

   const string     ii_ServerName;
   const int32_t    ii_ServerPortID;

   // This is set to true if a socket could be opened with the SMTP server
   bool     ib_Enabled;

   bool     ib_SMTPAuthentication;

   bool     ib_SMTPVerification;
};

#endif // #ifndef _Mail_

