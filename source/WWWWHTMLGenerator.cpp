#include "WWWWHTMLGenerator.h"
#include "WWWWServerHelper.h"
#include "SQLStatement.h"

void     WWWWHTMLGenerator::Send(string thePage)
{
   char tempPage[200];

   if (ii_ServerType == ST_WIEFERICH)    strcpy(ic_SearchType, "Wieferich");
   if (ii_ServerType == ST_WILSON)       strcpy(ic_SearchType, "Wilson");
   if (ii_ServerType == ST_WALLSUNSUN)   strcpy(ic_SearchType, "Wall-Sun-Sun");
   if (ii_ServerType == ST_WOLSTENHOLME) strcpy(ic_SearchType, "Wolstenholme");

   ip_Socket->Send("HTTP/1.1 200 OK");
   ip_Socket->Send("Connection: close");

   strcpy(tempPage, thePage.c_str());

   // The client needs this extra carriage return before sending HTML
   ip_Socket->Send("\n");

   if (!strcmp(tempPage, "user_stats.html"))
   {
      HeaderPlusLinks("User Statistics");
      UserStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "user_finds.html"))
   {
      HeaderPlusLinks("Finds by User");
      FindsByUser();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "team_stats.html"))
   {
      HeaderPlusLinks("Team Statistics");
      TeamStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "team_finds.html"))
   {
      HeaderPlusLinks("Finds by Team");
      FindsByTeam();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "userteam_stats.html"))
   {
      HeaderPlusLinks("User/Team Statistics");
      UserTeamStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "teamuser_stats.html"))
   {
      HeaderPlusLinks("Team/User Statistics");
      TeamUserStats();
      ip_Socket->Send("</body></html>");
   }
   else if (!strcmp(tempPage, "pending_work.html"))
   {
      HeaderPlusLinks("Pending Work");
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
      TeamStats();
      ip_Socket->Send("<br>");
      FindsByUser();
      ip_Socket->Send("<br>");
      FindsByTeam();
      ip_Socket->Send("<br>");
      TeamUserStats();

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

void     WWWWHTMLGenerator::PendingTests(void)
{
   SQLStatement *sqlStatement;
   int64_t     testID, expireSeconds;
   char        userID[ID_LENGTH+1], machineID[ID_LENGTH+1];
   char        instanceID[ID_LENGTH+1], teamID[ID_LENGTH+1];
   string      lowerLimitStr, upperLimitStr;
   int64_t     lowerLimit, upperLimit;
   int32_t     seconds, hours, minutes;
   int32_t     expireHours, expireMinutes;
   time_t      currentTime;

   const char *theSelect = "select WWWWRangeTest.LowerLimit, WWWWRangeTest.UpperLimit, " \
                           "       WWWWRangeTest.TestID, WWWWRangeTest.UserID, " \
                           "       WWWWRangeTest.MachineID, WWWWRangeTest.InstanceID, " \
                           "       $null_func$(WWWWRangeTest.TeamID, ' ') " \
                           "  from WWWWRange, WWWWRangeTest" \
                           " where WWWWRange.LowerLimit = WWWWRangeTest.LowerLimit " \
                           "   and WWWWRange.UpperLimit = WWWWRangeTest.UpperLimit " \
                           "   and WWWWRange.HasPendingTest = 1 " \
                           "   and WWWWRangeTest.SearchingProgram is null " \
                           "order by WWWWRangeTest.TestID, WWWWRangeTest.LowerLimit ";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&lowerLimit);
   sqlStatement->BindSelectedColumn(&upperLimit);
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
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=8>Pending WU\'s</tr>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext>Range<th class=headertext>User");
      ip_Socket->Send("<th class=headertext>Machine<th class=headertext>Instance<th class=headertext>Team");
      ip_Socket->Send("<th class=headertext>Date Assigned<th class=headertext>");
      ip_Socket->Send("Age (hh:mm)<th class=headertext>Expires (hh:mm)");
      ip_Socket->Send("</tr>");

      do
      {
         expireSeconds = testID + ip_Delay[0].expireDelay - (int64_t) currentTime;

         if (expireSeconds < 0) expireSeconds = 0;
         expireHours = (int32_t) (expireSeconds / 3600);
         expireMinutes = (int32_t) (expireSeconds - expireHours * 3600) / 60;

         seconds = (int32_t) (currentTime - testID);
         hours = seconds / 3600;
         minutes = (seconds - hours * 3600) / 60;

         ConvertToScientificNotation(lowerLimit, lowerLimitStr);
         ConvertToScientificNotation(upperLimit, upperLimitStr);

         ip_Socket->Send("<tr><td align=center>%s - %s<td>%s<td>%s<td>%s<td>%s<td>%s<td align=center>%d:%02d<td align=center>%d:%02d</tr>",
                         lowerLimitStr.c_str(), upperLimitStr.c_str(), userID, machineID, instanceID, teamID,
                         TimeToString(testID), hours, minutes, expireHours, expireMinutes);
      } while (sqlStatement->FetchRow(false));
   }

   ip_Socket->Send("</table>");

   delete sqlStatement;
}

