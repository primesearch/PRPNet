// LengthCalculator.cpp
//
// Mark Rodenkirch (rogue@wi.rr.com)

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include "defs.h"

#include "LengthCalculator.h"
#include "SQLStatement.h"

LengthCalculator::LengthCalculator(int32_t serverType, DBInterface *dbInterface, Log *theLog)
{
   ip_PrimeTable = 0;
   ii_ServerType = serverType;
   ip_DBInterface = dbInterface;
   ip_Log = theLog;
}

LengthCalculator::~LengthCalculator()
{
   if (ip_PrimeTable)
      delete [] ip_PrimeTable;
}

void    LengthCalculator::InitializePrimeSieve(void)
{
   int32_t   i, p, minp, *lowPrimes, lowPrimeCount, composite;
   char     *sieve, *sievePtr;

   // Make sure this is large enough to hold all of the primes we need.
   lowPrimes = (int32_t *) malloc(5000 * sizeof(int32_t));
   lowPrimes[0] = 3;
   lowPrimeCount = 1;

   // Find all primes below 1e4.  This will allow us to sieve
   // find all primes up to 1e8 and store them in the array
   for (p=5; p<10000; p+=2)
   {
      for (minp=0; minp<=lowPrimeCount; minp++)
      {
         if (lowPrimes[minp] * lowPrimes[minp] > p)
         {
            lowPrimes[lowPrimeCount] = p;
            lowPrimeCount++;
            break;
         }
         if (p % lowPrimes[minp] == 0)
             break;
      }
   }

   // We're really not concerned with memory usage as this is temporary
   sieve = (char *) malloc(50000000 * sizeof(char));
   ip_PrimeTable = (int32_t *) malloc(10000000 * sizeof(int32_t));
   memset(sieve, 1, 50000000);

   for (i=0; i<lowPrimeCount; i++)
   {
      // Get the current low prime.  Start sieving at 3x that prime
      // since 1x is prime and 2x is divisible by 2.
      // sieve[1] = 3, sieve[2] = 5, etc.
      composite = lowPrimes[i] * 3;
      sievePtr = &sieve[(composite - 1) >> 1];

      while (composite < 100000000)
      {
         // composite will always be odd, so add 2*lowPrimes[i]
         *sievePtr = 0;
         sievePtr += lowPrimes[i];
         composite += (lowPrimes[i] << 1);
      }
   }

   ii_PrimesInPrimeTable = 0;
   for (i=1; i<50000000; i++)
   {
      if (sieve[i])
      {
         // Convert the value back to an actual prime
         ip_PrimeTable[ii_PrimesInPrimeTable] = (i << 1) + 1;
         ii_PrimesInPrimeTable++;
      }
   }

   free(sieve);
   free(lowPrimes);
}

double   LengthCalculator::CalculateDecimalLength(int64_t intK, int32_t intB, int32_t intN)
{
   double    doubleK, doubleB, doubleN;

   switch (ii_ServerType)
   {
      case ST_CULLENWOODALL:
         doubleB = (double) intB;
         doubleN = (double) intN;
         return floor(log10(doubleB) * doubleN + log10(doubleN) + 1.0);

      case ST_GFN:
         if (intB < intN)
         {
            doubleB = (double) intB;
            doubleN = (double) intN;
         }
         else
         {
            doubleN = (double) intB;
            doubleB = (double) intN;
         }
         return floor(log10(doubleB) * doubleN + 1.0);
 
      case ST_XYYX:
         doubleB = (double) intB;
         doubleN = (double) intN;
         return floor(log10(doubleB) * doubleN + 1.0);

      case ST_CYCLOTOMIC:
         doubleB = (double) abs(intB);
         doubleN = (double) intN;
         return floor(log10(doubleB) * doubleN + 1.0);
         
      case ST_CAROLKYNEA:
         doubleB = (double) (intB);
         doubleN = (double) (intN * 2);
         return floor((log10(doubleB) * doubleN) / 3.0 + 1.0);
         
      case ST_WAGSTAFF:
         doubleB = 2.0;
         doubleN = (double) intN;
         return floor(log10(doubleB) * doubleN + 1.0);

      case ST_SIERPINSKIRIESEL:
      case ST_FIXEDBKC:
      case ST_FIXEDBNC:
      case ST_TWIN:
      case ST_SOPHIEGERMAIN:
         doubleK = (double) intK;
         doubleB = (double) intB;
         doubleN = (double) intN;
         return floor(log10(doubleB) * doubleN + log10(doubleK) + 1.0);
   }

   return 0.0;
}

