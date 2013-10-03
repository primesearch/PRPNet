#ifndef _WorkSender_

#define _WorkSender_

#include <string>
#include "defs.h"
#include "DBInterface.h"
#include "Log.h"
#include "Socket.h"
#include "server.h"
#include "SharedMemoryItem.h"

class WorkSender
{
public:
   WorkSender(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
              string userID, string emailID, string machineID,
              string instanceID, string teamID);

   virtual ~WorkSender() {};

protected:
   string      is_ProjectTitle;
   string      is_UserID;
   string      is_EmailID;
   string      is_MachineID;
   string      is_InstanceID;
   string      is_TeamID;
   int32_t     ii_MaxWorkUnits;
   int32_t     ii_ServerType;
   bool        ib_BriefTestLog;
   bool        ib_NeedsDoubleCheck;
   bool        ib_NoNewWork;
   int32_t     ii_DoubleChecker;
   string      is_ClientVersion;
   DBInterface *ip_DBInterface;
   Socket      *ip_Socket;
   Log         *ip_Log;
   SharedMemoryItem *ip_Locker;

   void        SendGreeting(string fileName);
};

#endif // #ifdef _WorkSender_
