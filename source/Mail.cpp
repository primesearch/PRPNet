#include "Mail.h"
#include "SQLStatement.h"

Mail::Mail(globals_t *globals, string serverName, uint32_t portID)
{
   int32_t  i;

   ip_Log = globals->p_Log;
   is_FromEmailID = globals->s_EmailID;
   is_FromPassword = globals->s_EmailPassword;
   is_HostName = "localhost";
   ii_ServerType = globals->i_ServerType;
   is_ProjectName = globals->s_ProjectName;
   ii_ServerPortID = globals->i_PortID;
   ib_SMTPAuthentication = false;
   ib_SMTPVerification = false;

   ip_Socket = new MailSocket(ip_Log, serverName, portID, globals->s_EmailID);

   ii_DestIDCount = 0;
   for (i=0; i<globals->i_DestIDCount; i++)
   {
      if (is_FromEmailID != globals->s_DestID[i])
      {
         is_DestID[ii_DestIDCount] = globals->s_DestID[i];
         ii_DestIDCount++;
      }
   }

   if (ip_Socket->Open())
   {
      ip_Socket->Close();
      ib_Enabled = true;
   }
   else
   {
      printf("Could not open socket with SMTP server at %s.  Mail has been disabled\n", serverName.c_str());
      ib_Enabled = false;
   }
}

Mail::~Mail()
{
   delete ip_Socket;
}

bool     Mail::NewMessage(string toEmailID, string subject, ...)
{
   uint32_t   rc;
   char     theSubject[BUFFER_SIZE];
   va_list  args;

   if (!ib_Enabled)
      return false;

   va_start(args, subject);
   vsprintf(theSubject, subject.c_str(), args);
   va_end(args);

   if (!ip_Socket->Open())
      return false;

   rc = GetLine();
   if (rc != 220)
   {
      ip_Log->LogMessage("SMTP server error.  Expected 220, got %ld", rc);
      ip_Socket->Close();
      return false;
   }

   if (!PrepareMessage(toEmailID))
   {
      ip_Socket->Close();
      return false;
   }

   if (!SendHeader(toEmailID, theSubject))
   {
      ip_Socket->Close();
      return false;
   }

   return true;
}

void    Mail::AppendLine(int32_t newLines, string line, ...)
{
   char     theLine[2000];
   va_list  args;

   va_start(args, line);
   vsprintf(theLine, line.c_str(), args);
   va_end(args);

   while (newLines-- > 0)
      ip_Socket->SendNewLine();

   ip_Socket->Send("%s  ", theLine);
}

bool     Mail::SendMessage(void)
{
   if (!ip_Socket->GetIsOpen())
      return false;

   ip_Socket->SendBuffer();

   ip_Socket->Send("\r\n.");

   if (GetLine() != 250)
   {
      ip_Log->LogMessage("SMTP server error.  The message was not accepted by the server");
      ip_Socket->Close();
      return false;
   }

   ip_Socket->Send("QUIT");
   GetLine();

   ip_Socket->Close();
   return true;
}

uint32_t  Mail::GetLine(void)
{
   char   *data;

   data = 0;
   while (!data)
   {
      data = ip_Socket->Receive(2);

      if (!data)
         break;

      if (strstr(data, "ESMTP"))
         ib_SMTPAuthentication = true;
      if (strstr(data, "VRFY"))
         ib_SMTPVerification = true;

      if (strlen(data) < 3 || data[3] == '-')
         data = 0;
   }

   if (!data)
      return 0;

   return atol(data);
}

bool     Mail::SendHeader(string toEmailID, string theSubject)
{
   uint32_t  i;
   char    theDate[255];
   time_t  theTime;

   theTime = time(NULL);
   strftime(theDate, 80, "%a, %d %b %Y %X %Z", localtime(&theTime));

   ip_Socket->Send("Subject: %s", theSubject.c_str());
   ip_Socket->Send("From: %s", is_FromEmailID.c_str());
   ip_Socket->Send("To: %s", is_FromEmailID.c_str());
   if (toEmailID != is_FromEmailID)
      ip_Socket->Send("To: %s", toEmailID.c_str());
   for (i=0; i<ii_DestIDCount; i++)
      ip_Socket->Send("To: %s", is_DestID[i].c_str());
   ip_Socket->Send("Date: %s", theDate);

   return true;
}