void     WWWWHTMLGenerator::FindsByUser(void)
{
   SQLStatement *sqlStatement;
   int64_t    dateReported, prime;
   int32_t    remainder, quotient;
   int32_t    foundCount, nearCount, showOnWebPage, hiddenCount;
   char       userID[ID_LENGTH+1], prevUserID[ID_LENGTH+1];
   char       machineID[ID_LENGTH+1], instanceID[ID_LENGTH+1], teamID[ID_LENGTH+1];

   const char *theSelect = "select UserID, TeamID, MachineID, InstanceID, Prime, Remainder, " \
                           "       Quotient, DateReported, ShowOnWebPage " \
                           "  from UserWWWWs " \
                           "order by UserID, Prime";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&prime);
   sqlStatement->BindSelectedColumn(&remainder);
   sqlStatement->BindSelectedColumn(&quotient);
   sqlStatement->BindSelectedColumn(&dateReported);
   sqlStatement->BindSelectedColumn(&showOnWebPage);

   ip_Socket->StartBuffering();

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>");
      ip_Socket->Send("%s and Near %s Found by User</tr>", ic_SearchType, ic_SearchType);
      ip_Socket->Send("<tr align=center><td class=headertext>No %s or Near %s found</tr>", ic_SearchType, ic_SearchType);
      ip_Socket->Send("</table>");
   }
   else
   {
      *prevUserID = 0;
      hiddenCount = foundCount = nearCount = 0;
      ip_Socket->Send("<table frame=box align=center border=1>");
      do
      {
         if (strcmp(prevUserID, userID))
         {
            if (*prevUserID)
            {
               if (hiddenCount)
                  ip_Socket->Send("<tr><td align=center colspan=7>User %s has found %d %s%s and %d Near %s%s, %d %s hidden</tr>",
                                  prevUserID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                                  nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"),
                                  hiddenCount, ((hiddenCount>1) ? "are" : "is"));
               else
                  ip_Socket->Send("<tr><td align=center colspan=7>User %s has found %d %s%s and %d Near %s%s</tr>",
                                  prevUserID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                                  nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"));
               ip_Socket->Send("<tr><td colspan=6>&nbsp;</tr>");
            }

            ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=7>%s and Near %s for User %s</tr>",
                            ic_SearchType, ic_SearchType, userID);
            ip_Socket->Send("<tr class=headercolor><th class=headertext>Value<th class=headertext>Team");
            ip_Socket->Send("<th class=headertext>Machine<th class=headertext>Instance<th class=headertext>Date Reported</tr>");

            strcpy(prevUserID, userID);
            hiddenCount = foundCount = nearCount = 0;
         }

         if (!remainder && !quotient)
            foundCount++;
         else
            nearCount++;

         if (showOnWebPage)
         {
            if (!remainder && !quotient)
               ip_Socket->Send("<tr><td align=center>%" PRId64"<td>%s<td>%s<td>%s<td>%s</tr>",
                               prime, teamID, machineID, instanceID, TimeToString(dateReported));
            else
            {
               if (ii_ServerType == ST_WALLSUNSUN)
                  ip_Socket->Send("<tr><td align=center>%" PRId64" (0 %+d p)<td>%s<td>%s<td>%s<td>%s</tr>",
                                  prime, quotient, teamID, machineID, instanceID, TimeToString(dateReported));
               else
                  ip_Socket->Send("<tr><td align=center>%" PRId64" (%+d %+d p)<td>%s<td>%s<td>%s<td>%s</tr>",
                                  prime, remainder, quotient, teamID, machineID, instanceID, TimeToString(dateReported));
            }
         }
         else
            hiddenCount++;
      } while (sqlStatement->FetchRow(false));

      if (hiddenCount)
         ip_Socket->Send("<tr><td align=center colspan=6>User %s has found %d %s%s and %d Near %s%s, %d %s hidden</tr>",
                         prevUserID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                         nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"),
                         hiddenCount, ((hiddenCount>1) ? "are" : "is"));
      else
         ip_Socket->Send("<tr><td align=center colspan=6>User %s has found %d %s%s and %d Near %s%s</tr>",
                         prevUserID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                         nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"));
      ip_Socket->Send("</table>");
   }

   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     WWWWHTMLGenerator::FindsByTeam(void)
{
   SQLStatement *sqlStatement;
   int64_t    dateReported, prime, remainder, quotient;
   int32_t    foundCount, nearCount, showOnWebPage, hiddenCount;
   char       userID[ID_LENGTH+1], prevTeamID[ID_LENGTH+1];
   char       machineID[ID_LENGTH+1], instanceID[ID_LENGTH+1], teamID[ID_LENGTH+1];

   const char *theSelect = "select UserID, TeamID, MachineID, InstanceID, Prime, " \
                           "       Remainder, Quotient, DateReported, ShowOnWebPage " \
                           "  from UserWWWWs " \
                           " where $null_func$(TeamID, ' ') <> ' ' " \
                           "order by TeamID, Prime";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(teamID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&prime);
   sqlStatement->BindSelectedColumn(&remainder);
   sqlStatement->BindSelectedColumn(&quotient);
   sqlStatement->BindSelectedColumn(&dateReported);
   sqlStatement->BindSelectedColumn(&showOnWebPage);

   ip_Socket->StartBuffering();

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>");
      ip_Socket->Send("%s and Near %s Found by Team</tr>", ic_SearchType, ic_SearchType);
      ip_Socket->Send("<tr align=center><td class=headertext>No %s or Near %s found</tr>", ic_SearchType, ic_SearchType);
      ip_Socket->Send("</table>");
   }
   else
   {
      *prevTeamID = 0;
      hiddenCount = foundCount = nearCount = 0;
      ip_Socket->Send("<table frame=box align=center border=1>");
      do
      {
         if (strcmp(prevTeamID, teamID))
         {
            if (*prevTeamID)
            {
               if (hiddenCount)
                  ip_Socket->Send("<tr><td align=center colspan=7>Team %s has found %d %s%s and %d Near %s%s, %d %s hidden</tr>",
                                  prevTeamID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                                  nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"),
                                  hiddenCount, ((hiddenCount>1) ? "are" : "is"));
               else
                  ip_Socket->Send("<tr><td align=center colspan=7>Team %s has found %d %s%s and %d Near %s%s</tr>",
                                  prevTeamID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                                  nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"));
               ip_Socket->Send("<tr><td colspan=6>&nbsp;</tr>");
            }

            ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=7>%s and Near %s for Team %s</tr>",
                            ic_SearchType, ic_SearchType, teamID);
            ip_Socket->Send("<tr class=headercolor><th class=headertext>Value<th class=headertext>User");
            ip_Socket->Send("<th class=headertext>Machine<th class=headertext>Instance<th class=headertext>Date Reported</tr>");

            strcpy(prevTeamID, teamID);
            hiddenCount = foundCount = nearCount = 0;
         }

         if (!remainder && !quotient)
            foundCount++;
         else
            nearCount++;

         if (showOnWebPage)
         {
            if (!remainder && !quotient)
               ip_Socket->Send("<tr><td align=center>%" PRId64"<td>%s<td>%s<td>%s<td>%s</tr>",
                               prime, userID, machineID, instanceID, TimeToString(dateReported));
            else
            {
               if (ii_ServerType == ST_WALLSUNSUN)
                  ip_Socket->Send("<tr><td align=center>%" PRId64" (0 %+d p)<td>%s<td>%s<td>%s<td>%s</tr>",
                                  prime, quotient, userID, machineID, instanceID, TimeToString(dateReported));
               else
                  ip_Socket->Send("<tr><td align=center>%" PRId64" (%+d %+d p)<td>%s<td>%s<td>%s<td>%s</tr>",
                                  prime, remainder, quotient, userID, machineID, instanceID, TimeToString(dateReported));
            }
         }
         else
            hiddenCount++;
      } while (sqlStatement->FetchRow(false));

      if (hiddenCount)
         ip_Socket->Send("<tr><td align=center colspan=6>Team %s has found %d %s%s and %d Near %s%s, %d %s hidden</tr>",
                         prevTeamID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                         nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"),
                         hiddenCount, ((hiddenCount>1) ? "are" : "is"));
      else
         ip_Socket->Send("<tr><td align=center colspan=6>Team %s has found %d %s%s and %d Near %s%s</tr>",
                         prevTeamID, foundCount, ic_SearchType, ((foundCount==1) ? "" : "s"),
                         nearCount, ic_SearchType, ((nearCount==1) ? "" : "s"));
      ip_Socket->Send("</table>");
   }

   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void  WWWWHTMLGenerator::ServerStats(void)
{
   SQLStatement *sqlStatement;
   int64_t     minInGroup, maxInGroup, completedThru, leadingEdge;
   int32_t     countInGroup, countInProgress, countedTested, countDoubleChecked;
   int32_t     countUntested, findCount, nearCount;
   string      minInGroupStr, maxInGroupStr;
   string      completedThruStr, leadingEdgeStr;

   const char *theSelect = "select CountInGroup, MinInGroup, MaxInGroup, " \
                           "       CountTested, CountDoubleChecked, CountUntested, " \
                           "       CountInProgress, CompletedThru, LeadingEdge, " \
                           "       WWWWPrimes, NearWWWWPrimes " \
                           "  from WWWWGroupStats ";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&countInGroup);
   sqlStatement->BindSelectedColumn(&minInGroup);
   sqlStatement->BindSelectedColumn(&maxInGroup);
   sqlStatement->BindSelectedColumn(&countedTested);
   sqlStatement->BindSelectedColumn(&countDoubleChecked);
   sqlStatement->BindSelectedColumn(&countUntested);
   sqlStatement->BindSelectedColumn(&countInProgress);
   sqlStatement->BindSelectedColumn(&completedThru);
   sqlStatement->BindSelectedColumn(&leadingEdge);
   sqlStatement->BindSelectedColumn(&findCount);
   sqlStatement->BindSelectedColumn(&nearCount);

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>Server Stats</tr>");
      ip_Socket->Send("<tr align=center><td class=headertext>No group stats found</tr>");
      ip_Socket->Send("</table>");
      delete sqlStatement;
      return;
   }

   ip_Socket->Send("<table frame=box align=center border=1>");
	ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=%d>Server Stats</tr>",
                    (ib_NeedsDoubleCheck ? 11 : 10));
   ip_Socket->Send("<tr class=headercolor><th class=headertext>Total Ranges<th class=headertext>Min Range Start");
   ip_Socket->Send("<th class=headertext>Max Range Start<th class=headertext>Count Tested");
   if (ib_NeedsDoubleCheck) ip_Socket->Send("<th class=headertext>Count DC\'d");
   ip_Socket->Send("<th class=headertext>Count Untested<th class=headertext>In Progress");
   ip_Socket->Send("<th class=headertext>Completed Thru<th class=headertext>Leading Edge");
   ip_Socket->Send("<th class=headertext>%ss<th class=headertext>Near %ss", ic_SearchType, ic_SearchType);
   ip_Socket->Send("</tr>");

   do
   {
      // If the socket was closed, then stop sending data
      if (!ip_Socket->Send("<tr bgcolor=\"%s\">", (countUntested ? "white" : "aqua")))
         break;

      ConvertToScientificNotation(minInGroup, minInGroupStr);
      ConvertToScientificNotation(maxInGroup, maxInGroupStr);
      ConvertToScientificNotation(completedThru, completedThruStr);
      ConvertToScientificNotation(leadingEdge, leadingEdgeStr);

      TD_32BIT(countInGroup);
      ip_Socket->Send("<td align=right>%s", minInGroupStr.c_str());
      ip_Socket->Send("<td align=right>%s", maxInGroupStr.c_str());
      TD_32BIT(countedTested);
      TD_IF_DC(countDoubleChecked);
      TD_32BIT(countUntested);
      TD_32BIT(countInProgress);
      ip_Socket->Send("<td align=right>%s", completedThruStr.c_str());
      ip_Socket->Send("<td align=right>%s", leadingEdgeStr.c_str());
      TD_32BIT(findCount);
      TD_32BIT(nearCount);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</table>");

   delete sqlStatement;
}

