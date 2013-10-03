#ifndef _client_

#define _client_

#include "defs.h"
#include "Log.h"
#include "TestingProgramFactory.h"

// Return options
#define RO_UNDEFINED       0
#define RO_ALL             1           // Return all workunits, even if in progress or not started
#define RO_COMPLETED       2           // Return workunits that are done
#define RO_IFDONE          3           // Return all only if all workunits are completed

typedef struct
{
   Log     *p_Log;
   TestingProgramFactory *p_TestingProgramFactory;

   string   s_EmailID;
   string   s_UserID;
   string   s_MachineID;
   string   s_InstanceID;
   string   s_TeamID;
} globals_t;

#endif // #ifndef _client_
