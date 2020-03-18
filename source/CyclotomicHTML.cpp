#include "CyclotomicHTML.h"
#include "SQLStatement.h"

void CyclotomicHTML::ServerStats(void)
{
   SQLStatement *sqlStatement;
   int64_t     k;
   int32_t     n;
   int32_t     countInGroup, countInProgress;
   int32_t     countedTested, countDoubleChecked, countUntested;
   int32_t     prpsAndPrimesFound;
   int32_t     summaryGroups = 0, summaryCountInGroup = 0, summaryCountInProgress = 0;
   int32_t     summaryTested = 0, summaryDoubleChecked = 0;
   int32_t     summaryUntested = 0, summaryPRPsAndPrimesFound = 0;
   int64_t     minInGroup, maxInGroup, completedThru, leadingEdge;
   int64_t     summaryMinInGroup = 999999999999999999LL, summaryMaxInGroup = 0;
   int64_t     summaryCompletedThru = 999999999999999999LL, summaryLeadingEdge = 999999999999999999LL;

   const char *theSelect = "select k, n, CountInGroup, MinInGroup, MaxInGroup, " \
                           "       CountTested, CountDoubleChecked, CountUntested, " \
                           "       CountInProgress, CompletedThru, LeadingEdge, " \
                           "       PRPandPrimesFound " \
                           "  from CandidateGroupStats " \
                           "order by k, n";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&k);
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

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr align=center><td class=headertext>No group stats found</tr>");
      ip_Socket->Send("</table>");
      delete sqlStatement;
      return;
   }

   ServerStatsHeader(BY_B);

   do
   {
      summaryGroups++;
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
      
      if (ib_ServerStatsSummaryOnly)
         continue;

      // If the socket was closed, then stop sending data
      if (!ip_Socket->Send("<tr bgcolor=\"%s\">", (countUntested ? "white" : "aqua")))
         break;
      
      if (n == 1)
         ip_Socket->Send("<td align=center>Phi(%" PRId64", b)", k);
      else
         ip_Socket->Send("<td align=center>Phi(%" PRId64", b^%d)", k, n);
      TD_32BIT(countInGroup);
      TD_64BIT(minInGroup);
      TD_64BIT(maxInGroup);
      TD_32BIT(countedTested);
      TD_IF_DC(countDoubleChecked);
      TD_32BIT(countUntested);
      TD_32BIT(countInProgress);
      TD_64BIT(completedThru);
      TD_64BIT(leadingEdge);
      TD_32BIT(prpsAndPrimesFound);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("<tr class=totalcolor>");
   
   ip_Socket->Send("<th class=totaltext align=center>Groups: %d", summaryGroups);
   TH_32BIT(summaryCountInGroup);
   TH_64BIT(summaryMinInGroup);
   TH_64BIT(summaryMaxInGroup);
   TH_32BIT(summaryTested);
   TH_IF_DC(summaryDoubleChecked);
   TH_32BIT(summaryUntested);
   TH_32BIT(summaryCountInProgress);
   TH_64BIT(summaryCompletedThru);
   TH_64BIT(summaryLeadingEdge);
   TH_32BIT(summaryPRPsAndPrimesFound);

   ip_Socket->Send("</tr></table>");

   delete sqlStatement;
}
