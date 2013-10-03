#include <string.h>
#include <ctype.h>
#include <math.h>
#include "Candidate.h"
#include "SQLStatement.h"

Candidate::Candidate(Log *theLog, uint32_t serverType, char *name, char *inputLine, uint32_t needsDoubleCheck)
{
   int32_t dummy;
   uint32_t nShift;
   char    active[15], tempName[40];

   m_Log = theLog;
   b_NeedsDoubleCheck = needsDoubleCheck;
   b_IsPRP = b_IsPrime = 0;
   b_IsTwinPRP = b_IsTwinPrime = 0;
   i_IssuedTests = i_CompletedTests = 0;
   b_HasValidForm = 1;
   b_IsActive = 1;
   i_ServerType = serverType;
   m_FirstGFN = 0;
   i_GFNDivisors = 0;

   strcpy(s_Name, name);
   sprintf(tempName, "%sx1", name);

   switch (i_ServerType)
   {
      case ST_PRIMORIAL:
         if (sscanf(tempName, "%u#%d", &i_n, &i_c) != 2)
            b_HasValidForm = 0;
         l_k = i_b = i_n;
         break;

      case ST_FACTORIAL:
         if (sscanf(tempName, "%d!%d", &i_n, &i_c) != 2)
            b_HasValidForm = 0;
         l_k = i_b = i_n;
         break;

      case ST_GFN:
         if (sscanf(tempName, "%u^%u+1", &i_b, &i_n) != 2)
            b_HasValidForm = 0;

         if (i_b & 1)
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: b must be even.  The candidate has been deactivated", s_Name);
         }
         nShift = i_n;
         while (nShift)
         {
            if (nShift & 1 && nShift > 1)
            {
               b_HasValidForm = 0;
               m_Log->LogMessage("%s: n must be a power of 2.  The candidate has been deactivated", s_Name);
               nShift = 0;
            }
            nShift >>= 1;
         }
         l_k = i_c = 1;
         break;

      default:
         if (sscanf(tempName, "%llu*%u^%u%dx%d", &l_k, &i_b, &i_n, &i_c, &dummy) != 5) 
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: The candidate could not be parsed.  The candidate has been deactivated", s_Name);
         }

         if (l_k < 2 || i_b < 2 || i_n < 2)
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: k, b, and n must be greater than 1.  The candidate has been deactivated", s_Name);
         }

         if ((i_b & 0x01) && (l_k & 0x01))
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: The candidate number is even.  The candidate has been deactivated", s_Name);
         }

         if (i_c != 1 && i_c != -1)
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: c must be +1 or -1.  The candidate has been deactivated", s_Name);
         }
   }

   // Support 1.x and 2.x formats
   if ((sscanf(inputLine, "%llu %d %s", &l_LastUpdateTime, &dummy, active) != 3) &&
       (sscanf(inputLine, "%llu %s", &l_LastUpdateTime, active) != 2))
      l_LastUpdateTime = 0;
   else
      if (!strcmp(active, "inactive"))
         b_IsActive = 0;

   if (!b_HasValidForm)
      b_IsActive = 0;

   m_Test = 0;
}

Candidate::Candidate(Log *theLog, uint32_t serverType, uint64_t k, uint32_t b, uint32_t n, int32_t c, uint32_t needsDoubleCheck)
{
   m_Log = theLog;
   b_NeedsDoubleCheck = needsDoubleCheck;
   b_IsPRP = b_IsPrime = 0;
   b_IsTwinPRP = b_IsTwinPrime = 0;
   i_IssuedTests = i_CompletedTests = 0;
   b_HasValidForm = 1;
   i_ServerType = serverType;
   m_FirstGFN = 0;
   i_GFNDivisors = 0;

   l_k = k;
   i_b = b;
   i_n = n;
   i_c = c;

   switch (i_ServerType)
   {
      case ST_PRIMORIAL:
         sprintf(s_Name, "%u#%+d", i_n, i_c);
         l_k = i_b = i_n;
         break;

      case ST_FACTORIAL:
         sprintf(s_Name, "%u!%+d", i_n, i_c);
         l_k = i_b = i_n;
         break;

      case ST_GFN:
         sprintf(s_Name, "%u^%u+1", i_b, i_n);
         l_k = i_c = 1;
         break;

      default:
         sprintf(s_Name, "%llu*%u^%u%+d", l_k, i_b, i_n, i_c);
 
         if (l_k < 2 || i_b < 2 || i_n < 2)
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: k, b, and n must be greater than 1.  The candidate has been deactivated", s_Name);
         }

         if ((i_b & 0x01) && (l_k & 0x01))
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: The candidate number is even.  The candidate has been deactivated", s_Name);
         }

         if (i_c != 1 && i_c != -1)
         {
            b_HasValidForm = 0;
            m_Log->LogMessage("%s: c must be +1 or -1.  The candidate has been deactivated", s_Name);
         }
         break;
   }

   b_IsActive = b_HasValidForm;

   l_LastUpdateTime = 0;
   m_Test = 0;
}

