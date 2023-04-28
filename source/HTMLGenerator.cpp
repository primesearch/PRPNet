#include "HTMLGenerator.h"
#include "SQLStatement.h"

HTMLGenerator::HTMLGenerator(globals_t *theGlobals)
{
   ii_ServerType = theGlobals->i_ServerType;
   is_HTMLTitle = theGlobals->s_HTMLTitle;
   is_ProjectTitle = theGlobals->s_ProjectTitle;
   is_CSSLink = theGlobals->s_CSSLink;
   is_SortLink = theGlobals->s_SortLink;
   ib_ServerStatsSummaryOnly = theGlobals->b_ServerStatsSummaryOnly;
   is_AllPrimesSortSequence = theGlobals->s_AllPrimesSortSequence;

   ib_ShowTeamsOnHtml = theGlobals->b_ShowTeamsOnHtml;
   ib_NeedsDoubleCheck = theGlobals->b_NeedsDoubleCheck;
   ib_UseLocalTime = theGlobals->b_LocalTimeHTML;
   ii_DelayCount = theGlobals->i_DelayCount;
   ip_Delay = theGlobals->p_Delay;
}

void     HTMLGenerator::Send(string thePage)
{
   ip_Socket->Send("HTTP/1.1 200 OK");
   ip_Socket->Send("Connection: close");

   // The client needs this extra carriage return before sending HTML
   ip_Socket->Send("\n");

   if (thePage == "user_stats.html")
   {
      HeaderPlusLinks("User Statistics");
      UserStats();
   }
   else if (thePage == "team_stats.html" && HasTeams())
   {
      HeaderPlusLinks("Team Statistics");
      TeamStats();
   }
   else if (thePage == "userteam_stats.html" && HasTeams())
   {
      HeaderPlusLinks("User/Team Statistics");
      UserTeamStats();
   }
   else if (thePage == "teamuser_stats.html" && HasTeams())
   {
      HeaderPlusLinks("Team/User Statistics");
      TeamUserStats();
   }
   else if (thePage == "server_stats.html" || thePage.empty())
   {
      HeaderPlusLinks("Server Statistics");
      GetDaysLeft();
      ServerStats();
   }
   else if (!SendSpecificPage(thePage))
   {
      HeaderPlusLinks("");
      ip_Socket->Send("<div style=\"margin-top: 3em; color: red; font-size: large; text-align: center;\"><p><strong>Page %s does not exist on this server.</strong></p>", thePage.c_str());
      ip_Socket->Send("<p><strong>Better luck next time!</strong></p></div>");
   }

   ip_Socket->Send("</body></html>");
   // The client needs this extra carriage return before closing the socket
   ip_Socket->Send("\n");
   return;
}

void     HTMLGenerator::HeaderPlusLinks(string pageTitle)
{
   ip_Socket->StartBuffering();

   SendHeader(pageTitle);
   SendLinks();
   ip_Socket->Send("</header>");

   ip_Socket->SendBuffer();
}

void     HTMLGenerator::SendHeader(string pageTitle)
{
   ip_Socket->Send("<!DOCTYPE html>");
   ip_Socket->Send("<html lang=\"en\"><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">");
   ip_Socket->Send("<title>PRPNet %s %s - %s</title>", PRPNET_VERSION, pageTitle.c_str(), is_HTMLTitle.c_str());

   if (is_SortLink.length() > 0)
      ip_Socket->Send("<script src=\"%s\"></script>", is_SortLink.c_str());

   ip_Socket->Send("<link rel=\"icon\" type=\"image/gif\" href=\"data:image/gif;base64,R0lGODlhEAAQAIABAAAAAP///yH5BAEAAAEALAAAAAAQABAAQAIijI9pwBDtoJq0Wuue1rmjuFziSB7S2YRc6G1L5qoqWNZIAQA7\">");

   if (is_CSSLink.length() > 0)
      ip_Socket->Send("<link rel=\"stylesheet\" type=\"text/css\" href=\"%s\"></head><body><header>", is_CSSLink.c_str());

   ip_Socket->Send("<h1>%s</h1>", is_ProjectTitle.c_str());

   if (is_ProjectTitle != is_HTMLTitle)
      ip_Socket->Send("<h2>%s - %s</h2>", is_HTMLTitle.c_str(), pageTitle.c_str());
}

string   HTMLGenerator::TimeToString(time_t inTime)
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

bool     HTMLGenerator::HasTeams(void)
{
   static time_t  lastCheck = 0;
   static bool    hasTeams;
   SQLStatement  *sqlStatement;
   int32_t        theCount = 0;
   const char    *selectSQL = "select count(*) from TeamStats";

   if (!ib_ShowTeamsOnHtml)
      return false;

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

void     HTMLGenerator::GetDaysLeft(void)
{
   ServerHelper *serverHelper = GetServerHelper();
   int64_t hoursLeft = serverHelper->ComputeHoursRemaining();
   int64_t daysLeft = hoursLeft / 24;

   if (daysLeft < 10)
      ip_Socket->Send("<p id=\"work-remaining-notice\" class=\"warning\">");
   else
      ip_Socket->Send("<p id=\"work-remaining-notice\">");

   if (hoursLeft < 0)
      ip_Socket->Send("The server has no work left");
   else if (hoursLeft < 72)
      ip_Socket->Send("Estimate of %" PRId64" hour%s before the server runs out of work", hoursLeft, PLURAL_ENDING(hoursLeft));
   else
      ip_Socket->Send("Estimate of %" PRId64" day%s before the server runs out of work", daysLeft, PLURAL_ENDING(daysLeft));

   ip_Socket->Send("</p>");

   delete serverHelper;
}

bool     HTMLGenerator::CheckIfRecordsWereFound(SQLStatement *sqlStatement, string noRecordsFoundMessage)
{
   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<div class=\"no-records-found\">");
      ip_Socket->Send("<span><span>%s</span></span>", noRecordsFoundMessage.c_str());
      ip_Socket->Send("</div></article>");
      delete sqlStatement;
      return false;
   }
   else
      return true;
}
