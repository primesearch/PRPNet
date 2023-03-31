#include "SierpinskiRieselHTML.h"
#include "SQLStatement.h"

void SierpinskiRieselHTML::ServerStats(void)
{
   SQLStatement *sqlStatement;
   int64_t     k;
   int32_t     b, c, prevB, prevC;
   int32_t     kCountInGroup, kMinN, kMaxN, kCountInProgress;
   int32_t     kCountedTested, kCountDoubleChecked, kCountUntested;
   int32_t     kCompletedThru, kLeadingEdge, kPRPsAndPrimesFound, sierpinskiRieselPrimeN;
   int32_t     conjectureKs = 0, conjectureNs = 0, conjectureMinN = 999999999;
   int32_t     conjectureMaxN = 0, conjectureTested = 0, conjectureDoubleChecked = 0;
   int32_t     conjectureUntested = 0, conjectureCompletedThru = 999999999, conjectureLeadingEdge = 999999999;
   int32_t     conjecturePRPsAndPrimesFound = 0, conjectureCountInProgress = 0, conjectureTestsSkipped = 0;
   int32_t     totalConjectures = 0, totalKs = 0, totalNs = 0, totalMinN = 999999999;
   int32_t     totalMaxN = 0, totalTested = 0, totalDoubleChecked = 0;
   int32_t     totalUntested = 0, totalCompletedThru = 999999999, totalLeadingEdge = 999999999;
   int32_t     totalPRPsAndPrimesFound = 0, totalCountInProgress = 0, totalTestsSkipped = 0;

   const char *theSelect = "select k, b, c, CountInGroup, MinInGroup, MaxInGroup, " \
                           "       CountTested, CountDoubleChecked, CountUntested, " \
                           "       CountInProgress, CompletedThru, LeadingEdge, PRPandPrimesFound, " \
                           "       SierpinskiRieselPrimeN " \
                           "  from CandidateGroupStats " \
                           "order by b, c, k";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&k);
   sqlStatement->BindSelectedColumn(&b);
   sqlStatement->BindSelectedColumn(&c);
   sqlStatement->BindSelectedColumn(&kCountInGroup);
   sqlStatement->BindSelectedColumn(&kMinN);
   sqlStatement->BindSelectedColumn(&kMaxN);
   sqlStatement->BindSelectedColumn(&kCountedTested);
   sqlStatement->BindSelectedColumn(&kCountDoubleChecked);
   sqlStatement->BindSelectedColumn(&kCountUntested);
   sqlStatement->BindSelectedColumn(&kCountInProgress);
   sqlStatement->BindSelectedColumn(&kCompletedThru);
   sqlStatement->BindSelectedColumn(&kLeadingEdge);
   sqlStatement->BindSelectedColumn(&kPRPsAndPrimesFound);
   sqlStatement->BindSelectedColumn(&sierpinskiRieselPrimeN);

   if (!CheckIfRecordsWereFound(sqlStatement, "No group stats found"))
      return;

   if (ib_ServerStatsSummaryOnly)
   {
      ip_Socket->Send("<table class=\"server-stats sortable\"><thead><tr>");

      TH_CLMN_HDR("Form");
      TH_CLMN_HDR("Total Candidates");
      TH_CLMN_HDR("Min <var>N</var>");
      TH_CLMN_HDR("Max <var>N</var>");
      TH_CLMN_HDR("Count Tested");
      TH_CH_IF_DC("Count DC\'d");
      TH_CLMN_HDR("Count Untested");
      TH_CLMN_HDR("In Progress");
      TH_CLMN_HDR("Tests Skipped");
      TH_CLMN_HDR("Completed Thru");
      TH_CLMN_HDR("Leading Edge");
      TH_CLMN_HDR("PRPs/Primes");

      ip_Socket->Send("</tr></thead><tbody>");
   }

   prevB = prevC = 0;
   do
   {
      if (b != prevB || c != prevC)
      {
         if (prevB)
         {
            totalConjectures++;

            if (ib_ServerStatsSummaryOnly)
                ip_Socket->Send("<tr><th scope=\"row\">%c%d <var>k</var>\'s: %d</th>",
                                (c > 0 ? 'S' : 'R'), prevB, conjectureKs);
            else
                ip_Socket->Send("</tbody><tfoot><tr class=\"%s\"><th scope=\"row\">%c%d <var>k</var>\'s: %d</th>",
                      (conjectureKs == conjecturePRPsAndPrimesFound ? "conjectureComplete" : "conjectureIncomplete"),
                      (c > 0 ? 'S' : 'R'), prevB, conjectureKs);

            TD_32BIT(conjectureNs);
            TD_32BIT(conjectureMinN);
            TD_32BIT(conjectureMaxN);
            TD_32BIT(conjectureTested);
            TD_IF_DC(conjectureDoubleChecked);
            TD_32BIT(conjectureUntested);
            TD_32BIT(conjectureCountInProgress);
            TD_32BIT(conjectureTestsSkipped);
            TD_32BIT(conjectureCompletedThru);
            TD_32BIT(conjectureLeadingEdge);
            TD_32BIT(conjecturePRPsAndPrimesFound);

            if (ib_ServerStatsSummaryOnly)
                ip_Socket->Send("<td style=\"text-align: center;\">%d remain</td></tr>", conjectureKs - conjecturePRPsAndPrimesFound);
            else
                ip_Socket->Send("<td style=\"text-align: center;\">%d remain</td></tr></tfoot></table>", conjectureKs - conjecturePRPsAndPrimesFound);
         }

         if (!ib_ServerStatsSummaryOnly)
         {
            ip_Socket->Send("<div class=\"header-box\">");
            ip_Socket->Send("<span><span>%s base %d</span></span>", (c > 0 ? "Sierpinski" : "Riesel"), b);
            ip_Socket->Send("</div>");
            ip_Socket->Send("<table class=\"server-stats sortable\"><thead><tr>");

            TH_CLMN_HDR("Form");
            TH_CLMN_HDR("Total Candidates");
            TH_CLMN_HDR("Min <var>N</var>");
            TH_CLMN_HDR("Max <var>N</var>");
            TH_CLMN_HDR("Count Tested");
            TH_CH_IF_DC("Count DC\'d");
            TH_CLMN_HDR("Count Untested");
            TH_CLMN_HDR("In Progress");
            TH_CLMN_HDR("Tests Skipped");
            TH_CLMN_HDR("Completed Thru");
            TH_CLMN_HDR("Leading Edge");
            TH_CLMN_HDR("PRPs/Primes");
            TH_CLMN_HDR("Smallest Prime");

            ip_Socket->Send("</tr></thead><tbody>");
         }

         prevB = b;
         prevC = c;

         totalKs += conjectureKs;
         totalPRPsAndPrimesFound += conjecturePRPsAndPrimesFound;

         totalNs += conjectureNs;
         totalCountInProgress += conjectureCountInProgress;
         totalMinN = (conjectureMinN < totalMinN ? conjectureMinN : totalMinN);
         totalMaxN = (conjectureMaxN > totalMaxN ? conjectureMaxN : totalMaxN);
         totalTested += conjectureTested;
         totalDoubleChecked += conjectureDoubleChecked;
         totalUntested += conjectureUntested;
         totalTestsSkipped += conjectureTestsSkipped;

         if (conjecturePRPsAndPrimesFound != conjectureKs && totalCompletedThru)
            totalCompletedThru = (conjectureCompletedThru < totalCompletedThru ? conjectureCompletedThru : totalCompletedThru);
         if (conjecturePRPsAndPrimesFound != conjectureKs)
            totalLeadingEdge = (conjectureLeadingEdge < totalLeadingEdge ? conjectureLeadingEdge : totalLeadingEdge);

         conjectureKs = 0; conjectureNs = 0; conjectureMinN = 999999999;
         conjectureMaxN = 0; conjectureTested = 0; conjectureDoubleChecked = 0;
         conjectureUntested = 0; conjectureCompletedThru = 999999999; conjectureLeadingEdge = 999999999;
         conjecturePRPsAndPrimesFound = 0, conjectureCountInProgress = 0;
         conjectureTestsSkipped = 0;
      }

      conjectureKs++;
      conjectureNs += kCountInGroup;
      conjectureCountInProgress += kCountInProgress;
      conjectureMinN = (kMinN < conjectureMinN ? kMinN : conjectureMinN);
      conjectureMaxN = (kMaxN > conjectureMaxN ? kMaxN : conjectureMaxN);
      conjectureTested += kCountedTested;
      conjectureDoubleChecked += kCountDoubleChecked;
      conjectureUntested += kCountUntested;

      if (!kPRPsAndPrimesFound && conjectureCompletedThru)
         conjectureCompletedThru = (kCompletedThru < conjectureCompletedThru ? kCompletedThru : conjectureCompletedThru);
      if (!kPRPsAndPrimesFound)
         conjectureLeadingEdge = (kLeadingEdge < conjectureLeadingEdge ? kLeadingEdge : conjectureLeadingEdge);

      if (!ib_ServerStatsSummaryOnly)
      {
         ip_Socket->Send("<tr class=\"%s\">", ((sierpinskiRieselPrimeN > 0) ? "found" : (kCountUntested ? "untested" : "tested")));

         if (k > 1)
            ip_Socket->Send("<th scope=\"row\">%" PRId64"*%d^<var>n</var>%+d</th>", k, b, c);
         else
            ip_Socket->Send("<th scope=\"row\">%d^<var>n</var>%+d</th>", b, c);

         TD_32BIT(kCountInGroup);
         TD_32BIT(kMinN);
         TD_32BIT(kMaxN);
         TD_32BIT(kCountedTested);
         TD_IF_DC(kCountDoubleChecked);
         TD_32BIT(kCountUntested);
         TD_32BIT(kCountInProgress);

         if (sierpinskiRieselPrimeN > 0)
         {
            TD_32BIT(kCountInGroup - kCountInProgress - kCountedTested);
            conjecturePRPsAndPrimesFound++;
            conjectureTestsSkipped += (kCountInGroup - kCountInProgress - kCountedTested);
         }
         else
            TD_32BIT(0);

         TD_32BIT(kCompletedThru);
         TD_32BIT(kLeadingEdge);
         TD_32BIT(kPRPsAndPrimesFound);

         if (sierpinskiRieselPrimeN > 0)
            ip_Socket->Send("<td style=\"text-align: center;\">%" PRIu64"*%d^%d%+d</td></tr>", k, b, sierpinskiRieselPrimeN, c);
         else
            ip_Socket->Send("<td style=\"text-align: center;\">none found</td></tr>");
      }
   } while (sqlStatement->FetchRow(false));

   if (ib_ServerStatsSummaryOnly)
      ip_Socket->Send("<tr><th scope=\"row\">%c%d <var>k</var>\'s: %d</th>",
                      (c > 0 ? 'S' : 'R'), prevB, conjectureKs);
   else
      ip_Socket->Send("</tbody><tfoot><tr class=\"%s\"><th scope=\"row\">%c%d <var>k</var>\'s: %d</th>",
                      (conjectureKs == conjecturePRPsAndPrimesFound ? "conjectureComplete" : "conjectureIncomplete"),
                      (c > 0 ? 'S' : 'R'), prevB, conjectureKs);

   TD_32BIT(conjectureNs);
   TD_32BIT(conjectureMinN);
   TD_32BIT(conjectureMaxN);
   TD_32BIT(conjectureTested);
   TD_IF_DC(conjectureDoubleChecked);
   TD_32BIT(conjectureUntested);
   TD_32BIT(conjectureCountInProgress);
   TD_32BIT(conjectureTestsSkipped);
   TD_32BIT(conjectureCompletedThru);
   TD_32BIT(conjectureLeadingEdge);
   TD_32BIT(conjecturePRPsAndPrimesFound);

   if (ib_ServerStatsSummaryOnly)
      ip_Socket->Send("<td style=\"text-align: center;\">%d remain</td></tr>", conjectureKs - conjecturePRPsAndPrimesFound);
   else
      ip_Socket->Send("<td style=\"text-align: center;\">%d remain</td></tr></tfoot></table>", conjectureKs - conjecturePRPsAndPrimesFound);

   totalConjectures++;
   totalKs += conjectureKs;
   totalPRPsAndPrimesFound += conjecturePRPsAndPrimesFound;

   totalNs += conjectureNs;
   totalCountInProgress += conjectureCountInProgress;
   totalMinN = (conjectureMinN < totalMinN ? conjectureMinN : totalMinN);
   totalMaxN = (conjectureMaxN > totalMaxN ? conjectureMaxN : totalMaxN);
   totalTested += conjectureTested;
   totalDoubleChecked += conjectureDoubleChecked;
   totalUntested += conjectureUntested;
   totalTestsSkipped += conjectureTestsSkipped;
   if (conjecturePRPsAndPrimesFound != conjectureKs && totalCompletedThru)
      totalCompletedThru = (conjectureCompletedThru < totalCompletedThru ? conjectureCompletedThru : totalCompletedThru);
   if (conjecturePRPsAndPrimesFound != conjectureKs)
      totalLeadingEdge = (conjectureLeadingEdge < totalLeadingEdge ? conjectureLeadingEdge : totalLeadingEdge);

   if (totalConjectures > 1)
   {
      if (!ib_ServerStatsSummaryOnly)
      {
         ip_Socket->Send("<table class=\"server-stats\">");
         ip_Socket->Send("<thead><tr><th colspan=\"%d\">Totals Across %d Conjectures</th></tr>",
                         (ib_NeedsDoubleCheck ? 12 : 11), totalConjectures);
         ip_Socket->Send("<tr>");

         TH_CLMN_HDR("Total <var>k</var>");
         TH_CLMN_HDR("Total Candidates");
         TH_CLMN_HDR("Min <var>N</var>");
         TH_CLMN_HDR("Max <var>N</var>");
         TH_CLMN_HDR("Count Tested");
         TH_CH_IF_DC("Count DC\'d");
         TH_CLMN_HDR("Count Untested");
         TH_CLMN_HDR("In Progress");
         TH_CLMN_HDR("Tests Skipped");
         TH_CLMN_HDR("Completed Thru");
         TH_CLMN_HDR("Leading Edge");
         TH_CLMN_HDR("PRPs/Primes");
         TH_CLMN_HDR("Remaining <var>k</var>");

         ip_Socket->Send("</tr></thead>");
         ip_Socket->Send("<tbody><tr>");
      }
      else
         ip_Socket->Send("</tbody><tfoot><tr>");

      TD_32BIT(totalKs);
      TD_32BIT(totalNs);
      TD_32BIT(totalMinN);
      TD_32BIT(totalMaxN);
      TD_32BIT(totalTested);
      TD_IF_DC(totalDoubleChecked);
      TD_32BIT(totalUntested);
      TD_32BIT(totalCountInProgress);
      TD_32BIT(totalTestsSkipped);
      TD_32BIT(totalCompletedThru);
      TD_32BIT(totalLeadingEdge);
      TD_32BIT(totalPRPsAndPrimesFound);
      if (!ib_ServerStatsSummaryOnly)
         ip_Socket->Send("<td style=\"text-align: right;\">%d</td></tr></tbody></table></article>", conjectureKs - conjecturePRPsAndPrimesFound);
      else
         ip_Socket->Send("<td style=\"text-align: center;\">%d remain</td></tr></tfoot></table></article>", conjectureKs - conjecturePRPsAndPrimesFound);

   } else if (ib_ServerStatsSummaryOnly)
      ip_Socket->Send("</tbody></table></article>");

   delete sqlStatement;
}