Candidate::~Candidate()
{
   test_t *tPtr;

   tPtr = m_Test;
   while (m_Test)
   {
      m_Test = (test_t *) tPtr->m_Next;

      delete tPtr;
      tPtr = m_Test;
   }
}

// Set the candidate to active if it doesn't have a known factor.
uint32_t Candidate::SetActive(uint32_t active)
{
   if (active)
   {
      if (!b_HasValidForm)
      {
         m_Log->LogMessage("%s: Candidate is not of the form k*2^n+1 or k*2^n-1 and cannot be activated", s_Name);
         return 0;
      }

      if (b_IsPRP || b_IsPrime)
      {
         m_Log->LogMessage("%s: Candidate is PRP/Prime and cannot be activated", s_Name);
         return 0;
      }
   }

   b_IsActive = active;
   return 1;
}

int32_t   Candidate::AddTest(uint64_t testID, char *program, char *residue,
                             char *emailID, char *userID, char *clientID)
{
   test_t   *tPtr;

   if (!m_Test)
   {
      m_Test = new test_t;
      tPtr = m_Test;
   }
   else
   {
      tPtr = m_Test;
      while (tPtr)
      {
         // We already know about this test, so ignore this result
         if (testID == tPtr->l_TestID &&
             !strcmp(emailID, tPtr->s_EmailID) && 
             !strcmp(clientID, tPtr->s_ClientID))
            return RC_FAILURE;

         // We have two tests with the same residue, thus no more double-checking is needed
         if (strcmp(tPtr->s_Residue, "inprogress") && !strcmp(residue, tPtr->s_Residue))
            b_NeedsDoubleCheck = false;

         tPtr = (test_t *) tPtr->m_Next;
      }

      if (!tPtr)
      {
         tPtr = m_Test;
         while (tPtr->m_Next)
            tPtr = (test_t *) tPtr->m_Next;

         tPtr->m_Next = new test_t;
         tPtr = (test_t *) tPtr->m_Next;
         tPtr->m_Next = 0;
      }
   }

   tPtr->l_TestID = testID;
   strcpy(tPtr->s_Program, program);
   strcpy(tPtr->s_Prover, "na");
   strcpy(tPtr->s_GeneferROE, "");
   strcpy(tPtr->s_Residue, residue);
   strcpy(tPtr->s_EmailID, emailID);
   strcpy(tPtr->s_UserID, userID);
   strcpy(tPtr->s_ClientID, clientID);
   strcpy(tPtr->s_TwinResidue, "na");
   tPtr->i_SearchedForGFNDivisors = false;
   tPtr->m_Next = 0;

   // Tests with round off errors from genefer do not count as completed
   if (strcmp(tPtr->s_Residue, "inprogress") && strcmp(residue, "roundofferr"))
      i_CompletedTests++;

   i_IssuedTests++;

   return RC_SUCCESS;
}

// Add a test as parsed from the save file
int32_t  Candidate::AddTest(char *testInfo)
{
   char     program[50], residue[100], prover[100], emailID[100], userID[100], clientID[100], *ptr;
   uint64_t testID;
   uint32_t isPrime;

   if (sscanf(testInfo, "%llu %s %s %s %s %s %s", &testID, emailID, userID, clientID, program, residue, prover) != 7)
   {
      if (sscanf(testInfo, "%llu %s %s %s %s", &testID, emailID, clientID, program, residue) != 5)
      {
         m_Log->LogMessage("%s: Result cannot be parsed from line %s", s_Name, testInfo);
         return RC_FAILURE;
      }
      isPrime = !strcmp(residue, "PRIME");

      if (isPrime)
         strcpy(prover, program);
      else
         strcpy(prover, "na");

      strcpy(userID, emailID);
      ptr = strchr(userID, '@');
      *ptr = 0;
   }

   return AddTest(testID, program, residue, emailID, userID, clientID);
}