void  WWWWHTMLGenerator::UserStats(void)
{
   SQLStatement *sqlStatement;
   char        userID[ID_LENGTH+1];
   int32_t     testsPerformed, findCount, nearCount;
   double      totalScore;

   const char *theSelect = "select UserID, TestsPerformed, WWWWPrimes, " \
                           "       NearWWWWPrimes, TotalScore " \
                           "  from UserStats " \
                           "order by TotalScore desc";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, sizeof(userID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&findCount);
   sqlStatement->BindSelectedColumn(&nearCount);
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
      ip_Socket->Send("<table frame=box align=center class=sortable>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext class=sorttable_numeric>User");
      ip_Socket->Send("<th class=headertext class=sorttable_numeric>Total Score<th class=headertext class=sorttable_numeric>RangesTested");
      ip_Socket->Send("<th class=headertext class=sorttable_numeric>%ss<th class=headertext class=sorttable_numeric>Near %ss</tr>", ic_SearchType, ic_SearchType);

      do
      {
         ip_Socket->Send("<tr><td>%s<td align=right>%.0lf<td align=right>%d",
                         userID, totalScore, testsPerformed);
         ip_Socket->Send("<td align=right>%d<td align=right>%d</tr>",
                         findCount, nearCount);
      } while (sqlStatement->FetchRow(false));

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

   delete sqlStatement;
}

