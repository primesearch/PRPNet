#include "SophieGermainHTML.h"
#include "SQLStatement.h"

void SophieGermainHTML::ServerStats(void)
{
   SQLStatement *sqlStatement;
   int32_t     b, n, c;
   int32_t     countInGroup, countInProgress;
   int32_t     countedTested, countDoubleChecked, countUntested;
   int32_t     prpsAndPrimesFound;
   int32_t     summaryGroups = 0, summaryCountInGroup = 0, summaryCountInProgress = 0;
   int32_t     summaryTested = 0, summaryDoubleChecked = 0;
   int32_t     summaryUntested = 0, summaryPRPsAndPrimesFound = 0;
   int64_t     minInGroup, maxInGroup, completedThru, leadingEdge;
   int64_t     summaryMinInGroup = 999999999999999999LL, summaryMaxInGroup = 0;
   int64_t     summaryCompletedThru = 999999999999999999LL, summaryLeadingEdge = 999999999999999999LL;

   const char *theSelect = "select b, n, c, CountInGroup, MinInGroup, MaxInGroup, " \
                           "       CountTested, CountDoubleChecked, CountUntested, " \
                           "       CountInProgress, CompletedThru, LeadingEdge, " \
                           "       PRPandPrimesFound " \
                           "  from CandidateGroupStats " \
                           "order by b, n, c";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&b);
   sqlStatement->BindSelectedColumn(&n);
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

   if (!CheckIfRecordsWereFound(sqlStatement, "No group stats found"))
      return;

   ServerStatsHeader(BY_N);

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
      if (!ip_Socket->Send("<tr class=\"%s\">", (countUntested ? "untested" : "tested")))
         break;

      ip_Socket->Send("<th scope=\"row\"><var>k</var>*%d^%d%+d</th>", b, n, c);
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

   ip_Socket->Send("</tbody><tfoot><tr>");

   ip_Socket->Send("<th scope=\"row\">Groups: %d</th>", summaryGroups);
   TD_32BIT(summaryCountInGroup);
   TD_64BIT(summaryMinInGroup);
   TD_64BIT(summaryMaxInGroup);
   TD_32BIT(summaryTested);
   TD_IF_DC(summaryDoubleChecked);
   TD_32BIT(summaryUntested);
   TD_32BIT(summaryCountInProgress);
   TD_64BIT(summaryCompletedThru);
   TD_64BIT(summaryLeadingEdge);
   TD_32BIT(summaryPRPsAndPrimesFound);

   ip_Socket->Send("</tr></tfoot></table></article>");

   delete sqlStatement;
}
