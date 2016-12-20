#ifndef _server_

#define _server_

#include "defs.h"
#include "Log.h"
#include "Socket.h"
#include "ServerSocket.h"
#include "SharedMemoryItem.h"

// Values for sortoption
#define SO_OLDEST      'A'
#define SO_LENGTH      'L'
#define SO_BKCN        'K'
#define SO_NKBC        'N'

// Values for doublechecker
#define DC_DIFFBOTH    1
#define DC_DIFFEMAIL   2
#define DC_DIFFMACHINE 3
#define DC_ANY         4

typedef struct
{
   double    minLength;
   double    maxLength;
   int64_t   doubleCheckDelay;
   int64_t   expireDelay;
} delay_t;

typedef struct
{
   int32_t  i_PortID;
   int32_t  i_ServerType;
   int32_t  i_MaxWorkUnits;
   int32_t  i_MaxClients;
   int32_t  i_DoubleChecker;
   bool     b_NeedsDoubleCheck;
   bool     b_ShowOnWebPage;
   bool     b_UseLLROverPFGW;
   bool     b_OneKPerInstance;
   bool     b_LocalTimeHTML;
   bool     b_BriefTestLog;
   bool     b_ServerStatsSummaryOnly;
   int32_t  i_DelayCount;
   int32_t  i_UnhidePrimeHours;
   int32_t  i_DestIDCount;
   int32_t  i_SpecialThreshhold;
   int32_t  i_NotifyLowWork;
   delay_t *p_Delay;
   string   s_SortSequence;
   string   s_HTMLTitle;
   string   s_ProjectTitle;
   string   s_SortLink;
   string   s_CSSLink;
   string   s_AdminPassword;
   string   s_ProjectName;
   string   s_EmailID;
   string   s_EmailPassword;
   string   s_DestID[10];
   Log     *p_Log;

   ServerSocket     *p_ServerSocket;

   SharedMemoryItem *p_Quitting;
   SharedMemoryItem *p_ClientCounter;
   SharedMemoryItem *p_Locker;
   bool     b_NoExpire;
   bool     b_NoNewWork;
} globals_t;

#endif // #ifndef _server_
