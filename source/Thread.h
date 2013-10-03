#ifndef  _Thread_
#define  _Thread_

#include <string>
#include "defs.h"

#ifndef WIN32
#include <pthread.h>
#endif

class Thread
{
public:
   virtual ~Thread() {};

protected:

#ifndef WIN32
   pthread_t   ih_thread;
#endif
};

#endif