// Add a test as parsed from the save file
int32_t  Candidate::AddGeneferROE(char *geneferList)
{
   uint64_t testID;
   char    *pos;
   test_t  *tPtr;

   if (sscanf(geneferList, "%llu", &testID) != 1)
      return RC_FAILURE;

   pos = strchr(geneferList, ' ');

   if (pos)
   {
      tPtr = m_Test;
      while (tPtr)
      {
         if (testID == tPtr->l_TestID)
         {
            strcpy(tPtr->s_GeneferROE, pos+1);
            return RC_SUCCESS;
         }

         tPtr = (test_t *) tPtr->m_Next;
      }
   }

   return RC_FAILURE;
}

uint32_t Candidate::TestedByPersonAndClient(char *emailID, char *clientID)
{
   test_t   *tPtr;

   tPtr = m_Test;

   while (tPtr)
   {
      if (!strcmp(emailID, tPtr->s_EmailID) &&
          !strcmp(clientID, tPtr->s_ClientID))
         return true;

      tPtr = (test_t *) tPtr->m_Next;
   }

   return false;
}

uint32_t Candidate::TestedByPerson(char *emailID)
{
   test_t   *tPtr;

   tPtr = m_Test;

   while (tPtr)
   {
      if (!strcmp(emailID, tPtr->s_EmailID))
         return true;

      tPtr = (test_t *) tPtr->m_Next;
   }

   return false;
}

uint32_t Candidate::TestedByClient(char *clientID)
{
   test_t   *tPtr;

   tPtr = m_Test;

   while (tPtr)
   {
      if (!strcmp(clientID, tPtr->s_ClientID))
         return true;

      tPtr = (test_t *) tPtr->m_Next;
   }

   return false;
}

int32_t  Candidate::SendWorkToDo(Socket *theSocket, char *clientVersion,
                                 char *fromEmailID, char *inetAddress,
                                 char *fromUserID, char *fromClientID)
{
   l_LastUpdateTime = time(NULL);

   if (memcmp(clientVersion, "2", 1) >= 0)
      if (memcmp(clientVersion, "2.4", 3) < 0)
         theSocket->Send("ServerType: %d", i_ServerType);

   if (USE_PFGW(i_ServerType))
      theSocket->Send("WorkUnit: %s %llu %u %d", s_Name, l_LastUpdateTime, i_n, i_c);
   else if (i_ServerType == ST_GFN)
      theSocket->Send("WorkUnit: %s %llu %u %u", s_Name, l_LastUpdateTime, i_b, i_n);
   else
      theSocket->Send("WorkUnit: %s %llu %llu %u %u %d", s_Name, l_LastUpdateTime, l_k, i_b, i_n, i_c);

   AddTest(l_LastUpdateTime, "na", "inprogress", fromEmailID, fromUserID, fromClientID);

   m_Log->LogMessage("%s sent to Email: %s  User: %s  Client: %s", s_Name, fromEmailID, fromUserID, fromClientID);

   return RC_SUCCESS;
}

