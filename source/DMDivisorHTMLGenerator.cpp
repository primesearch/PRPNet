#include <cstdarg>

#include "DMDivisorHTMLGenerator.h"
#include "DMDivisorServerHelper.h"
#include "ServerHelper.h"
#include "SQLStatement.h"
#include "mersenne.h"

bool     DMDivisorHTMLGenerator::SendSpecificPage(string thePage)
{
   if (thePage == "divisors.html")
   {
      HeaderPlusLinks("Divisors Found");
      DivisorsFound();
   }
   else if (thePage == "pending_work.html")
   {
      HeaderPlusLinks("Pending Work");
      PendingTests();
   }
   else if (thePage == "all.html")
   {
      HeaderPlusLinks("Status");

      PendingTests();
      ServerStats();
   }
   else
      return false;

   return true;
}

void     DMDivisorHTMLGenerator::PendingTests(void)
{
   SQLStatement* sqlStatement;
   int64_t     testID, expireSeconds;
   char        userID[ID_LENGTH + 1], machineID[ID_LENGTH + 1];
   char        instanceID[ID_LENGTH + 1];
   string      kMinStr, kMaxStr;
   int64_t     kMin, kMax;
   int32_t     n, seconds, hours, minutes;
   int32_t     expireHours, expireMinutes;
   time_t      currentTime;

   const char* theSelect = "select DMDRange.n, DMDRange.kMin, DMDRange.kMax, " \
      "       DMDRangeTest.testID, DMDRangeTest.userID, " \
      "       DMDRangeTest.machineID, DMDRangeTest.instanceID " \
      "  from DMDRange, DMDRangeTest" \
      " where DMDRange.n = DMDRangeTest.n " \
      "   and DMDRange.kMin = DMDRangeTest.kMin " \
      "   and DMDRange.rangeStatus = 1 " \
      "order by DMDRangeTest.testID, DMDRange.n, DMDRange.kMin ";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&n);
   sqlStatement->BindSelectedColumn(&kMin);
   sqlStatement->BindSelectedColumn(&kMax);
   sqlStatement->BindSelectedColumn(&testID);
   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);

   currentTime = time(NULL);

   ip_Socket->Send("<article><h2>Pending Work</h2>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No Pending WU\'s"))
      return;

   ip_Socket->Send("<table id=\"pending-tests-table\"><thead><tr>");

   TH_CLMN_HDR("n");
   TH_CLMN_HDR("Min k");
   TH_CLMN_HDR("Max k");
   TH_CLMN_HDR("User");
   TH_CLMN_HDR("Machine");
   TH_CLMN_HDR("Instance");
   TH_CLMN_HDR("Date Assigned");
   TH_CLMN_HDR("Age (hh:mm)");
   TH_CLMN_HDR("Expires (hh:mm)");

   ip_Socket->Send("</tr></thead><tbody>");

   do
   {
      expireSeconds = testID + ip_Delay[0].expireDelay - (int64_t)currentTime;

      if (expireSeconds < 0) expireSeconds = 0;
      expireHours = (int32_t)(expireSeconds / 3600);
      expireMinutes = (int32_t)(expireSeconds - expireHours * 3600) / 60;

      seconds = (int32_t)(currentTime - testID);
      hours = seconds / 3600;
      minutes = (seconds - hours * 3600) / 60;

      ConvertToScientificNotation(n, kMin, kMinStr);
      ConvertToScientificNotation(n, kMax, kMaxStr);

      ip_Socket->Send("<tr><th scope=\"row\">%u, %s - %s</th>",
         n, kMinStr.c_str(), kMaxStr.c_str());

      TD_CHAR(userID);
      TD_CHAR(machineID);
      TD_CHAR(instanceID);
      TD_CHAR(TimeToString(testID).c_str());
      TD_TIME(hours, minutes);
      TD_TIME(expireHours, expireMinutes);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");

   delete sqlStatement;
}