void     LengthCalculator::CalculateDecimalLengths(Socket *theSocket)
{
   SQLStatement *selectStatement;
   SQLStatement *updateStatement;
   double    decimalLength = 0.0, previousDecimalLength;
   int32_t   previousN, primeIndex;
   int32_t   intB, intN, count;
   int64_t   theK;
   char      candidateName[NAME_LENGTH+1];
   double    previousValue, tenth = 0.1;
   const char *selectSQL = "select CandidateName, k, b, n " \
                           "  from Candidate " \
                           " where DecimalLength = 0 " \
                           "order by n " \
                           " limit 12345 ";
   const char *updateSQL = "update Candidate " \
                           "   set DecimalLength = ?" \
                           " where CandidateName = ?";
   
   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   selectStatement->BindSelectedColumn(&theK);
   selectStatement->BindSelectedColumn(&intB);
   selectStatement->BindSelectedColumn(&intN);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(decimalLength);
   updateStatement->BindInputParameter(candidateName, NAME_LENGTH, false);

   do
   {
      if (ii_ServerType == ST_FACTORIAL)
      {
         previousN = 0;
         previousDecimalLength = 0;
         previousValue = 1.0;
      }
      
      if (ii_ServerType == ST_PRIMORIAL)
      {
         InitializePrimeSieve();
         previousN = 0;
         previousDecimalLength = 0;
         previousValue = 6.0;
         primeIndex = 0;
      }

      // We don't want to retrieve all candidates without a length in one cursor it
      // it will take time to allocate the memory and the connecting client might not
      // wait long enough.  By breaking it apart into multiple selects, the server
      // can respond to the client much faster, hopefully avoiding a client timeout.
      count = 0;
      while (selectStatement->FetchRow(false))
      {
         switch (ii_ServerType)
         {
            case ST_GENERIC:
               // Default the length.  The client will compute a more accurate length.
               decimalLength = (double) strlen(candidateName);
               break;

            case ST_FACTORIAL:
               if (previousN != intN)
               {
                  while (previousN < intN)
                  {
                     previousN++;
                     previousValue *= (double) previousN;
                     while (previousValue > 10.0)
                     {
                        previousValue *= tenth;
                        previousDecimalLength++;
                     }
                  }
               }

               decimalLength = previousDecimalLength;
               break;

            case ST_MULTIFACTORIAL:
               decimalLength = 0;
               previousValue = 1.0;
               while (intN > 0)
               {
                  previousValue *= (double) intN;
                  while (previousValue > 10.0)
                  {
                     previousValue *= tenth;
                     decimalLength++;
                  }

                  intN -= intB;
               }
               break;

            case ST_PRIMORIAL:
               if (previousN != intN)
               {
                  while (ip_PrimeTable[primeIndex] < intN)
                  {
                     primeIndex++;
                     previousValue *= (double) ip_PrimeTable[primeIndex];
                     while (previousValue > 10.0)
                     {
                        previousValue *= tenth;
                        previousDecimalLength++;
                     }
                  }
               }

               decimalLength = previousDecimalLength;
               break;
         }

         count++;

         updateStatement->SetInputParameterValue(floor(decimalLength) + 1, true);
         updateStatement->SetInputParameterValue(candidateName);

         if (updateStatement->Execute())
            ip_DBInterface->Commit();
         else
            ip_DBInterface->Rollback();

         if (theSocket && (count % 1000 == 0))
            theSocket->Send("keepalive");
      }
   } while (count > 0);

   delete selectStatement;
   delete updateStatement;

   if (ii_ServerType == ST_PRIMORIAL)
   {
      delete [] ip_PrimeTable;
      ip_PrimeTable = 0;
   }
}

