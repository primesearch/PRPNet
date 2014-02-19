#ifndef _CandidateTestResult_

#define _CandidateTestResult_

#include "defs.h"
#include "DBInterface.h"
#include "SQLStatement.h"
#include "Log.h"
#include "StatsUpdater.h"

class CandidateTestResult;

class CandidateTestResult
{
public:
   CandidateTestResult(DBInterface *dbInterface, Log *theLog, string candidateName,
                       int64_t testID, int32_t testIndex, bool briefTestLog,
                       string userID, string emailID, string machineID, string instanceID, string teamID);

   ~CandidateTestResult(); 

   bool        ProcessChildMessage(string socketMessage);
   bool        ProcessMessage(string socketMessage);

   result_t    GetTestResult(void)      { return ir_TestResult;       };
   bool        HadSQLError(void)        { return ib_HaveSQLError;     };
   bool        HadRoundOffError(void)   { return ib_HadRoundOffError; };
   string      GetResidue(void)         { return is_Residue;          };
   int32_t     GetGFNDivisorCount(void) { return ii_GFNDivisorCount;  };
   string      GetProgramVersion(void)  { return is_ProgramVersion;   };
   string      GetProgram(void)         { return is_Program;   };

   void        LogResults(int32_t socketID, int32_t completedTests, bool needsDoubleCheck,
                          bool showOnWebPage, double decimalLength);
   void        LogResults(int32_t socketID, CandidateTestResult *mainTestResult,
                          bool showOnWebPage, double decimalLength);
   void        LogTwinResults(int32_t socketID, CandidateTestResult *mainTestResult);
   void        LogSophieGermainResults(int32_t socketID, CandidateTestResult *mainTestResult, sgtype_t sgType);

private:
   DBInterface   *ip_DBInterface;
   Log           *ip_Log;

   int64_t        ii_TestID;
   int32_t        ii_TestIndex;
   string         is_ParentName;
   string         is_ChildName;
   string         is_UserID;
   string         is_EmailID;
   string         is_MachineID;
   string         is_InstanceID;
   string         is_TeamID;
   string         is_WhichTest;
   string         is_Residue;
   string         is_Program;
   string         is_Prover;
   string         is_ProgramVersion;
   string         is_ProverVersion;
   double         id_SecondsForTest;
   int32_t        ii_GFNDivisorCount;
   bool           ib_CheckedGFNDivisibility;
   bool           ib_HadRoundOffError;
   bool           ib_HaveSQLError;
   bool           ib_BriefTestLog;
   result_t       ir_TestResult;

   void        UpdateTest(void);
   void        InsertTestResult(void);
   void        InsertGFNDivsior(string gfn);
   void        InsertGeneferROE(string geneferVersion);
   void        InsertUserPrime(double decimalLength, bool showOnWebPage);

   void        LogTwinMessage(Log *testLog, Log *prpLog, string logHeader, string prover,
                              string residue, string doubleCheck);

   void        LogSophieGermainMessage(Log *testLog, Log *prpLog, string logHeader,
                                       string prover, string residue, string doubleCheck,
                                       string inResidue, string form);
};

#endif // #ifndef _CandidateTestResult_

