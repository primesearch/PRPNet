#include "PrimeHTMLGenerator.h"
#include "PrimeServerHelper.h"
#include "SQLStatement.h"

void     PrimeHTMLGenerator::Send(string thePage)
{
   char tempPage[200];

   strcpy(tempPage, thePage.c_str());

   ip_Socket->Send("HTTP/1.1 200 OK");
   ip_Socket->Send("Connection: close");

   // The client needs this extra carriage return before sending HTML
   ip_Socket->Send("\n");

   if (!strcmp(tempPage, "user_stats.html"))
   {
      HeaderPlusLinks("User Statistics");
      UserStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "user_primes.html"))
   {
      HeaderPlusLinks("Primes by User");
      PrimesByUser();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "user_gfns.html") && CanCheckForGFNs())
   {
      HeaderPlusLinks("GFN Divisors by User");
      GFNDivisorsByUser();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "team_stats.html") && HasTeams())
   {
      HeaderPlusLinks("Team Statistics");
      TeamStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "team_primes.html") && HasTeams())
   {
      HeaderPlusLinks("Primes by Team");
      PrimesByTeam();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "userteam_stats.html") && HasTeams())
   {
      HeaderPlusLinks("User/Team Statistics");
      UserTeamStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "teamuser_stats.html") && HasTeams())
   {
      HeaderPlusLinks("Team/User Statistics");
      TeamUserStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "pending_tests.html"))
   {
      HeaderPlusLinks("Pending Tests");
      PendingTests();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "all.html"))
   {
      HeaderPlusLinks("Status");

      PendingTests();
      ip_Socket->Send("<br>");
      ServerStats();
      ip_Socket->Send("<br>");
      UserStats();
      ip_Socket->Send("<br>");
      if (HasTeams())
      {
         TeamStats();
         ip_Socket->Send("<br>");
         PrimesByUser();
         ip_Socket->Send("<br>");
         PrimesByTeam();
         ip_Socket->Send("<br>");
         TeamUserStats();
         ip_Socket->Send("<br>");
      }
      if (CanCheckForGFNs())
      {
         GFNDivisorsByUser();
         ip_Socket->Send("<br>");
      }

      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "server_stats.html") || !strlen(tempPage))
   {
      HeaderPlusLinks("Server Statistics");
      GetDaysLeft();
      ServerStats();
      ip_Socket->Send("</body></html>");
   }
   else
   {
      HeaderPlusLinks("");
      ip_Socket->Send("<br><br><br><p align=center><b><font size=4 color=#FF0000>Page %s does not exist on this server.</font></b></p>", thePage.c_str());
      ip_Socket->Send("<p align=center><b><font size=4 color=#FF0000>Better luck next time!</font></b></p>");
   }

   // The client needs this extra carriage return before closing the socket
   ip_Socket->Send("\n");
   return;
}

void     PrimeHTMLGenerator::PendingTests(void)
{
   SQLStatement *sqlStatement;
   int64_t     testID, expireSeconds;
   char        candidatName[NAME_LENGTH+1], userID[ID_LENGTH+1];
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

   sqlStatement->BindSelectedColumn(candidatName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&decimalLength);
   sqlStatement->BindSelectedColumn(&testID);
   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(teamID, ID_LENGTH);

   currentTime = time(NULL);

   if (!sqlStatement->FetchRow(false))
   {
	  ip_Socket->Send("<table frame=box align=center border=1>");
	  ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>No Pending WU\'s</tr>");
	  ip_Socket->Send("</table>");
   }
   else
   {
      ip_Socket->Send("<table frame=box align=center border=1 class=sortable><tr class=headercolor>");
      TH_CLMN_HDR("Candidate");
      TH_CLMN_HDR("User");
      TH_CLMN_HDR("Machine");
      TH_CLMN_HDR("Instance");
      if (HasTeams()) TH_CLMN_HDR("Team");
      TH_CLMN_HDR("Date Assigned");
      TH_CLMN_HDR("Age (hh:mm)");
      TH_CLMN_HDR("Expires (hh:mm)");
      ip_Socket->Send("</tr>");

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

         TD_CHAR(candidatName);
         TD_CHAR(userID);
         TD_CHAR(machineID);
         TD_CHAR(instanceID);
         if (HasTeams()) TD_CHAR(teamID);
         TD_CHAR(TimeToString(testID).c_str());
         TD_TIME(hours, minutes);
         TD_TIME(expireHours, expireMinutes);

         ip_Socket->Send("</tr>");
      } while (sqlStatement->FetchRow(false));
   }

   ip_Socket->Send("</table>");

   delete sqlStatement;
}

