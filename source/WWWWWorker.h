#ifndef _WWWWWorker_

#define _WWWWWorker_

#include "Worker.h"

class WWWWWorker : public Worker
{
public:
   WWWWWorker(Log *theLog, TestingProgramFactory *testingProgramFactory,
              ClientSocket *theSocket, int32_t maxWorkUnits, string workSuffix);

   bool	    ProcessWorkUnit(int32_t &specialsFound, bool inProgressOnly, double &seconds);

protected:
   // Get work from a specific socket in the pool
   bool     GetWork(void);

   // Return work to a specific socket in the pool
   void     ReturnWork(uint32_t returnOption);

   bool     ReturnWorkUnit(workunit_t *wu, bool completed);

   uint32_t TestWorkUnit(workunit_t *wu, uint32_t isTwinTest);

   void	   Load(string saveFileName);
   void	   Save(FILE *fPtr);

   bool     IsWorkUnitCompleted(workunit_t *wu);

   bool     IsWorkUnitInProgress(workunit_t *wu);

private:
   int32_t  ii_ServerType;
   string   is_ServerVersion;
   int32_t  ii_SpecialThreshhold;
};

#endif // #ifndef _WWWWWorker_
