#include "WorkReceiver.h"
#include "StatsUpdaterFactory.h"

WorkReceiver::WorkReceiver(DBInterface *dbInterface, Socket *theSocket, globals_t *globals,
                           string userID, string emailID, 
                           string machineID, string instanceID, string teamID)
{
   ip_DBInterface = dbInterface;
   ip_Socket = theSocket;
   ip_Log = globals->p_Log;

   is_UserID = userID;
   is_EmailID = emailID;
   is_MachineID = machineID;
   is_InstanceID = instanceID;
   is_TeamID = teamID;

   ii_ServerType = globals->i_ServerType;
   ib_ShowOnWebPage = globals->b_ShowOnWebPage;
   ib_NeedsDoubleCheck = globals->b_NeedsDoubleCheck;
   ib_BriefTestLog = globals->b_BriefTestLog;

   StatsUpdaterFactory *suf;

   suf = new StatsUpdaterFactory();
   ip_StatsUpdater = suf->GetInstance(ip_DBInterface, ip_Log, ii_ServerType, ib_NeedsDoubleCheck);
   delete suf;
}

WorkReceiver::~WorkReceiver()
{
   delete ip_StatsUpdater;
}
