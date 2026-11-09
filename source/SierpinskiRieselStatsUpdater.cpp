#include "SierpinskiRieselStatsUpdater.h"
#include "SQLStatement.h"

string  SierpinskiRieselStatsUpdater::GenerateCandidateGroupId(int64_t theK, int32_t theB, int32_t theN, int64_t theC, int32_t theD)
{
   char temp[ID_LENGTH + 1];

   if (theD == 1)
      sprintf(temp, "%" PRId64"*%u^n%+" PRId64"", theK, theB, theC);
   else
      sprintf(temp, "(%" PRId64"*%u^n%+" PRId64")/%d", theK, theB, theC, theD);

   return temp;
}

bool  SierpinskiRieselStatsUpdater::SetSierspinkiRieselPrimeN(char *candidateGroupId, int32_t theN)
{
   SQLStatement  *sqlStatement;
   int32_t        sierpinskiRieselPrimeN;
   bool           success;
   const char    *selectSQL = "select sierpinskiRieselPrimeN from CandidateGroupStats " \
                              " where candidateGroupId = ? ";
   const char    *updateSQL  = "update CandidateGroupStats set sierpinskiRieselPrimeN = %d " \
                               " where candidateGroupId = ? ";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&sierpinskiRieselPrimeN);

   success = sqlStatement->FetchRow(true);
   delete sqlStatement;

   if (!success) 
      return false;

   if (theN > sierpinskiRieselPrimeN && sierpinskiRieselPrimeN > 0)
      return success;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL, theN);
   sqlStatement->BindInputParameter(candidateGroupId, ID_LENGTH);

   success = sqlStatement->Execute();
   delete sqlStatement;

   return success;
}
