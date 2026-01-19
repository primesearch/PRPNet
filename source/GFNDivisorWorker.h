#ifndef _GFNDivisorWorker_

#define _GFNDivisorWorker_

#include "Worker.h"

class GFNDivisorWorker : public Worker
{
public:
   GFNDivisorWorker(Log* theLog, TestingProgramFactory* testingProgramFactory,
      ClientSocket* theSocket, int32_t maxWorkUnits, string workSuffix);

   bool	    ProcessWorkUnit(int32_t& divisorsFound, bool inProgressOnly, double& seconds);

protected:
   // Get work from a specific socket in the pool
   bool     GetWork(void);

   // Return work to a specific socket in the pool
   void     ReturnWork(uint32_t returnOption);

   bool     ReturnWorkUnit(workunit_t* wu, bool completed);

   void	   Load(string saveFileName);
   void	   Save(FILE* fPtr);

   bool     IsWorkUnitCompleted(workunit_t* wu);

   bool     IsWorkUnitInProgress(workunit_t* wu);

private:
   int32_t  ii_ServerType;
   string   is_ServerVersion;
};

#endif // #ifndef _GFNDivisorWorker_