bool     Mail::SendHeader(string header, string text, uint32_t expectedRC)
{
   int32_t   rc;
   uint32_t   gotRC;

   if (text.length() > 0)
      rc = ip_Socket->Send("%s %s", header.c_str(), text.c_str());
   else
      rc = ip_Socket->Send("%s", header.c_str());

   if (rc < 0)
   {
      ip_Log->LogMessage("SMTP socket error sending '%s'", header.c_str());
      return false;
   }

   gotRC = GetLine();
   if (gotRC != expectedRC)
   {
      ip_Log->LogMessage("SMTP server error.  Sent '%s', expected %ld, got %ld", header.c_str(), expectedRC, gotRC);
      return false;
   }

   return true;
}

bool     Mail::PrepareMessage(string toEmailID)
{
   uint32_t  i;
   string    encodedEmailID, encodedPassword;

   if (!ib_SMTPAuthentication)
   {
      if (!SendHeader("HELO", "home.com", 250))
         return false;
   }
   else
   {
      encodeBase64(is_FromEmailID, encodedEmailID);
      encodeBase64(is_FromPassword, encodedPassword);

      if (!SendHeader("EHLO", is_HostName.c_str(), 250))
         return false;
      if (ib_SMTPVerification)
      {
         if (!SendHeader("VRFY", is_FromEmailID.c_str(), 250))
            return false;
      }
      else
      {
	 if (!strlen(is_FromPassword.c_str()))
	    return false;
         if (!SendHeader("AUTH LOGIN", 0, 334))
            return false;
         if (!SendHeader(encodedEmailID, 0, 334))
            return false;
         if (!SendHeader(encodedPassword, 0, 235))
            return false;
      }
   }

   if (!SendHeader("MAIL From:", is_FromEmailID.c_str(), 250))
      return false;

   if (!SendHeader("RCPT To:", toEmailID.c_str(), 250))
      return false;

   if (is_FromEmailID != toEmailID)
      if (!SendHeader("RCPT To:", is_FromEmailID.c_str(), 250))
         return false;

   for (i=0; i<ii_DestIDCount; i++)
      if (is_DestID[i] != toEmailID)
         if (!SendHeader("RCPT To:", is_DestID[i].c_str(), 250))
            return false;

   if (!SendHeader("DATA", "", 354))
      return false;

   return true;
}

// This presumes that outString is large enough to hold the encrypted string
// and has been initialized to all zeros
void     Mail::encodeBase64(string inString, string &outString)
{
   const char    *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   char     array3[3], array4[4];
   int32_t  i, j, bytes, index;
   char     tempIn[200], tempOut[200];
   char    *tempPtr;

   strcpy(tempIn, inString.c_str());
   memset(tempOut, 0x00, sizeof(tempOut));
   tempPtr = tempIn;

   i = j = index = 0;
   bytes = (int32_t) strlen(tempIn);
   while (bytes--)
   {
       array3[i++] = *(tempPtr++);
       if (i == 3)
       {
         array4[0] = (array3[0] & 0xfc) >> 2;
         array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
         array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
         array4[3] = array3[2] & 0x3f;

         for (i=0; i<4; i++)
            tempOut[index++] = base64[(int) array4[i]];
         i = 0;
      }
   }

   if (i)
   {
      for(j=i; j<3; j++)
         array3[j] = '\0';

      array4[0] = (array3[0] & 0xfc) >> 2;
      array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
      array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
      array4[3] = array3[2] & 0x3f;

      for (j = 0; (j < i + 1); j++)
         outString[index++] = base64[(int) array4[j]];

      while (i++ < 3)
         tempOut[index++] = '=';
   }

   outString = tempOut;
}
