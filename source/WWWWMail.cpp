#include "WWWWMail.h"
#include "SQLStatement.h"

WWWWMail::WWWWMail(globals_t *globals, string serverName, uint32_t portID)
                   : Mail(globals, serverName, portID)
{
}

void  WWWWMail::MailSpecialResults(void)
{
   SQLStatement *selectStatement;
   SQLStatement *updateStatement;
   int64_t  lowerLimit = 0, upperLimit = 0, testID = 0, prime = 0;
   int32_t  remainder = 0, quotient = 0, duplicate = 0;
   char     emailID[ID_LENGTH+1], machineID[ID_LENGTH+1], instanceID[ID_LENGTH+1];
   char     searchingProgram[ID_LENGTH+1];
   const char *selectSQL = "select WWWWRangeTest.LowerLimit, WWWWRangeTest.UpperLimit, " \
                           "       WWWWRangeTest.TestID, WWWWRangeTest.EmailID, " \
                           "       WWWWRangeTest.MachineID, WWWWRangeTest.InstanceID, " \
                           "       WWWWRangeTest.SearchingProgram, WWWWRangeTestResult.Duplicate, " \
                           "       WWWWRangeTestResult.Prime, WWWWRangeTestResult.Remainder, " \
                           "       WWWWRangeTestResult.Quotient " \
                           "  from WWWWRangeTest, WWWWRangeTestResult "
                           " where WWWWRangeTest.LowerLimit = WWWWRangeTestResult.LowerLimit " \
                           "   and WWWWRangeTest.UpperLimit = WWWWRangeTestResult.UpperLimit " \
                           "   and WWWWRangeTest.TestID = WWWWRangeTestResult.TestID " \
                           "   and WWWWRangeTestResult.EmailSent = 0 " \
                           "order by WWWWRangeTest.TestID";
   const char *updateSQL = "update WWWWRangeTestResult " \
                           "   set EmailSent = 1 " \
                           " where LowerLimit = ? " \
                           "   and UpperLimit = ? " \
                           "   and TestId = ? " \
                           "   and Prime = ? ";

   selectStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   selectStatement->BindSelectedColumn(&lowerLimit);
   selectStatement->BindSelectedColumn(&upperLimit);
   selectStatement->BindSelectedColumn(&testID);
   selectStatement->BindSelectedColumn(emailID, ID_LENGTH);
   selectStatement->BindSelectedColumn(machineID, ID_LENGTH);
   selectStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   selectStatement->BindSelectedColumn(searchingProgram, ID_LENGTH);
   selectStatement->BindSelectedColumn(&duplicate);
   selectStatement->BindSelectedColumn(&prime);
   selectStatement->BindSelectedColumn(&remainder);
   selectStatement->BindSelectedColumn(&quotient);

   updateStatement = new SQLStatement(ip_Log, ip_DBInterface, updateSQL);
   updateStatement->BindInputParameter(lowerLimit);
   updateStatement->BindInputParameter(upperLimit);
   updateStatement->BindInputParameter(testID);
   updateStatement->BindInputParameter(prime);

   while (selectStatement->FetchRow(false))
   {
      if (!NotifyUser(emailID, prime, remainder, quotient, duplicate, machineID, instanceID, searchingProgram)) continue;

      updateStatement->SetInputParameterValue(lowerLimit, true);
      updateStatement->SetInputParameterValue(upperLimit);
      updateStatement->SetInputParameterValue(testID);
      updateStatement->SetInputParameterValue(prime);

      if (updateStatement->Execute())
         ip_DBInterface->Commit();
      else
         ip_DBInterface->Rollback();

      ip_Log->LogMessage("Sent e-mail for %" PRId64":(%+d %+d p)", prime, remainder, quotient);
   }

   delete updateStatement;
   delete selectStatement;
}

void  WWWWMail::MailLowWorkNotification(int32_t daysLeft)
{
   bool  success;

   success = NewMessage(is_FromEmailID.c_str(), "PRPNet Running Dry");
   if (success)
   {
      ip_Socket->StartBuffering();
      ip_Socket->SetAutoNewLine(false);

      AppendLine(0, "There PRPNet server for project %s has an estimated %d days left of work.", is_ProjectName.c_str(), daysLeft);
      
      ip_Socket->SetAutoNewLine(true);
      SendMessage();
   }
}

bool  WWWWMail::NotifyUser(string toEmailID, int64_t prime, int32_t remainder, int32_t quotient,
                           int32_t duplicate, string machineID, string instanceID, string searchingProgram)
{
   const char  *searchType;
   bool   success;

   if (ii_ServerType == ST_WIEFERICH)    searchType = "Wieferich";
   if (ii_ServerType == ST_WILSON)       searchType = "Wilson";
   if (ii_ServerType == ST_WALLSUNSUN)   searchType = "Wall-Sun-Sun";
   if (ii_ServerType == ST_WOLSTENHOLME) searchType = "Wolstenholme";

   if (!remainder && !quotient)
      success = NewMessage(toEmailID, "%s prime %" PRId64" found by PRPNet!", searchType, prime);
   else
   {
      if (ii_ServerType == ST_WALLSUNSUN)
         success = NewMessage(toEmailID, "%s special instance %" PRId64" (0 %+d) p was found by PRPNet!",
                              searchType, prime, quotient);
      else
         success = NewMessage(toEmailID, "%s special instance %" PRId64" (%+d %+d) p was found by PRPNet!",
                              searchType, prime, remainder, quotient);
   }

   if (!success)
      return false;

   ip_Socket->StartBuffering();
   ip_Socket->SetAutoNewLine(false);

   if (!remainder && !quotient)
      AppendLine(0, "This a new %s Prime.", searchType);
   else
   {
      AppendLine(0, "This a near %s Prime.  Although it is not a %s Prime, it will be", searchType, searchType);
      AppendLine(0, "recorded as a special instance.  It also helps us to verify that the search");
      AppendLine(0, "algorithm is corect and increases the hope that");
      AppendLine(0, "that a new %s Prime will be found someday.", searchType);
   }

   AppendLine(0, "It was found on machine %s, instance %s using the program %s.", machineID.c_str(), instanceID.c_str(), searchingProgram.c_str());

   if (duplicate)
      AppendLine(2, "Unfortunately your client found this during a double-check so you will not get credit.");

   ip_Socket->SetAutoNewLine(true);

   SendMessage();

   return true;
}