void    Candidate::LogCompletedTest(test_t *theTest, char *inetAddress)
{
   Log      *prpLog, *testLog;
   test_t   *tPtr;
   char      doubleCheck[30], logHeader[200], residue[50], twinResidue[50], prover[50];
   char     *ptr1, *ptr2;

   if (b_NeedsDoubleCheck)
      sprintf(doubleCheck, "DoubleCheck? %s", ((i_CompletedTests > 0) ? "Yes" : "No"));
   else
      *doubleCheck = 0;

   sprintf(logHeader, "%s received by Email: %s  User: %s  Client: %s Program: %s",
           s_Name, theTest->s_EmailID, theTest->s_UserID, theTest->s_ClientID, theTest->s_Program);

   sprintf(prover,      "Prover: %s",       theTest->s_Prover);
   sprintf(residue,     "Residue: %s",      theTest->s_Residue);
   sprintf(twinResidue, "Twin Residue: %s", theTest->s_TwinResidue);

   if (!strcmp(theTest->s_Residue,     "PRP"))   b_IsPRP       = true;
   if (!strcmp(theTest->s_Residue,     "PRIME")) b_IsPrime     = true;
   if (!strcmp(theTest->s_TwinResidue, "PRP"))   b_IsTwinPRP   = true;
   if (!strcmp(theTest->s_TwinResidue, "PRIME")) b_IsTwinPrime = true;

   testLog = new Log(0, "completed_tests.log", 0, true);
   if (b_IsPRP || b_IsPrime)
   {
      b_IsActive = 0;

      prpLog = new Log(0, "PRP.log", 0, false);

      if (i_ServerType == ST_TWIN)
      {
         if (b_IsPrime && b_IsTwinPrime)
         {
            prpLog->LogMessage("%s: Twin Primes returned by %s!", s_Name, theTest->s_EmailID);
            testLog->LogMessage("%s  %s  %s  is a Twin Prime", logHeader, prover, residue);
         }
         else
         {
            if (b_IsTwinPRP || b_IsTwinPrime)
            {
               prpLog->LogMessage("%s: Potential Twin Primes returned by %s (%s).  Primality has not been proven!", s_Name, theTest->s_EmailID);
               testLog->LogMessage("%s  %s  %s  is a Potential Twin Prime", logHeader, prover, residue);
            }
            else
               if (b_IsPRP)
               {
                  prpLog->LogMessage("%s: PRP returned by %s.  The twin is composite.", s_Name, theTest->s_EmailID);
                  testLog->LogMessage("%s  %s  %s  (is not a twin)", logHeader, residue, doubleCheck);
               }
               else
               {
                  prpLog->LogMessage("%s: Prime returned by %s.  The twin is composite.", s_Name, theTest->s_EmailID);
                  testLog->LogMessage("%s  %s  %s  %s  (is not a twin)", logHeader, prover, residue, doubleCheck);
               }
         }
      }
      else
      {
         if (b_IsPRP)
         {
            prpLog->LogMessage("%s: PRP returned by %s using %s!",
                               s_Name, theTest->s_EmailID, theTest->s_Program);
            testLog->LogMessage("%s  %s  %s", logHeader, residue, doubleCheck);
         }
         else
         {
            prpLog->LogMessage("%s: Prime returned by %s using %s (%s)!",
                               s_Name, theTest->s_EmailID, theTest->s_Program, prover);
            testLog->LogMessage("%s  %s  %s  %s", logHeader, prover, residue, doubleCheck);
         }
      }

      delete prpLog;
   }
   else
      testLog->LogMessage("%s  %s  %s", logHeader, residue, doubleCheck);

   delete testLog;

   // If genefer had a round off error, then this cannot count as a double-check
   if (!strcmp(theTest->s_Residue, "roundofferr"))
      return;

   // Older versions of PFGW do not have leading zeros, so we need to 
   // find the first non-zero in the residue before we compare them.
   ptr1 = theTest->s_Residue;
   while (*ptr1 == '0')
      ptr1++;

   // Set the double-check flag as necessary
   tPtr = m_Test;
   while (tPtr)
   {
      if (theTest->l_TestID == tPtr->l_TestID &&
          !strcmp(theTest->s_EmailID, tPtr->s_EmailID) && 
          !strcmp(theTest->s_ClientID, tPtr->s_ClientID))
         ;
      else
      {
         ptr2 = tPtr->s_Residue;
         while (*ptr2 == '0')
            ptr2++;

         // PFGW residues are only 62-bits, while phrot and LLR are 64-bits,
         // thus ignore the first 4 bits of the residues when comparing.
         if (!strcmp(ptr1+1, ptr2+1))
            b_NeedsDoubleCheck = false;
      }

      tPtr = (test_t *) tPtr->m_Next;
   }
}

