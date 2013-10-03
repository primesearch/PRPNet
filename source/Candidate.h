#ifndef _Candidate_

#define _Candidate_

#include "Mail.h"
#include "Socket.h"
#include "gfn.h"
#include "DBInterface.h"

typedef struct
{
   uint64_t l_TestID;
   char     s_Program[50];
   char     s_Prover[50];
   char     s_Residue[20];
   char     s_TwinProgram[50];
   char     s_TwinProver[50];
   char     s_TwinResidue[20];
   char     s_EmailID[200];
   char     s_UserID[200];
   char     s_ClientID[200];
   char     s_GeneferROE[100];
   uint32_t i_SearchedForGFNDivisors;
   void    *m_Next;
} test_t;

#define ERROR_DUPLICATE -10
#define ERROR_INVALID -11

class Candidate
{
public:
   Candidate(Log *theLog, uint32_t serverType, char *name, char *inputLine, uint32_t needsDoubleCheck);

   Candidate(Log *theLog, uint32_t serverType, uint64_t k, uint32_t b, uint32_t n, int32_t c, uint32_t needsDoubleCheck);

   ~Candidate();

   int32_t  WriteToDB(DBInterface *dbInterface);

   // Indicates if this candidate is active
   uint32_t IsActive() { return b_IsActive; } ;

   // Returns the name of the candidate to test
   char    *GetName() { return s_Name; } ;

   // Returns the length of the candidate to test
   uint32_t GetDecimalLength() { return i_DecimalLength; } ;

   // Set the length of the candidate to test, only used for Primorials and Factorials
   // or other forms which require more effort to determine their length
   void     SetDecimalLength(uint32_t decimalLength) { i_DecimalLength = decimalLength; } ;

   uint64_t GetK() { return l_k; } ;
   uint32_t GetB() { return i_b; } ;
   uint32_t GetN() { return i_n; } ;
   int32_t  GetC() { return i_c; } ;

   // Returns the time of the last update to this candidate
   uint64_t GetLastUpdateTime() { return l_LastUpdateTime; } ;

   // The number of PRP tests issued on this candidate
   uint32_t CountOfIssuedTests() { return i_IssuedTests; } ;

   // The number of PRP tests completed this candidate
   uint32_t CountOfCompletedTests() { return i_CompletedTests; } ;

   // Return a flag indicating if the candidate needs another test
   uint32_t NeedsDoubleCheck() { return (b_NeedsDoubleCheck && !(b_IsPRP || b_IsPrime)); } ;

   // Return a flag indicating if the candidate is PRP
   uint32_t IsPRP() { return b_IsPRP; } ;

   // Return a flag indicating if the candidate is Prime
   uint32_t IsPrime() { return b_IsPrime; } ;

   // Receive work done by a client via the given socket
   int32_t  ReceiveWorkDone(Socket *theSocket, uint64_t testID, uint32_t isBuffered,
                            char *fromEmailID, char *inetAddress,
                            char *fromUserID, char *fromClientID);

   // Send work to a client via the given socket
   int32_t  SendWorkToDo(Socket *theSocket, char *clientVersion,
                         char *fromEmailID, char *inetAddress,
                         char *fromUserID, char *fromClientID);

   // Send an email stating that the candidate is PRP
   void     SendPRPFoundEmail(Mail *theMail, char *toEmailID, uint32_t duplicate);

   // Set this candidate to active
   uint32_t SetActive(uint32_t active);

   // Determine if user/client has already done a test on this number;
   uint32_t TestedByPersonAndClient(char *emailID, char *clientID);
   uint32_t TestedByPerson(char *emailID);
   uint32_t TestedByClient(char *clientID);

   void     Save(FILE *fPtr);

   // Add test results from a file
   int32_t  AddTest(char *testInfo);

   // Add versions of genefer with a round off error.
   int32_t  AddGeneferROE(char *geneferList);

   // Return an indication if any work has been sent out
   uint32_t HasTest(void) { return (m_Test == 0); }

   uint32_t GetGFNDivisorCount(void) { return i_GFNDivisors; }

   uint32_t HasPendingTest(void);

   void     ExpireOldTests(uint64_t cutoffTime);

   uint32_t HasValidForm(void) { return b_HasValidForm; };

   uint32_t HadRoundoffError(char *geneferName);

   void     SendPendingTestHTML(Socket *theSocket);

private:
   // Add test results given the details
   int32_t  AddTest(uint64_t testID, char *program, char *residue,
                    char *emailID, char *userID, char *clientID);

   // Add a GFN divisor to the linked list
   void     AddGFNToList(char *divisor);

   void     LogCompletedTest(test_t *theTest, char *inetAddress);

   // The type of server
   uint32_t i_ServerType;

   // Unique name to identify the number
   char     s_Name[50];

   // Indicates if the number is in a valid form and can be activated
   uint32_t b_HasValidForm;

   uint32_t i_GFNDivisors;

   // If this is set, then k, b, n, c are all 0
   uint32_t i_UsePFGW;

   // The k, b, n, and c values for the number
   uint64_t l_k;
   uint32_t i_b;
   uint32_t i_n;
   int32_t  i_c;

   // An approximate decimal length of the candiate number
   uint32_t i_DecimalLength;

   // This controls whether or not a number is active.
   uint32_t b_IsActive;

   // This is the last time the Candidate was updated by SendWorkToDone
   // or ReceiveWorkDone
   uint64_t l_LastUpdateTime;

   // A pointer to an instance of the Log class
   Log     *m_Log;

   // The number of entries in m_Test;
   uint32_t i_IssuedTests;

   // The number of completed tests;
   uint32_t i_CompletedTests;

   // Whether or not a double-check is needed
   uint32_t b_NeedsDoubleCheck;

   // Whether or not the number is PRP
   uint32_t b_IsPRP;

   // Whether or not the number is Prime
   uint32_t b_IsPrime;

   // Whether or not the twin is PRP
   uint32_t b_IsTwinPRP;

   // Whether or not the twin is Prime
   uint32_t b_IsTwinPrime;

   // This is a linked list of PRP test results.
   test_t   *m_Test;

   // This is a list of GFN divisors
   gfn_t    *m_FirstGFN;
};

#endif // #ifndef _Candidate_

