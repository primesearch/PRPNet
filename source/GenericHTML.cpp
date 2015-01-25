#include "GenericHTML.h"
#include "SQLStatement.h"

void  GenericHTML::ServerStats(void)
{
   SQLStatement *sqlStatement;
   int32_t     c;
   int32_t     countInGroup, countInProgress;
   int32_t     countedTested, countDoubleChecked, countUntested;
   int32_t     prpsAndPrimesFound;

   const char *theSelect = "select c, CountInGroup, " \
                           "       CountTested, CountDoubleChecked, CountUntested, " \
                           "       CountInProgress, PRPandPrimesFound " \
                           "  from CandidateGroupStats " \
                           "order by c";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&c);
   sqlStatement->BindSelectedColumn(&countInGroup);
   sqlStatement->BindSelectedColumn(&countedTested);
   sqlStatement->BindSelectedColumn(&countDoubleChecked);
   sqlStatement->BindSelectedColumn(&countUntested);
   sqlStatement->BindSelectedColumn(&countInProgress);
   sqlStatement->BindSelectedColumn(&prpsAndPrimesFound);

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr align=center><td class=headertext>No group stats found</tr>");
      ip_Socket->Send("</table>");
      delete sqlStatement;
      return;
   }

   ServerStatsHeader(BY_N);

   do
   {
      // If the socket was closed, then stop sending data
      if (!ip_Socket->Send("<trbgcolor=\"%s\">", (countUntested ? "white" : "aqua")))
         break;

      ip_Socket->Send("<td align=center>n!%+d", c);
      TD_32BIT(countInGroup);
      TD_32BIT(countedTested);
      TD_IF_DC(countDoubleChecked);
      TD_32BIT(countUntested);
      TD_32BIT(countInProgress);
      TD_32BIT(prpsAndPrimesFound);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("<tr class=totalcolor>");

   delete sqlStatement;
}
