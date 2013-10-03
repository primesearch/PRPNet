#include "PrimorialHTML.h"
#include "SQLStatement.h"

void  PrimorialHTML::ServerStats(void)
{
   SQLStatement *sqlStatement;
   int32_t     c;
   int32_t     countInGroup, minInGroup, maxInGroup, countInProgress;
   int32_t     countedTested, countDoubleChecked, countUntested;
   int32_t     completedThru, leadingEdge, prpsAndPrimesFound;
   int32_t     summarySigns = 0, summaryCountInGroup = 0, summaryMinInGroup = 999999999;
   int32_t     summaryMaxInGroup = 0, summaryTested = 0, summaryDoubleChecked = 0;
   int32_t     summaryUntested = 0, summaryCompletedThru = 999999999, summaryLeadingEdge = 999999999;
   int32_t     summaryPRPsAndPrimesFound = 0, summaryCountInProgress = 0;

   const char *theSelect = "select c, CountInGroup, MinInGroup, MaxInGroup, " \
                           "       CountTested, CountDoubleChecked, CountUntested, " \
                           "       CountInProgress, CompletedThru, LeadingEdge, " \
                           "       PRPandPrimesFound " \
                           "  from CandidateGroupStats " \
                           "order by c";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&c);
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
      summarySigns++;
      summaryCountInGroup += countInGroup;
      summaryCountInProgress += countInProgress;
      summaryMinInGroup = (minInGroup < summaryMinInGroup ? minInGroup : summaryMinInGroup);
      summaryMaxInGroup = (maxInGroup > summaryMaxInGroup ? maxInGroup : summaryMaxInGroup);
      summaryTested += countedTested;
      summaryDoubleChecked += countDoubleChecked;
      summaryUntested += countUntested;
      summaryCompletedThru = (completedThru < summaryCompletedThru ? completedThru : summaryCompletedThru);
      summaryLeadingEdge = (leadingEdge < summaryLeadingEdge ? leadingEdge : summaryLeadingEdge);
      summaryPRPsAndPrimesFound += prpsAndPrimesFound;

      // If the socket was closed, then stop sending data
      if (!ip_Socket->Send("<trbgcolor=\"%s\">", (countUntested ? "white" : "aqua")))
         break;

      ip_Socket->Send("<td align=center>n#%+d", c);
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

   ip_Socket->Send("<tr class=totalcolor>");

   ip_Socket->Send("<th class=totaltext align=center>Signs: %d", summarySigns);
   TH_32BIT(summaryCountInGroup);
   TH_32BIT(summaryMinInGroup);
   TH_32BIT(summaryMaxInGroup);
   TH_32BIT(summaryTested);
   TH_IF_DC(summaryDoubleChecked);
   TH_32BIT(summaryUntested);
   TH_32BIT(summaryCountInProgress);
   TH_32BIT(summaryCompletedThru);
   TH_32BIT(summaryLeadingEdge);
   TH_32BIT(summaryPRPsAndPrimesFound);

   ip_Socket->Send("</tr></table>");

   delete sqlStatement;
}
