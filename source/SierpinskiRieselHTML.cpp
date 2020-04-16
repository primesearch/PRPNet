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

   if (!sqlStatement->FetchRow(false))
   {
      ip_Socket->Send("<table frame=box align=center border=1>");
      ip_Socket->Send("<tr align=center><td class=headertext>No group stats found</tr>");
      ip_Socket->Send("</table>");
      delete sqlStatement;
      return;
   }

   if (ib_ServerStatsSummaryOnly)
   {
      ip_Socket->Send("<table frame=box align=center border=1><tr class=headercolor>");

      TH_CLMN_HDR("Form");
      TH_CLMN_HDR("Total Candidates");
      TH_CLMN_HDR("Min N");
      TH_CLMN_HDR("Max N");
      TH_CLMN_HDR("Count Tested");
      TH_CH_IF_DC("Count DC\'d");
      TH_CLMN_HDR("Count Untested");
      TH_CLMN_HDR("In Progress");
      TH_CLMN_HDR("Tests Skipped");
      TH_CLMN_HDR("Completed Thru");
      TH_CLMN_HDR("Leading Edge");
      TH_CLMN_HDR("PRPs/Primes");

      ip_Socket->Send("</tr>");
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
               ip_Socket->Send("<tr><th align=center>%c%d k\'s: %d",
                               (c > 0 ? 'S' : 'R'), prevB, conjectureKs);
            else
               ip_Socket->Send("<tr class=totalcolor bgcolor=\"%s\"><th class=totaltext align=center>%c%d k\'s: %d",
                               (conjectureKs == conjecturePRPsAndPrimesFound ? "red" : "yellow"),
                               (c > 0 ? 'S' : 'R'), prevB, conjectureKs);

            TH_32BIT(conjectureNs);
            TH_32BIT(conjectureMinN);
            TH_32BIT(conjectureMaxN);
            TH_32BIT(conjectureTested);
            TH_IF_DC(conjectureDoubleChecked);
            TH_32BIT(conjectureUntested);
            TH_32BIT(conjectureCountInProgress);
            TH_32BIT(conjectureTestsSkipped);
            TH_32BIT(conjectureCompletedThru);
            TH_32BIT(conjectureLeadingEdge);
            TH_32BIT(conjecturePRPsAndPrimesFound);

            ip_Socket->Send("<th class=totaltext align=center>%d remain</tr></table><p><p>", conjectureKs - conjecturePRPsAndPrimesFound);
         }

         if (!ib_ServerStatsSummaryOnly)
         {
            ip_Socket->Send("<table frame=box align=center border=1>");
	         ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=%d>%s base %d</tr>",
	                        (ib_NeedsDoubleCheck ? 12 : 11), (c > 0 ? "Sierpinski" : "Riesel"), b);
            ip_Socket->Send("</table><p>");
            ip_Socket->Send("<table frame=box align=center border=1 class=sortable>");
            ip_Socket->Send("<tr class=headercolor><th class=headertext>Form<th class=headertext>Total Candidates<th class=headertext>Min N<th class=headertext>Max N");
            ip_Socket->Send("<th class=headertext>Count Tested");
            if (ib_NeedsDoubleCheck) ip_Socket->Send("<th class=headertext>Count DC\'d");
            ip_Socket->Send("<th class=headertext>Count Untested<th class=headertext>In Progress<th class=headertext>Tests Skipped");
            ip_Socket->Send("<th class=headertext>Completed Thru<th class=headertext>Leading Edge<th class=headertext>PRPs/Primes<th class=headertext>Smallest Prime");
            ip_Socket->Send("</tr>");
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
         ip_Socket->Send("<tr bgcolor=\"%s\">", ((sierpinskiRieselPrimeN > 0) ? "lime" : (kCountUntested ? "white" : "aqua")));

         if (k > 1)
            ip_Socket->Send("<td align=center>%" PRId64"*%d^n%+d", k, b, c);
         else
            ip_Socket->Send("<td align=center>%d^n%+d", b, c);

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
            ip_Socket->Send("<td align=center>%" PRIu64"*%d^%d%+d</tr>", k, b, sierpinskiRieselPrimeN, c);
         else
            ip_Socket->Send("<td align=center>none found</tr>");
      }
   } while (sqlStatement->FetchRow(false));

   if (ib_ServerStatsSummaryOnly)
      ip_Socket->Send("<tr><th align=center>%c%d k\'s: %d",
                      (c > 0 ? 'S' : 'R'), prevB, conjectureKs);
   else
      ip_Socket->Send("<tr class=totalcolor bgcolor=\"%s\"><th class=totaltext align=center>%c%d k\'s: %d",
                      (conjectureKs == conjecturePRPsAndPrimesFound ? "red" : "yellow"),
                      (c > 0 ? 'S' : 'R'), prevB, conjectureKs);

   TH_32BIT(conjectureNs);
   TH_32BIT(conjectureMinN);
   TH_32BIT(conjectureMaxN);
   TH_32BIT(conjectureTested);
   TH_IF_DC(conjectureDoubleChecked);
   TH_32BIT(conjectureUntested);
   TH_32BIT(conjectureCountInProgress);
   TH_32BIT(conjectureTestsSkipped);
   TH_32BIT(conjectureCompletedThru);
   TH_32BIT(conjectureLeadingEdge);
   TH_32BIT(conjecturePRPsAndPrimesFound);

   ip_Socket->Send("<th class=totaltext align=center>%d remain</tr></table><p><p>", conjectureKs - conjecturePRPsAndPrimesFound);

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
         ip_Socket->Send("<table frame=box align=center border=1>");
	      ip_Socket->Send("<tr class=headercolor><th class=headertext align=center colspan=%d>Totals Across %d Conjectures</tr>",
	                     (ib_NeedsDoubleCheck ? 12 : 11), totalConjectures);
         ip_Socket->Send("<tr class=headercolor><th class=headertext>Total k<th class=headertext>Total Candidates<th class=headertext>Min N<th class=headertext>Max N");
         ip_Socket->Send("<th class=headertext>Count Tested");
         if (ib_NeedsDoubleCheck) ip_Socket->Send("<th class=headertext>Count DC\'d");
         ip_Socket->Send("<th class=headertext>Count Untested<th class=headertext>In Progress<th class=headertext>Tests Skipped");
         ip_Socket->Send("<th class=headertext>Completed Thru<th class=headertext>Leading Edge<th class=headertext>PRPs/Primes<th class=headertext>Remaining k");
         ip_Socket->Send("</tr>");

         ip_Socket->Send("<tr bgcolor=\"white\">");
      }

      TH_32BIT(totalKs);
      TH_32BIT(totalNs);
      TH_32BIT(totalMinN);
      TH_32BIT(totalMaxN);
      TH_32BIT(totalTested);
      TH_IF_DC(totalDoubleChecked);
      TH_32BIT(totalUntested);
      TH_32BIT(totalCountInProgress);
      TH_32BIT(totalTestsSkipped);
      TH_32BIT(totalCompletedThru);
      TH_32BIT(totalLeadingEdge);
      TH_32BIT(totalPRPsAndPrimesFound);
      TH_32BIT(totalKs - totalPRPsAndPrimesFound);
      
      ip_Socket->Send("</tr></table><p><p>");
   }

   if (ib_ServerStatsSummaryOnly)
      ip_Socket->Send("<tr bgcolor=\"white\">");

   delete sqlStatement;
}
