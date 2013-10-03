#ifndef _HelperThread_

#define _HelperThread_

#include "defs.h"
#include "server.h"
#include "Thread.h"
#include "HelperSocket.h"
#include "Log.h"
#include "DBInterface.h"

class HelperThread : public Thread
{
public:
   void     StartThread(globals_t *theGlobals, HelperSocket *theSocket);

   void     HandleClient(void);

   void     ReleaseResources(void);

protected:
   Log              *ip_Log;
   HelperSocket     *ip_Socket;
   DBInterface      *ip_DBInterface;
   SharedMemoryItem *ip_ClientCounter;
   globals_t        *ip_Globals;

   string         is_HTMLTitle;
   string         is_ProjectTitle;
   string         is_SortLink;
   string         is_CSSLink;
   string         is_AdminPassword;
   int32_t        ii_ServerType;

   string         is_UserID;
   string         is_EmailID;
   string         is_MachineID;
   string         is_InstanceID;
   string         is_TeamID;

   void           SendHTML(string theMessage);
   bool           VerifyAdminPassword(string password);
   void           SendGreeting(string fileName);
   void           AdminStatsUpdate(bool userStats);
   virtual void   ProcessRequest(string theMessage) {};
};

#endif // #ifndef _HelperThread_