int32_t  Candidate::ReceiveWorkDone(Socket *theSocket, uint64_t testID, uint32_t isBuffered,
                                    char *fromEmailID, char *inetAddress,
                                    char *fromUserID, char *fromClientID)
{
   Log      *gfnLog;
   test_t   *tPtr;
   gfn_t    *gfnPtr;
   char     *theMessage, program[50], residue[20], prover[50];
   char      twinProgram[50], twinResidue[20], twinProver[50];
   char      standardizedNames[100];
   uint32_t  i, abandoned = false, searchedForGFNDivisors = false;

   tPtr = m_Test;

   while (tPtr)
   {
      if (testID == tPtr->l_TestID &&
          !strcmp(fromEmailID, tPtr->s_EmailID) && 
          !strcmp(fromClientID, tPtr->s_ClientID))
         break;

      tPtr = (test_t *) tPtr->m_Next;
   }

   *prover = 0;
   *residue = 0;
   *program = 0;
   *twinProver = 0;
   *twinResidue = 0;
   *twinProgram = 0;
   *standardizedNames = 0;

   theMessage = theSocket->Receive();
   while (theMessage)
   {
      if (!strcmp(theMessage, "End of WorkUnit"))
         break;

      if (!strcmp(theMessage, "Test Abandoned"))
         abandoned = true;
      else if (!strcmp(theMessage, "GFN Test Done"))
         searchedForGFNDivisors = true;
      else if (!memcmp(theMessage, "StandardizedName: ", 18))
         strcpy(standardizedNames, theMessage + 18);
      else if (!memcmp(theMessage, "Test Result: ", 13))
         sscanf(theMessage, "Test Result: %s %s", program, residue);
      else if (!memcmp(theMessage, "Twin Result: ", 13))
         sscanf(theMessage, "Twin Result: %s %s %s", twinProgram, twinResidue, twinProver);
      else if (!memcmp(theMessage, "Proven By: ", 11))
         strcpy(prover, theMessage+11);
      else if (!memcmp(theMessage, "GFN: ", 5))
         AddGFNToList(theMessage + 5);
  
      delete [] theMessage;

      theMessage = theSocket->Receive();
   }

   if (theMessage)
      delete [] theMessage;

   if (*residue == 0 && !abandoned)
   {
      theSocket->Send("ERROR:  Test for %s rejected.  No residue reported", s_Name);
      m_Log->LogMessage("%s (%s) at %s: Rejected test on %s due to no residue",
                        fromEmailID, fromClientID, inetAddress, s_Name);
      return RC_FAILURE;
   }

   // This tells the client that the server has acknowledged the test.  This will
   // happen even if the server has no record of the test.  The server does not
   // provide this to the client if a residue was not found.
   if (isBuffered)
      theSocket->Send("Acknowledge: %s", s_Name);

   for (i=0; i<strlen(residue); i++)
      residue[i] = toupper(residue[i]);

   if (!tPtr)
   {
      theSocket->Send("INFO: Server has no record of this test.  The test result for %s has been logged", s_Name);
      m_Log->LogMessage("%s (%s) at %s: Rejected test on %s due to no matching test.  Residue was %s.",
                        fromEmailID, fromClientID, inetAddress, s_Name, residue);
 
      return RC_FAILURE;
   }

   strcpy(tPtr->s_UserID,        fromUserID);
   strcpy(tPtr->s_Program,       program);
   strcpy(tPtr->s_Prover,       (strlen(prover)      ? prover      : "na"));
   strcpy(tPtr->s_Residue,      (abandoned           ? "ABANDONED" : residue));
   strcpy(tPtr->s_TwinProgram,  (strlen(twinProgram) ? twinProgram : "na"));
   strcpy(tPtr->s_TwinProver,   (strlen(twinProver)  ? twinProver  : "na"));
   strcpy(tPtr->s_TwinResidue,  (strlen(twinResidue) ? twinResidue : "na"));
   strcpy(tPtr->s_GeneferROE,    standardizedNames);

   tPtr->i_SearchedForGFNDivisors = searchedForGFNDivisors;

   // Don't make any more changes if the test was abandoned
   if (abandoned)
   {
      i_IssuedTests--;
      return RC_ABANDONED;
   }

   if (!b_IsActive)
      theSocket->Send("INFO: The candidate %s is 'inactive'.  The test result has been accepted.", s_Name);
   else
      theSocket->Send("INFO: Test for candidate %s accepted", s_Name);

   LogCompletedTest(tPtr, inetAddress);

   l_LastUpdateTime = time(NULL);

   // genefer might have had a round off error.  If so, don't mark this test as completed.
   if (strcmp(residue, "roundofferr"))
      i_CompletedTests++;

   // Write GFNs to a log
   gfnLog = new Log(0, "GFN.log", 0, true);
   gfnPtr = m_FirstGFN;
   while (gfnPtr)
   {
      gfnLog->LogMessage("%s: %s (found by %s)", s_Name, gfnPtr->s_Divisor, fromEmailID);
      gfnPtr = (gfn_t *) gfnPtr->m_NextGFN;
   }
   delete gfnLog;

   return RC_SUCCESS;
}

