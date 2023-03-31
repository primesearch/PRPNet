#include "PrimeHTMLGenerator.h"
#include "PrimeServerHelper.h"
#include "ServerHelper.h"
#include "SQLStatement.h"

bool     PrimeHTMLGenerator::SendSpecificPage(string thePage)
{
   if (thePage == "user_primes.html")
   {
      HeaderPlusLinks("Primes by User");
      PrimesByUser();
   }
   else if (thePage == "user_gfns.html" && CanCheckForGFNs())
   {
      HeaderPlusLinks("GFN Divisors by User");
      GFNDivisorsByUser();
   }
   else if (thePage == "team_primes.html" && HasTeams())
   {
      HeaderPlusLinks("Primes by Team");
      PrimesByTeam();
   }
   else if (thePage == "pending_tests.html")
   {
      HeaderPlusLinks("Pending Tests");
      PendingTests();
   }
   else if (thePage == "all.html")
   {
      HeaderPlusLinks("Status");

      PendingTests();
      ServerStats();
      UserStats();
      if (HasTeams())
      {
         TeamStats();
         PrimesByUser();
         PrimesByTeam();
         TeamUserStats();
      }
      if (CanCheckForGFNs())
      {
         GFNDivisorsByUser();
      }
   }
   else
      return false;

   return true;
}

void     PrimeHTMLGenerator::PendingTests(void)
{
   SQLStatement *sqlStatement;
   int64_t     testID, expireSeconds;
   char        candidateName[NAME_LENGTH+1], userID[ID_LENGTH+1];
   char        machineID[ID_LENGTH+1], instanceID[ID_LENGTH+1],teamID[ID_LENGTH+1];
   int32_t     ii, seconds, hours, minutes;
   int32_t     expireHours, expireMinutes;
   double      decimalLength;
   time_t      currentTime;

   const char *theSelect = "select Candidate.CandidateName, Candidate.DecimalLength, " \
                           "       ct.TestID, ct.UserID, ct.MachineID, ct.InstanceID, $null_func$(ct.TeamID, ' ') " \
                           "  from Candidate, CandidateTest ct" \
                           " where Candidate.CandidateName = ct.CandidateName " \
                           "   and ct.IsCompleted = 0 " \
                           "order by ct.TestID, ct.CandidateName";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&decimalLength);
   sqlStatement->BindSelectedColumn(&testID);
   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(teamID, ID_LENGTH);

   currentTime = time(NULL);

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No Pending WU\'s"))
      return;

   ip_Socket->Send("<table id=\"pending-tests-table\" class=\"sortable\"><thead><tr>");
   TH_CLMN_HDR("Candidate");
   TH_CLMN_HDR("User");
   TH_CLMN_HDR("Machine");
   TH_CLMN_HDR("Instance");
   if (HasTeams()) TH_CLMN_HDR("Team");
   TH_CLMN_HDR("Date Assigned");
   TH_CLMN_HDR("Age (hh:mm)");
   TH_CLMN_HDR("Expires (hh:mm)");
   ip_Socket->Send("</tr></thead><tbody>");

   do
   {
      expireSeconds = 0;
      for (ii=0; ii<ii_DelayCount; ii++)
         if (decimalLength < ip_Delay[ii].maxLength)
         {
            expireSeconds = testID + ip_Delay[ii].expireDelay - (int64_t) currentTime;
            break;
         }

      if (expireSeconds < 0) expireSeconds = 0;
      expireHours = (int32_t) (expireSeconds / 3600);
      expireMinutes = (int32_t) (expireSeconds - expireHours * 3600) / 60;

      seconds = (int32_t) (currentTime - testID);
      hours = seconds / 3600;
      minutes = (seconds - hours * 3600) / 60;

      ip_Socket->Send("<tr>");

      TD_CHAR(candidateName);
      TD_CHAR(userID);
      TD_CHAR(machineID);
      TD_CHAR(instanceID);
      if (HasTeams()) TD_CHAR(teamID);
      TD_CHAR(TimeToString(testID).c_str());
      TD_TIME(hours, minutes);
      TD_TIME(expireHours, expireMinutes);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");

   delete sqlStatement;
}

