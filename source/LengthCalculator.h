#ifndef _LengthCalculator_

#define _LengthCalculator_

#include "DBInterface.h"
#include "Log.h"
#include "Socket.h"

class LengthCalculator
{
public:
   LengthCalculator(int32_t serverType, DBInterface *dbInterface, Log *theLog);

   ~LengthCalculator();

   double     CalculateDecimalLength(int64_t intK, int32_t intB, int32_t intN);
   void       CalculateDecimalLengths(Socket *theSocket);

private:
   int32_t    ii_ServerType;
   DBInterface *ip_DBInterface;
   Log         *ip_Log;

   int32_t   *ip_PrimeTable;
   int32_t    ii_PrimesInPrimeTable;

   void       InitializePrimeSieve(void);
};

#endif // #ifndef _LengthCalculator_