void  Candidate::SendPRPFoundEmail(Mail *theMail, char *toEmailID, uint32_t duplicate)
{
   test_t  *tPtr;
   int32_t  rc, isPrime;
   gfn_t   *gfnPtr;

   tPtr = m_Test;
   while (strcmp(tPtr->s_Residue, "PRP") && strcmp(tPtr->s_Residue, "PRIME"))
   {
      tPtr = (test_t *) tPtr->m_Next;
   }

   isPrime = !strcmp(tPtr->s_Residue, "PRIME");

   rc = theMail->NewMessage(toEmailID, "Candidate %s was found to be %s by PRPNet!", s_Name, (isPrime ? "Prime" : "PRP"));
   if (rc != RC_SUCCESS)
      return;

   if (duplicate)
   {
      theMail->AppendLine("The person listed below has already reported this %s, so you will not get credit for it.", (isPrime ? "Prime" : "PRP"));
   }
   else
   {
      theMail->AppendLine("This number is approximately %d digits long.", i_DecimalLength);
      theMail->AppendLine("It was found on machine %s using the program %s.", tPtr->s_ClientID, tPtr->s_Program);
      if (!isPrime)
      {
         theMail->AppendLine("Since the test performed as only a PRP test, another program, such as PFGW must be used to prove primality.");
         theMail->AppendLine("If the number is prime and is large enough for the Prime Pages, please include the program %s in the credits.", 
                             tPtr->s_Program);
      }
      else
      {
         theMail->AppendLine("It was proven prime with the program %s", tPtr->s_Prover);
         if (strcmp(tPtr->s_Program, tPtr->s_Prover))
            theMail->AppendLine("If the number is large enough for the Prime Pages, please include the programs %s and %s in the credits.",
                                tPtr->s_Program, tPtr->s_Prover);
         else
            theMail->AppendLine("If the number is large enough for the Prime Pages, please include the program %s in the credits.", 
                                 tPtr->s_Program);
         if (!USE_PFGW(i_ServerType) && i_b == 2 && i_c == 1)
            if (m_Test->i_SearchedForGFNDivisors)
            {
               if (!m_FirstGFN) 
                  theMail->AppendLine("The client searched for GFN divisors, but did not find any.");
               else
               {
                  gfnPtr = m_FirstGFN;
                  theMail->AppendLine("The client searched and found the following GFN divisors:");
                  while (gfnPtr)
                  {
                     theMail->AppendLine("   %s", gfnPtr->s_Divisor);
                     gfnPtr = (gfn_t *) gfnPtr->m_NextGFN;
                  }
               }
            }
            else
               theMail->AppendLine("The client did not run a test to search for GFN divisors.");
      }

      theMail->AppendLine("If there is a project on the Prime Pages for this prime, please include that as well.");
   }

   theMail->SendMessage();
}

void    Candidate::Save(FILE *saveFilePtr)
{
   test_t   *tPtr;

   fprintf(saveFilePtr, "%s N %llu %s\n", s_Name, l_LastUpdateTime, 
                         (b_IsActive ? "active" : "inactive"));

   tPtr = m_Test;
   while (tPtr)
   {
      if (!strcmp(tPtr->s_Residue, "EXPIRED"))
         i_IssuedTests--;

      // Don't write abandoned or expired tests to the save file
      if (strcmp(tPtr->s_Residue, "ABANDONED") && strcmp(tPtr->s_Residue, "EXPIRED"))
         fprintf(saveFilePtr, "%s T %llu %s %s %s %s %s %s\n", 
                 s_Name, 
                 tPtr->l_TestID,
                 tPtr->s_EmailID,
                 tPtr->s_UserID,
                 tPtr->s_ClientID,
                 tPtr->s_Program,
                 tPtr->s_Residue,
                 tPtr->s_Prover);

      if (strlen(tPtr->s_GeneferROE) > 0)
         fprintf(saveFilePtr, "%s R %llu %s\n", s_Name, tPtr->l_TestID, tPtr->s_GeneferROE);

      tPtr = (test_t *) tPtr->m_Next;
   }
}

// Add the GFN to the linked list of the given work unit
void  Candidate::AddGFNToList(char *divisor)
{
   gfn_t   *gfnPtr, *listPtr;

   gfnPtr = (gfn_t *) malloc(sizeof(gfn_t));
   gfnPtr->m_NextGFN = 0;
   strcpy(gfnPtr->s_Divisor, divisor);
   i_GFNDivisors++;

   if (!m_FirstGFN)
   {
      m_FirstGFN = gfnPtr;
      return;
   }

   listPtr = m_FirstGFN;

   while (listPtr->m_NextGFN)
      listPtr = (gfn_t *) listPtr->m_NextGFN;

   listPtr->m_NextGFN = gfnPtr;
}

