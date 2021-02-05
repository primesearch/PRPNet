/* SharedMemoryItem.h -- (C) Mark Rodenkirch, February 2012

   Header for class that manages mutexes.
   
*/

#ifndef  _SharedMemoryItem_
#define  _SharedMemoryItem_

#include <string>
#include "defs.h"

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

class SharedMemoryItem
{
public:
   SharedMemoryItem(string itemName, bool withCondition = false);

   ~SharedMemoryItem(void);

   // returns true if mutex is locked
   void        Lock(void);

   // returns true if mutex is locked
   bool        TryLock(void);

   void        Release(void);

   // These assume that the mutex is locked
   int64_t     GetValueHaveLock(void) { return il_Value; };
   void        SetValueHaveLock(int64_t newValue) { il_Value = newValue; };

   // These will lock/unlock the mutex while getting/setting the value
   int64_t     GetValueNoLock(void);
   void        SetValueNoLock(int64_t newValue);

   // These will lock/unlock the mutex while updating the value
   void        IncrementValue(int64_t increment = 1);
   void        DecrementValue(int64_t decrement = 1);

   void        SetCondition(void);
   void        ClearCondition(void);

private:
   string      is_ItemName;
   bool        ib_HasCondition;
   int64_t     il_Value;

#ifdef WIN32
   uint32_t    ii_CountWaiting;
   CRITICAL_SECTION    im_CriticalSection;
   LPCRITICAL_SECTION  ih_CriticalSection;
   HANDLE              ih_Condition;
#else
   pthread_mutex_t     ih_PthreadMutex;
   pthread_mutexattr_t ih_PthreadMutexAttr;
   pthread_cond_t      ih_Condition;
#endif
};

#endif