void  WWWWHTMLGenerator::UserTeamStats(void)
{
   SQLStatement *sqlStatement;
   char        userID[ID_LENGTH+1], teamID[ID_LENGTH+1];
   int32_t     testsPerformed, findCount, nearCount;
   double      totalScore;

   const char *theSelect = "select UserID, TeamID, TestsPerformed, WWWWPrimes, " \
                           "       NearWWWWPrimes, TotalScore " \
                           "  from UserTeamStats " \
                           "order by UserID, TotalScore, TeamID";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, sizeof(userID));
   sqlStatement->BindSelectedColumn(teamID, sizeof(teamID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&findCount);
   sqlStatement->BindSelectedColumn(&nearCount);
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
      ip_Socket->Send("<table frame=box align=center border=1>");
	   ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=6>User Team Stats</tr>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext>User");
      ip_Socket->Send("<th class=headertext>Team<th class=headertext>Total Score<th class=headertext>Ranges Tested");
      ip_Socket->Send("<th class=headertext>%ss<th class=headertext>Near %ss</tr>", ic_SearchType, ic_SearchType);

      do
      {
         ip_Socket->Send("<tr><td>%s<td>%s<td align=right>%.0lf<td align=right>%d",
                         userID, teamID, totalScore, testsPerformed);
         ip_Socket->Send("<td align=right>%d<td align=right>%d</tr>",
                         findCount, nearCount);
      } while (sqlStatement->FetchRow(false));

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

   delete sqlStatement;
}

