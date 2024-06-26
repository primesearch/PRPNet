#include "GFNHTML.h"
#include "SQLStatement.h"

void GFNHTML::ServerStats(void)
{
   SQLStatement *sqlStatement;
   int32_t     n;
   int32_t     countInGroup, minInGroup, maxInGroup, countInProgress;
   int32_t     countedTested, countDoubleChecked, countUntested;
   int32_t     completedThru, leadingEdge, prpsAndPrimesFound;

   const char *theSelect = "select n, CountInGroup, MinInGroup, MaxInGroup, " \
                           "       CountTested, CountDoubleChecked, CountUntested, " \
                           "       CountInProgress, CompletedThru, LeadingEdge, " \
                           "       PRPandPrimesFound " \
                           "  from CandidateGroupStats " \
                           "order by n";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&n);
   sqlStatement->BindSelectedColumn(&countInGroup);
   sqlStatement->BindSelectedColumn(&minInGroup);
   sqlStatement->BindSelectedColumn(&maxInGroup);
   sqlStatement->BindSelectedColumn(&countedTested);
   sqlStatement->BindSelectedColumn(&countDoubleChecked);
   sqlStatement->BindSelectedColumn(&countUntested);
   sqlStatement->BindSelectedColumn(&countInProgress);
   sqlStatement->BindSelectedColumn(&completedThru);
   sqlStatement->BindSelectedColumn(&leadingEdge);
   sqlStatement->BindSelectedColumn(&prpsAndPrimesFound);

   if (!CheckIfRecordsWereFound(sqlStatement, "No group stats found"))
      return;

   ServerStatsHeader(BY_B);

   do
   {
      // If the socket was closed, then stop sending data
      if (!ip_Socket->Send("<tr class=\"%s\">", (countUntested ? "untested" : "tested")))
         break;

      ip_Socket->Send("<th scope=\"row\"><var>b</var>^%d+1</th>", n);
      TD_32BIT(countInGroup);
      TD_32BIT(minInGroup);
      TD_32BIT(maxInGroup);
      TD_32BIT(countedTested);
      TD_IF_DC(countDoubleChecked);
      TD_32BIT(countUntested);
      TD_32BIT(countInProgress);
      TD_32BIT(completedThru);
      TD_32BIT(leadingEdge);
      TD_32BIT(prpsAndPrimesFound);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");

   delete sqlStatement;
}