uint32_t  Candidate::HasPendingTest(void)
{
   test_t  *tPtr;

   tPtr = m_Test;
   while (tPtr)
   {
      if (!strcmp(tPtr->s_Residue, "inprogress"))
         return true;

      tPtr = (test_t *) tPtr->m_Next;
   }

   return false;
}

void  Candidate::ExpireOldTests(uint64_t cutoffTime)
{
   test_t  *tPtr;

   tPtr = m_Test;
   while (tPtr)
   {
     if (!strcmp(tPtr->s_Residue, "inprogress"))
        if (tPtr->l_TestID < cutoffTime)
        {
           strcpy(tPtr->s_Residue, "EXPIRED");
           m_Log->LogMessage("%s: Test for user %s has expired", s_Name, tPtr->s_EmailID);
           m_Log->Debug(DEBUG_ALL, "%s: Test %llu < %llu (time %u)", s_Name, tPtr->l_TestID, cutoffTime, time(NULL));
        }

      tPtr = (test_t *) tPtr->m_Next;
   }
}

uint32_t  Candidate::HadRoundoffError(char *geneferName)
{
   test_t  *tPtr;
   char     nameToFind[20], geneferROE[100];

   sprintf(nameToFind, "%s ", geneferName);

   tPtr = m_Test;
   while (tPtr)
   {
      sprintf(geneferROE, "%s ", tPtr->s_GeneferROE);

      if (strstr(geneferROE, nameToFind) > 0)
         return true;

      tPtr = (test_t *) tPtr->m_Next;
   }

   return false;
}

void     Candidate::SendPendingTestHTML(Socket *theSocket)
{
   test_t  *tPtr;
   char     dtString[80];
   long     seconds, hours, minutes;
   time_t   theTime;
   struct tm *theTM;

   tPtr = m_Test;
   while (tPtr)
   {
      if (!strcmp(tPtr->s_Residue, "inprogress"))
      {
         theTime = (time_t) tPtr->l_TestID;
         theTM = gmtime(&theTime);
         strftime(dtString, sizeof(dtString), "%c", theTM);
         seconds = (long) (time(NULL) - theTime);
         hours = seconds / 3600;
         minutes = (seconds - hours * 3600) / 60;
         theSocket->Send("<tr><td>%s<td>%s<td>%s<td>%s<td>%d:%02d</tr>",
                         s_Name, tPtr->s_UserID, tPtr->s_ClientID, dtString, hours, minutes);
      }

      tPtr = (test_t *) tPtr->m_Next;
   }
}