void     PrimeHTMLGenerator::PrimesByUser(void)
{
   SQLStatement *sqlStatement;
   int64_t    dateReported;
   int32_t    primeCount, prpCount, showOnWebPage, hiddenCount;
   int32_t    testResult;
   char       userID[ID_LENGTH+1], prevUserID[ID_LENGTH+1];
   char       candidateName[NAME_LENGTH+1], machineID[ID_LENGTH+1];
   char       instanceID[ID_LENGTH+1], teamID[ID_LENGTH+1];
   double     decimalLength;

   const char *theSelect = "select UserID, $null_func$(TeamID, '&nbsp;'), MachineID, InstanceId, TestedNumber, TestResult, " \
                           "       DateReported, DecimalLength, ShowOnWebPage " \
                           "  from UserPrimes " \
                           "order by UserID, CandidateName, DecimalLength";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&testResult);
   sqlStatement->BindSelectedColumn(&dateReported);
   sqlStatement->BindSelectedColumn(&decimalLength);
   sqlStatement->BindSelectedColumn(&showOnWebPage);

   ip_Socket->StartBuffering();

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No primes found"))
      return;

   *prevUserID = 0;
   hiddenCount = primeCount = prpCount = 0;
   do
   {
      if (strcmp(prevUserID, userID))
      {
         if (*prevUserID)
         {
            if (hiddenCount)
               ip_Socket->Send("<tfoot><tr><td colspan=\"7\">User %s has found %d prime%s and %d PRP%s, %d %s hidden</td></tr></tfoot>",
                              prevUserID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount),
                              hiddenCount, PLURAL_COPULA(hiddenCount));
            else
               ip_Socket->Send("<tfoot><tr><td colspan=\"7\">User %s has found %d prime%s and %d PRP%s</td></tr></tfoot>",
                              prevUserID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount));
            ip_Socket->Send("</table>");
         }

         ip_Socket->Send("<h3>Primes and PRPs for User %s</h3>", userID);

         ip_Socket->Send("<table class=\"primes-by-user-table\"><thead><tr>");
         TH_CLMN_HDR("Candidate");
         TH_CLMN_HDR("Prime/PRP");
         TH_CLMN_HDR("Machine");
         TH_CLMN_HDR("Instance");
         TH_CLMN_HDR("Team");
         TH_CLMN_HDR("Date Reported");
         TH_CLMN_HDR("Decimal Length");
         ip_Socket->Send("</tr></thead>");

         strcpy(prevUserID, userID);
         hiddenCount = primeCount = prpCount = 0;
      }

      if (testResult == R_PRIME)
         primeCount++;
      else
         prpCount++;

      if (showOnWebPage)
      {
         ip_Socket->Send("<tbody><tr>");

         TD_CHAR(candidateName);
         TD_CHAR(((testResult == R_PRIME) ? "Prime" : "PRP"));
         TD_CHAR(machineID);
         TD_CHAR(instanceID);
         TD_CHAR(teamID);
         TD_CHAR(TimeToString(dateReported).c_str());
         TD_32BIT((uint32_t) decimalLength);

         ip_Socket->Send("</tr></tbody>");
      }
      else
         hiddenCount++;
   } while (sqlStatement->FetchRow(false));

   if (hiddenCount)
      ip_Socket->Send("<tfoot><tr><td colspan=\"7\">User %s has found %d prime%s and %d PRP%s, %d %s hidden</td></tr></tfoot>",
                     prevUserID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount),
                     hiddenCount, PLURAL_COPULA(hiddenCount));
   else
      ip_Socket->Send("<tfoot><tr><td colspan=\"7\">User %s has found %d prime%s and %d PRP%s</td></tr></tfoot>",
                     prevUserID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount));
   ip_Socket->Send("</table></article>");

   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     PrimeHTMLGenerator::PrimesByTeam(void)
{
   SQLStatement *sqlStatement;
   int64_t    dateReported;
   int32_t    primeCount, prpCount, showOnWebPage, hiddenCount;
   int32_t    testResult;
   char       userID[ID_LENGTH+1], prevTeamID[ID_LENGTH+1];
   char       candidateName[NAME_LENGTH+1], machineID[ID_LENGTH+1];
   char       instanceID[ID_LENGTH+1], teamID[ID_LENGTH+1];
   double     decimalLength;

   const char *theSelect = "select UserID, TeamID, MachineID, InstanceID, TestedNumber, TestResult, " \
                           "       DateReported, DecimalLength, ShowOnWebPage " \
                           "  from UserPrimes " \
                           " where $null_func$(TeamID, ' ') <> ' ' " \
                           "order by TeamID, DecimalLength";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(candidateName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&testResult);
   sqlStatement->BindSelectedColumn(&dateReported);
   sqlStatement->BindSelectedColumn(&decimalLength);
   sqlStatement->BindSelectedColumn(&showOnWebPage);

   ip_Socket->StartBuffering();

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No primes found"))
      return;

   *prevTeamID = 0;
   hiddenCount = primeCount = prpCount = 0;
   do
   {
      if (strcmp(prevTeamID, teamID))
      {
         if (*prevTeamID)
         {
            if (hiddenCount)
               ip_Socket->Send("<tfoot><tr><td colspan=\"7\">Team %s has found %d prime%s and %d PRP%s, %d %s hidden</td></tr></tfoot>",
                              prevTeamID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount),
                              hiddenCount, PLURAL_COPULA(hiddenCount));
            else
               ip_Socket->Send("<tfoot><tr><td colspan=\"7\">Team %s has found %d prime%s and %d PRP%s</td></tr></tfoot>",
                              prevTeamID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount));
            ip_Socket->Send("</table>");
         }

         ip_Socket->Send("<h3>Primes and PRPs for Team %s</h3>", teamID);

         ip_Socket->Send("<table class=\"primes-by-team-table\"><thead><tr>");
         TH_CLMN_HDR("Candidate");
         TH_CLMN_HDR("Prime/PRP");
         TH_CLMN_HDR("User");
         TH_CLMN_HDR("Machine");
         TH_CLMN_HDR("Instance");
         TH_CLMN_HDR("Date Reported");
         TH_CLMN_HDR("Decimal Length");
         ip_Socket->Send("</tr></thead>");

         strcpy(prevTeamID, teamID);
         hiddenCount = primeCount = prpCount = 0;
      }

      if (testResult == R_PRIME)
         primeCount++;
      else
         prpCount++;

      if (showOnWebPage)
      {
         ip_Socket->Send("<tbody><tr>");

         TD_CHAR(candidateName);
         TD_CHAR(((testResult == R_PRIME) ? "Prime" : "PRP"));
         TD_CHAR(userID);
         TD_CHAR(machineID);
         TD_CHAR(instanceID);
         TD_CHAR(TimeToString(dateReported).c_str());
         TD_FLOAT(decimalLength);

         ip_Socket->Send("</tr></tbody>");
      }
      else
         hiddenCount++;
   } while (sqlStatement->FetchRow(false));

   if (hiddenCount)
      ip_Socket->Send("<tfoot><tr><td colspan=\"7\">Team %s has found %d prime%s and %d PRP%s, %d %s hidden</td></tr></tfoot>",
                     prevTeamID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount),
                     hiddenCount, PLURAL_COPULA(hiddenCount));
   else
      ip_Socket->Send("<tfoot><tr><td colspan=\"7\">Team %s has found %d prime%s and %d PRP%s</td></tr></tfoot>",
                     prevTeamID, primeCount, PLURAL_ENDING(primeCount), prpCount, PLURAL_ENDING(prpCount));
   ip_Socket->Send("</table></article>");

   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     PrimeHTMLGenerator::GFNDivisorsByUser(void)
{
   SQLStatement *sqlStatement;
   int64_t    dateReported;
   int32_t    gfnDivisorCount;
   char       userID[ID_LENGTH+1], prevUserID[ID_LENGTH+1];
   char       dividesGFN[51], testedNumber[51];

   const char *theSelect = "select CandidateGFNDivisor.UserID, GFN, DateReported, " \
                           "       CandidateGFNDivisor.TestedNumber " \
                           "  from CandidateTest, CandidateGFNDivisor, UserPrimes " \
                           " where CandidateTest.UserID = CandidateGFNDivisor.UserID " \
                           "   and CandidateTest.CandidateName = CandidateGFNDivisor.CandidateName " \
                           "   and CandidateTest.EmailID = CandidateGFNDivisor.EmailID " \
                           "   and CandidateTest.MachineID = CandidateGFNDivisor.MachineID " \
                           "   and CandidateTest.InstanceID = CandidateGFNDivisor.InstanceID " \
                           "   and UserPrimes.UserID = CandidateGFNDivisor.UserID " \
                           "   and UserPrimes.CandidateName = CandidateGFNDivisor.CandidateName " \
                           "   and ShowOnWebPage = 1 " \
                           "order by UserID, TestedNumber, GFN";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, sizeof(userID));
   sqlStatement->BindSelectedColumn(dividesGFN, sizeof(dividesGFN));
   sqlStatement->BindSelectedColumn(&dateReported);
   sqlStatement->BindSelectedColumn(testedNumber, sizeof(testedNumber));

   ip_Socket->StartBuffering();

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No GFN divisors found"))
      return;

   *prevUserID = 0;
   gfnDivisorCount = 0;
   do
   {
      if (strcmp(prevUserID, userID))
      {
         if (*prevUserID)
         {
            ip_Socket->Send("<tr><td style=\"text-align: center;\" colspan=\"4\">User %s has found %d GFN divisor%s</td></tr>",
                           prevUserID, gfnDivisorCount, PLURAL_ENDING(gfnDivisorCount));
            ip_Socket->Send("</table>");
         }

         ip_Socket->Send("<table class=\"gfn-divisors-by-user-table sortable\"><thead>");
         ip_Socket->Send("<tr><th colspan=\"3\">GFN Divisors for User %s</th></tr><tr>", userID);

         TH_CLMN_HDR("Tested Number");
         TH_CLMN_HDR("Divides GFN");
         TH_CLMN_HDR("Date Reported");

         ip_Socket->Send("</tr></thead><tbody>");

         strcpy(prevUserID, userID);
         gfnDivisorCount = 0;
      }

      gfnDivisorCount++;

      ip_Socket->Send("<tr><th scope=\"row\">%s</th>", testedNumber);
      TD_CHAR(dividesGFN);
      TD_CHAR(TimeToString(dateReported).c_str());

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));
   ip_Socket->Send("</tbody><tfoot>");
   ip_Socket->Send("<tr><td colspan=\"3\">User %s has found %d GFN divisor%s</td></tr>",
                  prevUserID, gfnDivisorCount, PLURAL_ENDING(gfnDivisorCount));
   ip_Socket->Send("</tfoot></table></article>");

   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     PrimeHTMLGenerator::UserStats(void)
{
   SQLStatement *sqlStatement;
   char        userID[ID_LENGTH+1];
   int32_t     testsPerformed, prpsFound, primesFound, gfnDivisorsFound;
   double      totalScore;

   const char *theSelect = "select UserID, TestsPerformed, PRPsFound, " \
                           "       PrimesFound, GFNDivisorsFound, TotalScore " \
                           "  from UserStats " \
                           "order by TotalScore desc";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, sizeof(userID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&prpsFound);
   sqlStatement->BindSelectedColumn(&primesFound);
   sqlStatement->BindSelectedColumn(&gfnDivisorsFound);
   sqlStatement->BindSelectedColumn(&totalScore);

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No user stats found"))
      return;

   ip_Socket->StartBuffering();
   ip_Socket->Send("<div class=\"header-box\">");
   ip_Socket->Send("<span><span>User Stats</span></span>");
   ip_Socket->Send("</div>");
   ip_Socket->Send("<table id=\"user-stats-table\" class=\"sortable\">");

   ip_Socket->Send("<thead><tr>");
   TH_CLMN_HDR("User");
   TH_CLMN_HDR("Total Score");
   TH_CLMN_HDR("Tests Performed");
   TH_CLMN_HDR("PRPs Found");
   TH_CLMN_HDR("Primes Found");
   if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
   ip_Socket->Send("</tr></thead><tbody>");

   do
   {
      ip_Socket->Send("<tr>");

      TH_ROW_HDR(userID);
      TD_FLOAT(totalScore);
      TD_32BIT(testsPerformed);
      TD_32BIT(prpsFound);
      TD_32BIT(primesFound);
      if (CanCheckForGFNs()) TD_32BIT(gfnDivisorsFound);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");
   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     PrimeHTMLGenerator::UserTeamStats(void)
{
   SQLStatement *sqlStatement;
   char        userID[ID_LENGTH+1], teamID[ID_LENGTH+1];
   int32_t     testsPerformed, prpsFound, primesFound, gfnDivisorsFound;
   double      totalScore;

   const char *theSelect = "select UserID, TeamID, TestsPerformed, PRPsFound, " \
                           "       PrimesFound, GFNDivisorsFound, TotalScore " \
                           "  from UserTeamStats " \
                           "order by UserID, TotalScore, TeamID";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, sizeof(userID));
   sqlStatement->BindSelectedColumn(teamID, sizeof(teamID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&prpsFound);
   sqlStatement->BindSelectedColumn(&primesFound);
   sqlStatement->BindSelectedColumn(&gfnDivisorsFound);
   sqlStatement->BindSelectedColumn(&totalScore);

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No user team stats found"))
      return;

   ip_Socket->StartBuffering();
   ip_Socket->Send("<table id=\"user-team-stats-table\" class=\"sortable\">");

   ip_Socket->Send("<thead><tr>");
   TH_CLMN_HDR("User");
   TH_CLMN_HDR("Team");
   TH_CLMN_HDR("Total Score");
   TH_CLMN_HDR("Tests Performed");
   TH_CLMN_HDR("PRPs Found");
   TH_CLMN_HDR("Primes Found");
   if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
   ip_Socket->Send("</tr></thead><tbody>");

   do
   {
      ip_Socket->Send("<tr>");

      TD_CHAR(userID);
      TD_CHAR(teamID);
      TD_FLOAT(totalScore);
      TD_32BIT(testsPerformed);
      TD_32BIT(prpsFound);
      TD_32BIT(primesFound);
      if (CanCheckForGFNs()) TD_32BIT(gfnDivisorsFound);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");
   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     PrimeHTMLGenerator::TeamUserStats(void)
{
   SQLStatement *sqlStatement;
   char        userID[ID_LENGTH+1], teamID[ID_LENGTH+1];
   int32_t     testsPerformed, prpsFound, primesFound, gfnDivisorsFound;
   double      totalScore;

   const char *theSelect = "select UserID, TeamID, TestsPerformed, PRPsFound, " \
                           "       PrimesFound, GFNDivisorsFound, TotalScore " \
                           "  from UserTeamStats " \
                           "order by TeamID, TotalScore, UserID";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, sizeof(userID));
   sqlStatement->BindSelectedColumn(teamID, sizeof(teamID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&prpsFound);
   sqlStatement->BindSelectedColumn(&primesFound);
   sqlStatement->BindSelectedColumn(&gfnDivisorsFound);
   sqlStatement->BindSelectedColumn(&totalScore);

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No teams found"))
      return;

   ip_Socket->StartBuffering();
   ip_Socket->Send("<table id=\"team-user-stats-table\" class=\"sortable\">");

   ip_Socket->Send("<thead><tr>");
   TH_CLMN_HDR("Team");
   TH_CLMN_HDR("User");
   TH_CLMN_HDR("Total Score");
   TH_CLMN_HDR("Tests Performed");
   TH_CLMN_HDR("PRPs Found");
   TH_CLMN_HDR("Primes Found");
   if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
   ip_Socket->Send("</tr></thead><tbody>");

   do
   {
      ip_Socket->Send("<tr>");

      TD_CHAR(teamID);
      TD_CHAR(userID);
      TD_FLOAT(totalScore);
      TD_32BIT(testsPerformed);
      TD_32BIT(prpsFound);
      TD_32BIT(primesFound);
      if (CanCheckForGFNs()) TD_32BIT(gfnDivisorsFound);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");
   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     PrimeHTMLGenerator::TeamStats(void)
{
   SQLStatement *sqlStatement;
   char        teamID[ID_LENGTH+1];
   int32_t     testsPerformed, prpsFound, primesFound, gfnDivisorsFound;
   double      totalScore;

   const char *theSelect = "select TeamID, TestsPerformed, PRPsFound, " \
                           "       PrimesFound, GFNDivisorsFound, TotalScore " \
                           "  from TeamStats " \
                           "order by TotalScore desc";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(teamID, sizeof(teamID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&prpsFound);
   sqlStatement->BindSelectedColumn(&primesFound);
   sqlStatement->BindSelectedColumn(&gfnDivisorsFound);
   sqlStatement->BindSelectedColumn(&totalScore);

   ip_Socket->Send("<article>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No team stats found"))
      return;

   ip_Socket->StartBuffering();
   ip_Socket->Send("<table id=\"team-stats-table\" class=\"sortable\">");

   ip_Socket->Send("<thead><tr>");
   TH_CLMN_HDR("Team");
   TH_CLMN_HDR("Total Score");
   TH_CLMN_HDR("Tests Performed");
   TH_CLMN_HDR("PRPs Found");
   TH_CLMN_HDR("Primes Found");
   if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
   ip_Socket->Send("</tr></thead><tbody>");

   do
   {
      ip_Socket->Send("<tr>");

      TH_ROW_HDR(teamID);
      TD_FLOAT(totalScore);
      TD_32BIT(testsPerformed);
      TD_32BIT(prpsFound);
      TD_32BIT(primesFound);
      if (CanCheckForGFNs()) TD_32BIT(gfnDivisorsFound);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");
   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     PrimeHTMLGenerator::SendLinks()
{
   ip_Socket->Send("<nav class=\"%s\">", (CanCheckForGFNs() ? "five-track" : "four-track"));

   ip_Socket->Send("<div><a href=\"server_stats.html\">Server Statistics</a></div>");
   ip_Socket->Send("<div><a href=\"pending_tests.html\">Pending Tests</a></div>");
   ip_Socket->Send("<div><a href=\"user_stats.html\">User Statistics</a></div>");
   ip_Socket->Send("<div><a href=\"user_primes.html\">User Primes</a></div>");
   if (CanCheckForGFNs())
      ip_Socket->Send("<div><a href=\"user_gfns.html\">User GFNs</a></div>");
   if (HasTeams())
   {
      ip_Socket->Send("<div><a href=\"userteam_stats.html\">User/Team Statistics</a></div>");
      ip_Socket->Send("<div><a href=\"teamuser_stats.html\">Team/User Statistics</a></div>");
      ip_Socket->Send("<div><a href=\"team_stats.html\">Team Statistics</a></div>");
      ip_Socket->Send("<div><a href=\"team_primes.html\">Team Primes</a></div>");
      if (CanCheckForGFNs())
        ip_Socket->Send("<div>&nbsp;</div>");
   }
   ip_Socket->Send("</nav><div style=\"clear: both;\"></div>");
}

bool     PrimeHTMLGenerator::CanCheckForGFNs(void)
{
   static time_t  lastCheck = 0;
   static bool    doesGFNChecks;
   SQLStatement  *sqlStatement;
   int32_t        theB;
   const char    *selectSQL = "select b from CandidateGroupStats where c = 1 and b > 0";

   // This will check no more than once every ten minutes
   if (time(NULL) - lastCheck < 600)
      return doesGFNChecks;

   doesGFNChecks = true;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindSelectedColumn(&theB);

   while (sqlStatement->FetchRow(false) && doesGFNChecks)
   {
      while (theB > 1)
      {
         if (theB & 1)
            doesGFNChecks = false;
         theB <<= 1;
      }
   }

   if (sqlStatement->GetFetchedRowCount() == 0)
      doesGFNChecks = false;

   sqlStatement->CloseCursor();
   delete sqlStatement;

   lastCheck = time(NULL);
   return doesGFNChecks;
}

ServerHelper *PrimeHTMLGenerator::GetServerHelper(void)
{
   ServerHelper *serverHelper = new PrimeServerHelper(ip_DBInterface, ip_Log);
   return serverHelper;
}

// The server_stats.html table for all of the searches has the same column headings with the exception 
// of the Min/Max columns.
void     PrimeHTMLGenerator::ServerStatsHeader(sss_t summarizedBy)
{
   ip_Socket->Send("<article><table id=\"server-stats-table\" class=\"sortable\"><thead><tr>");

   TH_CLMN_HDR("Form");
   TH_CLMN_HDR("Total Candidates");

   if (summarizedBy == BY_K)
   {
      TH_CLMN_HDR("Min <var>K</var>");
      TH_CLMN_HDR("Max <var>K</var>");
   }

   if (summarizedBy == BY_B)
   {
      TH_CLMN_HDR("Min <var>B</var>");
      TH_CLMN_HDR("Max <var>B</var>");
   }

   if (summarizedBy == BY_Y)
   {
      TH_CLMN_HDR("Min <var>Y</var>");
      TH_CLMN_HDR("Max <var>Y</var>");
   }

   if (summarizedBy == BY_N)
   {
      TH_CLMN_HDR("Min <var>N</var>");
      TH_CLMN_HDR("Max <var>N</var>");
   }

   TH_CLMN_HDR("Count Tested");
   TH_CH_IF_DC("Count DC\'d");
   TH_CLMN_HDR("Count Untested");
   TH_CLMN_HDR("In Progress");
   TH_CLMN_HDR("Completed Thru");
   TH_CLMN_HDR("Leading Edge");
   TH_CLMN_HDR("PRPs/Primes");

   ip_Socket->Send("</tr></thead><tbody>");
}