void  WWWWHTMLGenerator::TeamUserStats(void)
{
   SQLStatement *sqlStatement;
   char        userID[ID_LENGTH+1], teamID[ID_LENGTH+1];
   int32_t     testsPerformed, findCount, nearCount;
   double      totalScore;

   const char *theSelect = "select UserID, TeamID, TestsPerformed, WWWWPrimes, " \
                           "       NearWWWWPrimes, TotalScore " \
                           "  from UserTeamStats " \
                           "order by TeamID, TotalScore, UserID";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(userID, sizeof(userID));
   sqlStatement->BindSelectedColumn(teamID, sizeof(teamID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&findCount);
   sqlStatement->BindSelectedColumn(&nearCount);
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
      ip_Socket->Send("<table frame=box align=center border=1>");
		ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=6>User Stats by Team</tr>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext>Team<th class=headertext>User");
      ip_Socket->Send("<th class=headertext>Total Score<th class=headertext>Ranges Tested");
      ip_Socket->Send("<th class=headertext>%ss<th class=headertext>Near %ss</tr>", ic_SearchType, ic_SearchType);

      do
      {
         ip_Socket->Send("<tr><td>%s<td>%s<td align=right>%.0lf<td align=right>%d",
                         teamID, userID, totalScore, testsPerformed);
         ip_Socket->Send("<td align=right>%d<td align=right>%d</tr>",
                         findCount, nearCount);
      } while (sqlStatement->FetchRow(false));

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

   delete sqlStatement;
}

void  WWWWHTMLGenerator::TeamStats(void)
{
   SQLStatement *sqlStatement;
   char        teamID[ID_LENGTH+1];
   int32_t     testsPerformed, findCount, nearCount;
   double      totalScore;

   const char *theSelect = "select TeamID, TestsPerformed, WWWWPrimes, " \
                           "       NearWWWWPrimes, TotalScore " \
                           "  from TeamStats " \
                           "order by TotalScore desc";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(teamID, sizeof(teamID));
   sqlStatement->BindSelectedColumn(&testsPerformed);
   sqlStatement->BindSelectedColumn(&findCount);
   sqlStatement->BindSelectedColumn(&nearCount);
   sqlStatement->BindSelectedColumn(&totalScore);

   if (!sqlStatement->FetchRow(false))
   {
	  ip_Socket->Send("<table frame=box align=center border=1>");
	  ip_Socket->Send("<tr class=headercolor><th class=headertext align=center>Team Stats</tr>");
	  ip_Socket->Send("<tr align=center><td class=headertext>No team stats found</tr>");
	  ip_Socket->Send("</table>");
   }
   else
   {
      ip_Socket->StartBuffering();
      ip_Socket->Send("<table frame=box align=center border=1>");
	   ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=6>Team Stats</tr>");
      ip_Socket->Send("</table><p>");
      ip_Socket->Send("<table frame=box align=center class=sortable>");
      ip_Socket->Send("<tr class=headercolor><th class=headertext class=sorttable_numeric>Team");
      ip_Socket->Send("<th class=headertext class=sorttable_numeric>Total Score<th class=headertext class=sorttable_numeric>Tests Performed");
      ip_Socket->Send("<th class=headertext class=sorttable_numeric>%ss<th class=headertext class=sorttable_numeric>Near %ss</tr>", ic_SearchType, ic_SearchType);

      do
      {
         ip_Socket->Send("<tr><td>%s<td align=right>%.0lf<td align=right>%d",
                         teamID, totalScore, testsPerformed);
         ip_Socket->Send("<td align=right>%d<td align=right>%d</tr>",
                         findCount, nearCount);
      } while (sqlStatement->FetchRow(false));

      ip_Socket->Send("</table>");
      ip_Socket->SendBuffer();
   }

   delete sqlStatement;
}

void  WWWWHTMLGenerator::ConvertToScientificNotation(int64_t valueInt, string &valueStr)
{
   int32_t  eValue = 0, maxE;
   char     tempValue[50];

   if (ii_ServerType == ST_WIEFERICH)    maxE = 11;
   if (ii_ServerType == ST_WILSON)       maxE = 6;
   if (ii_ServerType == ST_WALLSUNSUN)   maxE = 10;
   if (ii_ServerType == ST_WOLSTENHOLME) maxE = 7;

   if (valueInt == 0)
   {
      valueStr = "0";
      return;
   }

   eValue = 0;
   while (valueInt % 10 == 0 && eValue < maxE)
   {
      eValue++;
      valueInt /= 10;
   }

   sprintf(tempValue, "%" PRId64"e%d", valueInt, eValue);
   valueStr = tempValue;
}
char  *WWWWHTMLGenerator::TimeToString(time_t inTime)
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

void     WWWWHTMLGenerator::HeaderPlusLinks(string pageTitle)
{
   ip_Socket->StartBuffering();

   ip_Socket->Send("<html><head><title>PRPNet %s %s - %s</title>", PRPNET_VERSION, pageTitle.c_str(), is_HTMLTitle.c_str());

   if (is_SortLink.length() > 0)
		ip_Socket->Send("<script src=\"%s\"></script>", is_SortLink.c_str());

   ip_Socket->Send("<link rel=\"icon\" type=\"image/gif\" href=\"data:image/gif;base64,R0lGODlhEAAQAIABAAAAAP///yH5BAEAAAEALAAAAAAQABAAQAIijI9pwBDtoJq0Wuue1rmjuFziSB7S2YRc6G1L5qoqWNZIAQA7\">");

   if (is_CSSLink.length() > 0)
      ip_Socket->Send("<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\" /></head><body>", is_CSSLink.c_str());

   ip_Socket->Send("<p align=center><b><font size=6>%s</font></b></p>", is_ProjectTitle.c_str());

   if (is_ProjectTitle != is_HTMLTitle)
      ip_Socket->Send("<p align=center><b><font size=4>%s</font></b></p>", is_HTMLTitle.c_str());

   ip_Socket->Send("<table frame=box align=center border=1><b><font size=4><tr>");

   ip_Socket->Send("<td><a href=server_stats.html>Server Statistics</a></td>");
   ip_Socket->Send("<td><a href=pending_work.html>Pending Work</a></td>");
   ip_Socket->Send("<td><a href=user_stats.html>User Statistics</a></td>");
   ip_Socket->Send("<td><a href=user_finds.html>User Finds</a></td>");
   ip_Socket->Send("<tr><td><a href=team_stats.html>Team Statistics</a></td>");
   ip_Socket->Send("<td><a href=team_finds.html>Team Finds</a></td>");
   ip_Socket->Send("<td><a href=userteam_stats.html>User/Team Statistics</a></td>");
   ip_Socket->Send("<td><a href=teamuser_stats.html>Team/User Statistics</a></td>");
   ip_Socket->Send("</tr></font></b></table><br><br>");

   ip_Socket->SendBuffer();
}

void     WWWWHTMLGenerator::GetDaysLeft(void)
{
   WWWWServerHelper *serverHelper = new WWWWServerHelper(ip_DBInterface, ip_Log);
  
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