int32_t   Candidate::WriteToDB(DBInterface *dbInterface)
{
   SQLStatement *sqlStatement;
   test_t  *t1, *t2;
   char     theK[20], testID[20], lastUpdateTime[20];
   gfn_t   *gfn;
   int32_t  doubleChecked = false, searchedGFN = false;;
   int32_t  gen64 = false, gen80 = false;

   t1 = m_Test;
   while (t1)
   {
      if (!strcmp(t1->s_Residue, "inprogress"))
      {
         t2 = (test_t *) t1->m_Next;
         while (t2)
         {
            if (!strcmp(t1->s_Residue, t1->s_Residue))
               doubleChecked = true;
         }
      }

      if (strstr(t1->s_GeneferROE, "genefer80"))
         gen80 = true;
      if (strstr(t1->s_GeneferROE, "genefx64"))
         gen64 = true;

      if (t1->i_SearchedForGFNDivisors)
         searchedGFN = true;

      t1 = (test_t *) t1->m_Next;
   }

   sqlStatement = new SQLStatement(m_Log, dbInterface);

   sqlStatement->PrepareInsert("Candidate");

   sprintf(theK, "%lld", l_k);
   sprintf(lastUpdateTime, "%lld", l_LastUpdateTime);
   sqlStatement->InsertNameValuePair("CandidateName", s_Name);
   sqlStatement->InsertNameValuePair("DecimalLength", (int32_t) 0);
   sqlStatement->InsertNameValuePair("CompletedTests", (int32_t) i_CompletedTests);
   sqlStatement->InsertNameValuePair("HasPendingTest", (int32_t) HasPendingTest());
   sqlStatement->InsertNameValuePair("DoubleChecked", doubleChecked);
   if (i_ServerType != ST_PRIMORIAL && i_ServerType != ST_FACTORIAL && i_ServerType != ST_GFN)
      sqlStatement->InsertNameValuePair("k", theK);
   if (i_ServerType != ST_PRIMORIAL && i_ServerType != ST_FACTORIAL)
      sqlStatement->InsertNameValuePair("b", (int32_t) i_b);
   sqlStatement->InsertNameValuePair("n", (int32_t) i_n);
   if (i_ServerType != ST_GFN)
      sqlStatement->InsertNameValuePair("c", (int32_t) i_c);
   sqlStatement->InsertNameValuePair("IsPRP", (int32_t) b_IsPRP);
   sqlStatement->InsertNameValuePair("IsPrime", (int32_t) b_IsPrime);
   sqlStatement->InsertNameValuePair("IsTwinPRP", (int32_t) b_IsTwinPRP);
   sqlStatement->InsertNameValuePair("IsTwinPrime", (int32_t) b_IsTwinPrime);
   sqlStatement->InsertNameValuePair("SearchedForGFNDivisors", (int32_t) searchedGFN);
   sqlStatement->InsertNameValuePair("LastUpdateTime", lastUpdateTime);

   if (!sqlStatement->ExecuteInsert())
   {
      delete sqlStatement;
      return false;
   }

   gfn = m_FirstGFN;
   while (gfn)
   {
      sqlStatement->PrepareInsert("CandidateGFNDivisor");

      sqlStatement->InsertNameValuePair("CandidateName", s_Name);
      sqlStatement->InsertNameValuePair("GFN", gfn->s_Divisor);
 
      if (!sqlStatement->ExecuteInsert())
      {
         delete sqlStatement;
         return false;
      }

      gfn = (gfn_t *) gfn->m_NextGFN;
   }

   if (gen64)
   {
      sqlStatement->PrepareInsert("GeneferROE");

      sqlStatement->InsertNameValuePair("CandidateName", s_Name);
      sqlStatement->InsertNameValuePair("GeneferVersion", "genefx64");
 
      if (!sqlStatement->ExecuteInsert())
      {
         delete sqlStatement;
         return false;
      }
   }

   if (gen80)
   {
      sqlStatement->PrepareInsert("GeneferROE");

      sqlStatement->InsertNameValuePair("CandidateName", s_Name);
      sqlStatement->InsertNameValuePair("GeneferVersion", "genefer80");
 
      if (!sqlStatement->ExecuteInsert())
      {
         delete sqlStatement;
         return false;
      }
   }

   t1 = m_Test;
   while (t1)
   {
      sqlStatement->PrepareInsert("CandidateTest");

      sprintf(testID, "%lld", t1->l_TestID);
      sqlStatement->InsertNameValuePair("CandidateName", s_Name);
      sqlStatement->InsertNameValuePair("TestID", testID);
      if (strcmp(t1->s_Program, "na"))
         sqlStatement->InsertNameValuePair("PRPingProgram", t1->s_Program);
      if (strcmp(t1->s_Prover, "na"))
         sqlStatement->InsertNameValuePair("ProvingProgram", t1->s_Prover);
      if (strcmp(t1->s_Residue, "inprogress"))
         sqlStatement->InsertNameValuePair("Residue", t1->s_Residue);

      if (i_ServerType == ST_TWIN)
      {
         if (strcmp(t1->s_TwinProgram, "na"))
            sqlStatement->InsertNameValuePair("TwinPRPingProgram", t1->s_TwinProgram);
         if (strcmp(t1->s_TwinProver, "na"))
            sqlStatement->InsertNameValuePair("TwinProvingProgram", t1->s_TwinProver);
         if (strcmp(t1->s_TwinResidue, "inprogress"))
            sqlStatement->InsertNameValuePair("TwinResidue", t1->s_TwinResidue);
      }
      sqlStatement->InsertNameValuePair("EmailID", t1->s_EmailID);
      sqlStatement->InsertNameValuePair("UserID", t1->s_UserID);
      sqlStatement->InsertNameValuePair("ClientID", t1->s_ClientID);
      if (b_IsPRP || b_IsPrime)
         sqlStatement->InsertNameValuePair("EmailSent", 1);
      else
         sqlStatement->InsertNameValuePair("EmailSent", 0);

      if (!sqlStatement->ExecuteInsert())
      {
         delete sqlStatement;
         return false;
      }

      t1 = (test_t *) t1->m_Next;
   }

   delete sqlStatement;
   return true;
}