void     DMDivisorHTMLGenerator::DivisorsFound(void)
{
   SQLStatement* sqlStatement;
   int64_t    dateReported, k;
   int32_t    n;
   int32_t    divisorCount = 0;
   char       divisor[ID_LENGTH + 1], userID[ID_LENGTH + 1], prevUserID[ID_LENGTH + 1];
   char       machineID[ID_LENGTH + 1], instanceID[ID_LENGTH + 1];

   const char* theSelect = "select n, k, divisor, userID, machineID, instanceID, dateReported " \
      "  from DMDivisor " \
      "order by n, k, divisor";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&n);
   sqlStatement->BindSelectedColumn(&k);
   sqlStatement->BindSelectedColumn(divisor, ID_LENGTH);
   sqlStatement->BindSelectedColumn(userID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(machineID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(instanceID, ID_LENGTH);
   sqlStatement->BindSelectedColumn(&dateReported);

   ip_Socket->Send("<article><h2>GFN Divisors Found</h2>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No GFN divisors found"))
      return;

   ip_Socket->StartBuffering();

   *prevUserID = 0;
   ip_Socket->Send("<table id=\"divisors-table\">");
   TH_CLMN_HDR("Divisor");
   TH_CLMN_HDR("User");
   TH_CLMN_HDR("Machine");
   TH_CLMN_HDR("Instance");
   TH_CLMN_HDR("Date Reported");

   ip_Socket->Send("</tr>");

   do
   {
      ip_Socket->Send("<tr>");

      TD_CHAR(divisor);
      TD_CHAR(userID);
      TD_CHAR(machineID);
      TD_CHAR(instanceID);
      TD_CHAR(TimeToString(dateReported).c_str());

      ip_Socket->Send("</tr>");

   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");

   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void  DMDivisorHTMLGenerator::ServerStats(void)
{
   SQLStatement* sqlStatement;
   int64_t     kMin, kMax;
   int32_t     n, rangeStatus, divisors;

   const char* theSelect = "select n, kMin, kMax, rangeStatus, divisors " \
      "  from DMDRange order by n, kMin";

   sqlStatement = new SQLStatement(ip_Log, ip_DBInterface, theSelect);

   sqlStatement->BindSelectedColumn(&n);
   sqlStatement->BindSelectedColumn(&kMin);
   sqlStatement->BindSelectedColumn(&kMax);
   sqlStatement->BindSelectedColumn(&rangeStatus);
   sqlStatement->BindSelectedColumn(&divisors);

   ip_Socket->Send("<article><h2>Server Stats</h2>");

   if (!CheckIfRecordsWereFound(sqlStatement, "No stats found"))
      return;

   ip_Socket->StartBuffering();

   ip_Socket->Send("<article><table id=\"server-stats-table\"><thead><tr>");

   TH_CLMN_HDR("n");
   TH_CLMN_HDR("Min k");
   TH_CLMN_HDR("Max k");
   TH_CLMN_HDR("Status");
   TH_CLMN_HDR("Divisors");

   ip_Socket->Send("</tr></thead><tbody>");

   do
   {
      if (ip_Socket->Send("<tr class=\"%s\">", (rangeStatus > 1 ? "untested" : "tested")))
         break;

      TD_32BIT(n);
      TD_64BIT(kMin);
      TD_64BIT(kMax);
      if (rangeStatus == 0)
         TD_CHAR("to do");
      if (rangeStatus == 1)
         TD_CHAR("in progress");
      if (rangeStatus == 2)
         TD_CHAR("done");
      TD_32BIT(divisors);

      ip_Socket->Send("</tr>");
   } while (sqlStatement->FetchRow(false));

   ip_Socket->Send("</tbody></table></article>");

   ip_Socket->SendBuffer();

   delete sqlStatement;
}

void     DMDivisorHTMLGenerator::SendLinks()
{
   ip_Socket->Send("<nav class=\"three-track\">");

   ip_Socket->Send("<div><a href=\"server_stats.html\">Server Statistics</a></div>");
   ip_Socket->Send("<div><a href=\"pending_work.html\">Pending Work</a></div>");
   ip_Socket->Send("<div><a href=\"divisors.html\">Found Divisors</a></div>");
   ip_Socket->Send("</nav><div style=\"clear: both;\"></div>");
}

void  DMDivisorHTMLGenerator::ConvertToScientificNotation(int32_t n, int64_t valueInt, string& valueStr)
{
   int32_t  eValue = 0, maxE;
   char     tempValue[50];

   if (valueInt == 0)
   {
      valueStr = "0";
      return;
   }

   for (eValue = 0; ; eValue++)
   {
      if (dmdList[eValue].n == n)
      {
         maxE = dmdList[eValue].e;
         break;
      }

      if (dmdList[eValue].n == 0)
         break;
   }

   eValue = 0;
   while (valueInt % 10 == 0 && eValue < maxE)
   {
      eValue++;
      valueInt /= 10;
   }

   snprintf(tempValue, 50, "%" PRIu64"e%d", valueInt, eValue);
   valueStr = tempValue;
}