void     PrimeHTMLGenerator::PrimesByUser(void)
{
   SQLStatement *sqlStatement;
   int64_t    dateReported;
   int32_t    primeCount, prpCount, showOnWebPage, hiddenCount;
   int32_t    testResult;
   char       userID[ID_LENGTH+1], prevUserID[ID_LENGTH+1];
   char       candidatName[NAME_LENGTH+1], machineID[ID_LENGTH+1];
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
   sqlStatement->BindSelectedColumn(candidatName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&testResult);
   sqlStatement->BindSelectedColumn(&dateReported);
   sqlStatement->BindSelectedColumn(&decimalLength);
   sqlStatement->BindSelectedColumn(&showOnWebPage);

   ip_Socket->StartBuffering();

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr align=center><td class=headertext>No primes found</tr>");
      ip_Socket->Send("</table>");
   }
   else
   {
      *prevUserID = 0;
      hiddenCount = primeCount = prpCount = 0;
      ip_Socket->Send("<table frame=box align=center border=1>");
      do
      {
         if (strcmp(prevUserID, userID))
         {
            if (*prevUserID)
            {
               if (hiddenCount)
                  ip_Socket->Send("<tr><td align=center colspan=7>User %s has found %d prime%s and %d PRP%s, %d %s hidden</tr>",
                                  prevUserID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"),
                                  hiddenCount, ((hiddenCount>1) ? "are" : "is"));
               else
                  ip_Socket->Send("<tr><td align=center colspan=7>User %s has found %d prime%s and %d PRP%s</tr>",
                                  prevUserID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"));
               ip_Socket->Send("<tr><td colspan=7>&nbsp;</tr>");
            }

            ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=7>Primes and PRPs for User %s</tr>", userID);

            ip_Socket->Send("<tr class=headercolor>");
            TH_CLMN_HDR("Candidate");
            TH_CLMN_HDR("Prime/PRP");
            TH_CLMN_HDR("Machine");
            TH_CLMN_HDR("Instance");
            TH_CLMN_HDR("Team");
            TH_CLMN_HDR("Date Reported");
            TH_CLMN_HDR("Decimal Length");
            ip_Socket->Send("</tr>");

            strcpy(prevUserID, userID);
            hiddenCount = primeCount = prpCount = 0;
         }

         if (testResult == R_PRIME)
            primeCount++;
         else
            prpCount++;

         if (showOnWebPage)
         {
            ip_Socket->Send("<tr>");

            TD_CHAR(candidatName);
            TD_CHAR(((testResult == R_PRIME) ? "Prime" : "PRP"));
            TD_CHAR(machineID);
            TD_CHAR(instanceID);
            TD_CHAR(teamID);
            TD_CHAR(TimeToString(dateReported).c_str());
            TD_32BIT((uint32_t) decimalLength);

            ip_Socket->Send("</tr>");
         }
         else
            hiddenCount++;
      } while (sqlStatement->FetchRow(false));

      if (hiddenCount)
         ip_Socket->Send("<tr><td align=center colspan=7>User %s has found %d prime%s and %d PRP%s, %d %s hidden</tr>",
                         prevUserID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"),
                         hiddenCount, ((hiddenCount>1) ? "are" : "is"));
      else
         ip_Socket->Send("<tr><td align=center colspan=7>User %s has found %d prime%s and %d PRP%s</tr>",
                         prevUserID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"));
      ip_Socket->Send("</table>");
   }

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
   char       candidatName[NAME_LENGTH+1], machineID[ID_LENGTH+1];
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
   sqlStatement->BindSelectedColumn(candidatName, NAME_LENGTH);
   sqlStatement->BindSelectedColumn(&testResult);
   sqlStatement->BindSelectedColumn(&dateReported);
   sqlStatement->BindSelectedColumn(&decimalLength);
   sqlStatement->BindSelectedColumn(&showOnWebPage);

   ip_Socket->StartBuffering();

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr align=center><td class=headertext>No primes found</tr>");
      ip_Socket->Send("</table>");
   }
   else
   {
      *prevTeamID = 0;
      hiddenCount = primeCount = prpCount = 0;
      ip_Socket->Send("<table frame=box align=center border=1>");
      do
      {
         if (strcmp(prevTeamID, teamID))
         {
            if (*prevTeamID)
            {
               if (hiddenCount)
                  ip_Socket->Send("<tr><td align=center colspan=7>Team %s has found %d prime%s and %d PRP%s, %d %s hidden</tr>",
                                  prevTeamID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"),
                                  hiddenCount, ((hiddenCount>1) ? "are" : "is"));
               else
                  ip_Socket->Send("<tr><td align=center colspan=7>Team %s has found %d prime%s and %d PRP%s</tr>",
                                  prevTeamID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"));
               ip_Socket->Send("<tr><td colspan=7>&nbsp;</tr>");
            }

            ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=7>Primes and PRPS for Team %s</tr>", teamID);

            ip_Socket->Send("<tr class=headercolor>");
            TH_CLMN_HDR("Candidate");
            TH_CLMN_HDR("Prime/PRP");
            TH_CLMN_HDR("User");
            TH_CLMN_HDR("Machine");
            TH_CLMN_HDR("Instance");
            TH_CLMN_HDR("Date Reported");
            TH_CLMN_HDR("Decimal Length");
            ip_Socket->Send("</tr>");

            strcpy(prevTeamID, teamID);
            hiddenCount = primeCount = prpCount = 0;
         }

         if (testResult == R_PRIME)
            primeCount++;
         else
            prpCount++;

         if (showOnWebPage)
         {
            ip_Socket->Send("<tr>");

            TD_CHAR(candidatName);
            TD_CHAR(((testResult == R_PRIME) ? "Prime" : "PRP"));
            TD_CHAR(userID);
            TD_CHAR(machineID);
            TD_CHAR(instanceID);
            TD_CHAR(TimeToString(dateReported).c_str());
            TD_FLOAT(decimalLength);

            ip_Socket->Send("</tr>");
         }
         else
            hiddenCount++;
      } while (sqlStatement->FetchRow(false));

      if (hiddenCount)
         ip_Socket->Send("<tr><td align=center colspan=7>Team %s has found %d prime%s and %d PRP%s, %d %s hidden</tr>",
                         prevTeamID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"),
                         hiddenCount, ((hiddenCount>1) ? "are" : "is"));
      else
         ip_Socket->Send("<tr><td align=center colspan=7>Team %s has found %d prime%s and %d PRP%s</tr>",
                         prevTeamID, primeCount, ((primeCount==1) ? "" : "s"), prpCount, ((prpCount==1) ? "" : "s"));
      ip_Socket->Send("</table>");
   }

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

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr align=center><td>No GFN divisors found</tr>");
      ip_Socket->Send("</table>");
   }
   else
   {
      *prevUserID = 0;
      gfnDivisorCount = 0;
      do
      {
         if (strcmp(prevUserID, userID))
         {
            if (*prevUserID)
            {
               ip_Socket->Send("<tr><td align=center colspan=4>User %s has found %d GFN divisor%s</tr>",
                               prevUserID, gfnDivisorCount, ((gfnDivisorCount>1) ? "s" : ""));
               ip_Socket->Send("</table><p><p>");
            }

            ip_Socket->Send("<table frame=box align=center border=1 class=sortable>");
            ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=4>GFN Divisors for User %s</tr>", userID);
            ip_Socket->Send("<tr class=headercolor><th class=headertext>Tested Number<th class=headertext>Divides GFN<th class=headertext>Date Reported</tr>");

            strcpy(prevUserID, userID);
            gfnDivisorCount = 0;
         }

         gfnDivisorCount++;

         ip_Socket->Send("<tr><td align=center>%s<td>%s<td>%s</tr>",
                         testedNumber, dividesGFN, TimeToString(dateReported).c_str());
      } while (sqlStatement->FetchRow(false));

       ip_Socket->Send("<tr><td align=center colspan=4>User %s has found %d GFN divisor%s</tr>",
                       prevUserID, gfnDivisorCount, ((gfnDivisorCount>1) ? "s" : ""));
      ip_Socket->Send("</table>");
   }

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

   if (!sqlStatement->FetchRow(false))
   {
	  ip_Socket->Send("<table frame=box align=center border=1>");
	  ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>User Stats</tr>");
	  ip_Socket->Send("<tr align=center><td class=headertext>No user stats found</tr>");
	  ip_Socket->Send("</table>");
   }
   else
   {
      ip_Socket->StartBuffering();
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=6>User Stats</tr>");
      ip_Socket->Send("</table><p>");
      ip_Socket->Send("<table frame=box align=center border=1 class=sortable>");

      ip_Socket->Send("<tr class=headercolor>");
      TH_CLMN_HDR("User");
      TH_CLMN_HDR("Total Score");
      TH_CLMN_HDR("Tests Performed");
      TH_CLMN_HDR("PRPs Found");
      TH_CLMN_HDR("Primes Found");
      if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
      ip_Socket->Send("</tr>");

      do
      {
         ip_Socket->Send("<tr>");

         TD_CHAR(userID);
         TD_FLOAT(totalScore);
         TD_32BIT(testsPerformed);
         TD_32BIT(prpsFound);
         TD_32BIT(primesFound);
         if (CanCheckForGFNs()) TD_32BIT(gfnDivisorsFound);

         ip_Socket->Send("</tr>");
      } while (sqlStatement->FetchRow(false));

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

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

   if (!sqlStatement->FetchRow(false))
   {
	  ip_Socket->Send("<table frame=box align=center border=1>");
	  ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>User Team Stats</tr>");
	  ip_Socket->Send("<tr align=center><td class=headertext>No user team stats found</tr>");
	  ip_Socket->Send("</table>");
   }
   else
   {
      ip_Socket->StartBuffering();
      ip_Socket->Send("<table frame=box align=center border=1 class=sortable>");

      ip_Socket->Send("<tr class=headercolor>");
      TH_CLMN_HDR("User");
      TH_CLMN_HDR("Team");
      TH_CLMN_HDR("Total Score");
      TH_CLMN_HDR("Tests Performed");
      TH_CLMN_HDR("PRPs Found");
      TH_CLMN_HDR("Primes Found");
      if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
      ip_Socket->Send("</tr>");

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

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

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

   if (!sqlStatement->FetchRow(false))
   {
	  ip_Socket->Send("<table frame=box align=center border=1>");
	  ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>User Stats by Team</tr>");
	  ip_Socket->Send("<tr align=center><td class=headertext>No teams found</tr>");
	  ip_Socket->Send("</table>");
   }
   else
   {
      ip_Socket->StartBuffering();
      ip_Socket->Send("<table frame=box align=center border=1 class=sortable>");

      ip_Socket->Send("<tr class=headercolor>");
      TH_CLMN_HDR("Team");
      TH_CLMN_HDR("User");
      TH_CLMN_HDR("Total Score");
      TH_CLMN_HDR("Tests Performed");
      TH_CLMN_HDR("PRPs Found");
      TH_CLMN_HDR("Primes Found");
      if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
      ip_Socket->Send("</tr>");

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

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

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

   if (!sqlStatement->FetchRow(false))
   {
	  ip_Socket->Send("<table frame=box align=center border=1>");
	  ip_Socket->Send("<tr align=center><td class=headertext>No team stats found</tr>");
	  ip_Socket->Send("</table>");
   }
   else
   {
      ip_Socket->StartBuffering();
      ip_Socket->Send("<table frame=box align=center border=1>");

      ip_Socket->Send("<tr class=headercolor>");
      TH_CLMN_HDR("Team");
      TH_CLMN_HDR("Total Score");
      TH_CLMN_HDR("Tests Performed");
      TH_CLMN_HDR("PRPs Found");
      TH_CLMN_HDR("Primes Found");
      if (CanCheckForGFNs()) TH_CLMN_HDR("GFN Divisors Found");
      ip_Socket->Send("</tr>");

      do
      {
         ip_Socket->Send("<tr>");

         TD_CHAR(teamID);
         TD_FLOAT(totalScore);
         TD_32BIT(testsPerformed);
         TD_32BIT(prpsFound);
         TD_32BIT(primesFound);
         if (CanCheckForGFNs()) TD_32BIT(gfnDivisorsFound);

         ip_Socket->Send("</tr>");
      } while (sqlStatement->FetchRow(false));

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

   delete sqlStatement;
}

string   PrimeHTMLGenerator::TimeToString(time_t inTime)
{
   static char   outTime[80];
   struct tm *ltm;
   char   timeZone[100];
   char  *ptr1, *ptr2;

   while (true)
   {
     if (ib_UseLocalTime)
        ltm = localtime(&inTime);
     else
        ltm = gmtime(&inTime);

     if (!ltm)
        continue;

     if (ib_UseLocalTime)
     {
        strftime(timeZone, sizeof(timeZone), "%Z", ltm);

        ptr1 = timeZone;
	     ptr1++;
        ptr2 = ptr1;
        while (true)
        {
           while (*ptr2 && *ptr2 != ' ')
	           ptr2++;

           if (!*ptr2) break;

           ptr2++;
           if (!*ptr2) break;

           *ptr1 = *ptr2;
           ptr1++;
           *ptr1 = 0;
        }
     }
     else
        strcpy(timeZone, "GMT");

     strftime(outTime, sizeof(outTime), "%Y-%m-%d %X ", ltm);
     strcat(outTime, timeZone);

     return outTime;
   }
}

void     PrimeHTMLGenerator::HeaderPlusLinks(string pageTitle)
{
   ip_Socket->StartBuffering();

   ip_Socket->Send("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">");
   ip_Socket->Send("<title>PRPNet %s %s - %s</title>", PRPNET_VERSION, pageTitle.c_str(), is_HTMLTitle.c_str());

   if (is_SortLink.length() > 0)
		ip_Socket->Send("<script src=\"%s\"></script>", is_SortLink.c_str());

   ip_Socket->Send("<link rel=\"icon\" type=\"image/gif\" href=\"data:image/gif;base64,R0lGODlhEAAQAIABAAAAAP///yH5BAEAAAEALAAAAAAQABAAQAIijI9pwBDtoJq0Wuue1rmjuFziSB7S2YRc6G1L5qoqWNZIAQA7\">");
	
   if (is_CSSLink.length() > 0)
      ip_Socket->Send("<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\" /></head><body>", is_CSSLink.c_str());

   ip_Socket->Send("<p align=center><b><font size=6>%s</font></b></p>", is_ProjectTitle.c_str());

   if (is_ProjectTitle != is_HTMLTitle)
      ip_Socket->Send("<p align=center><b><font size=4>%s - %s</font></b></p>", is_HTMLTitle.c_str(), pageTitle.c_str());

   ip_Socket->Send("<table frame=box align=center border=1><b><font size=4><tr>");

   ip_Socket->Send("<td><a href=server_stats.html>Server Statistics</a></td>");
   ip_Socket->Send("<td><a href=pending_tests.html>Pending Tests</a></td>");
   ip_Socket->Send("<td><a href=user_stats.html>User Statistics</a></td>");
   ip_Socket->Send("<td><a href=user_primes.html>User Primes</a></td>");
   if (CanCheckForGFNs())
      ip_Socket->Send("<td><a href=user_gfns.html>User GFNs</a></td>");
   if (HasTeams())
   {
      ip_Socket->Send("</tr><tr><td><a href=userteam_stats.html>User/Team Statistics</a></td>");
      ip_Socket->Send("<td><a href=teamuser_stats.html>Team/User Statistics</a></td>");
      ip_Socket->Send("<td><a href=team_stats.html>Team Statistics</a></td>");
      ip_Socket->Send("<td><a href=team_primes.html>Team Primes</a></td>");
   }
   ip_Socket->Send("</tr></font></b></table><br>");

   ip_Socket->SendBuffer();
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

bool     PrimeHTMLGenerator::HasTeams(void)
{
   static time_t  lastCheck = 0;
   static bool    hasTeams;
   SQLStatement  *sqlStatement;
   int32_t        theCount = 0;
   const char    *selectSQL = "select count(*) from TeamStats";

   // This will check no more than once every ten minutes
   if (time(NULL) - lastCheck < 600)
      return hasTeams;

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, selectSQL);
   sqlStatement->BindSelectedColumn(&theCount);

   sqlStatement->FetchRow(true);

   hasTeams = (theCount > 0);

   sqlStatement->CloseCursor();
   delete sqlStatement;

   lastCheck = time(NULL);
   return hasTeams;
}

void     PrimeHTMLGenerator::GetDaysLeft(void)
{
   PrimeServerHelper *serverHelper = new PrimeServerHelper(ip_DBInterface, ip_Log);
   int64_t hoursLeft = serverHelper->ComputeHoursRemaining();
   int64_t daysLeft = hoursLeft / 24;

   ip_Socket->Send("<p align=center>");

   if (daysLeft < 10)
      ip_Socket->Send("<font color=\"red\">");

   if (hoursLeft < 72)
      ip_Socket->Send("Estimate of %" PRId64" hour%s before the server runs out of work<p><p>", hoursLeft, (hoursLeft > 1 ? "s" : ""));
   else
      ip_Socket->Send("Estimate of %" PRId64" day%s before the server runs out of work<p><p>", daysLeft, (daysLeft > 1 ? "s" : ""));

   delete serverHelper;
}

// The server_stats.html table for all of the searches has the same column headings with the exception 
// of the Min/Max columns.
void     PrimeHTMLGenerator::ServerStatsHeader(sss_t summarizedBy)
{
   ip_Socket->Send("<table frame=box align=center border=1 class=sortable><tr class=headercolor>");

   TH_CLMN_HDR("Form");
   TH_CLMN_HDR("Total Candidates");

   if (summarizedBy == BY_K)
   {
      TH_CLMN_HDR("Min K");
      TH_CLMN_HDR("Max K");
   }

   if (summarizedBy == BY_B)
   {
      TH_CLMN_HDR("Min B");
      TH_CLMN_HDR("Max B");
   }

   if (summarizedBy == BY_Y)
   {
      TH_CLMN_HDR("Min Y");
      TH_CLMN_HDR("Max Y");
   }

   if (summarizedBy == BY_N)
   {
      TH_CLMN_HDR("Min N");
      TH_CLMN_HDR("Max N");
   }

   TH_CLMN_HDR("Count Tested");
   TH_CH_IF_DC("Count DC\'d");
   TH_CLMN_HDR("Count Untested");
   TH_CLMN_HDR("In Progress");
   TH_CLMN_HDR("Completed Thru");
   TH_CLMN_HDR("Leading Edge");
   TH_CLMN_HDR("PRPs/Primes");

   ip_Socket->Send("</tr>");
}
