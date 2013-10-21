// ABCParser.cpp
//
// Mark Rodenkirch (rogue@wi.rr.com)

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"

#include "ABCParser.h"

// Cullen/Woodall form from gcwsieve (fixed base)
static const char *cwfb_string = "$a*%d^$a%d";
static const char *cwfbastring = "$a*%d^$a$%c";

// Cullen/Woodall form from gcwsieve (variable base)
static const char *cwvb_string = "$a*$b^$a%d";
static const char *cwvbastring = "$a*$b^$a$c";

// Fixed k forms for k*b^n+/-c
static const char *fk_string = "%d*$a^$b%d";
static const char *fkastring = "%d*$a^$b$%c";

// Fixed b forms for k*b^n+/-c
static const char *fb_string = "$a*%d^$b%d";
static const char *fbastring = "$a*%d^$b$%c";

// Fixed n forms for k*b^n+/-c
static const char *fn_string = "$a*$b^%d%d";
static const char *fnastring = "$a*$b^%d$%c";

// Any form of k*b^n+/-c
static const char *abc_string = "$a*$b^$c%d";
static const char *abcastring = "$a*$b^$c$d";

// Any form of n!+/-c (factorials)
static const char *fact_string = "$a!%d";
static const char *factastring = "$a!+$b";

// Any form of n#+/-c (primorials)
static const char *prim_string = "$a#%d";
static const char *primastring = "$a#+$b";

static const char *gfnstring = "$a^$b+1";

#define ABC_UNKNOWN      0
#define ABC_CW_FBP      11
#define ABC_CW_FBM      12
#define ABC_CW_FBA      13
#define ABC_CW_VBP      21
#define ABC_CW_VBM      22
#define ABC_CW_VBA      23
#define ABC_FKP         31
#define ABC_FKM         32
#define ABC_FKA         33
#define ABC_FBP         41
#define ABC_FBM         42
#define ABC_FBA         43
#define ABC_FNP         51
#define ABC_FNM         52
#define ABC_FNA         53
#define ABC_VP          61
#define ABC_VM          62
#define ABC_VA          63
#define ABC_GFN         70

#define ABC_PRIMP      101
#define ABC_PRIMM      102
#define ABC_PRIMA      103
#define ABC_FACTP      111
#define ABC_FACTM      112
#define ABC_FACTA      113

ABCParser::ABCParser(string fileName)
{
   is_ABCFile = fileName.c_str();

   ip_Socket = 0;

   ii_ABCFormat = DetermineABCFormat();

   if (ii_ABCFormat == ABC_UNKNOWN && ip_ABCFile)
   {
      fclose(ip_ABCFile);
      ip_ABCFile = 0;
   }
}

ABCParser::ABCParser(Socket *theSocket, int32_t serverType)
{
   char  *theMessage;

   ip_Socket = theSocket;

   ii_ABCFormat = ABC_UNKNOWN;
   ii_ServerType = serverType;

   theMessage = ip_Socket->Receive();

   if (theMessage)
      ii_ABCFormat = DetermineABCFormat(theMessage);

   if (ii_ABCFormat != ABC_UNKNOWN)
   {
      if (!IsValidFormat())
      {
         ip_Socket->Send("ERR: ABC Format not compatible with server");
         ii_ABCFormat = ABC_UNKNOWN;
      }
      else
         ip_Socket->Send("OK: ABC Format Identified");
   }

   ip_ABCFile = 0;
}

ABCParser::~ABCParser()
{
   if (ip_ABCFile)
      fclose(ip_ABCFile);
}

int32_t  ABCParser::IsValidFormat(void)
{
   if (ii_ABCFormat == ABC_UNKNOWN)
      return false;

   if (ii_ServerType == ST_SIERPINSKIRIESEL || 
       ii_ServerType == ST_FIXEDBKC         ||
       ii_ServerType == ST_FIXEDBNC         ||
       ii_ServerType == ST_TWIN             ||
       ii_ServerType == ST_SOPHIEGERMAIN)
      if (ii_ABCFormat == ABC_FKP || ii_ABCFormat == ABC_FKM || ii_ABCFormat == ABC_FKA ||
          ii_ABCFormat == ABC_FBP || ii_ABCFormat == ABC_FBM || ii_ABCFormat == ABC_FBA ||
          ii_ABCFormat == ABC_FNP || ii_ABCFormat == ABC_FNM || ii_ABCFormat == ABC_FNA ||
          ii_ABCFormat == ABC_VP  || ii_ABCFormat == ABC_VM  || ii_ABCFormat == ABC_VA)
         return true;
 
   if (ii_ServerType == ST_CULLENWOODALL)
      if (ii_ABCFormat == ABC_CW_FBP || ii_ABCFormat == ABC_CW_FBM || ii_ABCFormat == ABC_CW_FBA ||
          ii_ABCFormat == ABC_CW_VBP || ii_ABCFormat == ABC_CW_VBM || ii_ABCFormat == ABC_CW_VBA)
         return true;
    
   if (ii_ServerType == ST_PRIMORIAL) 
       if (ii_ABCFormat == ABC_PRIMP || ii_ABCFormat == ABC_PRIMM || ii_ABCFormat == ABC_PRIMA)
          return true;

   if (ii_ServerType == ST_FACTORIAL) 
       if (ii_ABCFormat == ABC_FACTP || ii_ABCFormat == ABC_FACTM || ii_ABCFormat == ABC_FACTA)
          return true;

   if (ii_ServerType == ST_GFN)
      if (ii_ABCFormat == ABC_GFN)
         return true;

   return false;
}

// Determine which ABC file format is being used
int32_t  ABCParser::DetermineABCFormat(void)
{
   char     abcLine[200];

   ip_ABCFile = fopen(is_ABCFile.c_str(), "r");
   if (!ip_ABCFile)
   {
      printf("ABC file [%s] could not be opened\n", is_ABCFile.c_str());
      return ABC_UNKNOWN;
   }

   if (fgets(abcLine, sizeof(abcLine), ip_ABCFile) == NULL)
   {
      printf("ABC file [%s] is empty or could not be read\n", is_ABCFile.c_str());
      return ABC_UNKNOWN;
   }

   return DetermineABCFormat(abcLine);
}

int32_t  ABCParser::DetermineABCFormat(string abcHeader)
{
   char     ch, *pos;
   char     tempHeader[200];

   strcpy(tempHeader, abcHeader.c_str());

   if (memcmp(tempHeader, "ABC ", 4))
   {
      if (ip_Socket)
         ip_Socket->Send("ABC header [%s] does not appear to be of the correct format", tempHeader);
      else
         printf("ABC header [%s] does not appear to be of the correct format\n", tempHeader);

      return ABC_UNKNOWN;
   }

   if (strlen(tempHeader) > sizeof(tempHeader))
   {
      if (ip_Socket)
         ip_Socket->Send("ABC header [%s] is too long", tempHeader);
      else
         printf("ABC header [%s] is too long\n", tempHeader);

      return ABC_UNKNOWN;
   }

   strcpy(tempHeader, abcHeader.c_str() + 4);
   pos = strchr(tempHeader, ' ');
   if (pos) *pos = 0;

   // Cullen/Woodall form (fixed base), typically from gcwsieve
   if (sscanf(tempHeader, cwfb_string, &ii_theB, &ii_theC) == 2)
   {
     if (ii_theC > 0)
       return ABC_CW_FBP;
     if (ii_theC < 0)
       return ABC_CW_FBM;
   }

   if (sscanf(tempHeader, cwfbastring, &ii_theB, &ch) == 2)
   {
     if (ch == 'b')
       return ABC_CW_FBA;
   }

   // Cullen/Woodall form (variable base)
   if (sscanf(tempHeader, cwvb_string, &ii_theC) == 1)
   {
     if (ii_theC > 0)
       return ABC_CW_VBP;
     if (ii_theC < 0)
       return ABC_CW_VBM;
   }

   if (!strncmp(tempHeader, cwvbastring, strlen(cwvbastring))) return ABC_CW_VBA;

   // Fixed k forms for k*b^n+/-c
   if (sscanf(tempHeader, fk_string, &il_theK, &ii_theC) == 2)
   {
     if (ii_theC > 0)
       return ABC_FKP;
     if (ii_theC < 0)
       return ABC_FKM;
   }

   if (sscanf(tempHeader, fkastring, &il_theK, &ch) == 2)
   {
     if (ch == 'c')
       return ABC_FKA;
   }

   // Fixed b forms for k*b^n+/-c
   if (sscanf(tempHeader, fb_string, &ii_theB, &ii_theC) == 2)
   {
     if (ii_theC > 0)
       return ABC_FBP;
     if (ii_theC < 0)
       return ABC_FBM;
   }
   if (sscanf(tempHeader, fbastring, &ii_theB, &ch) == 2)
   {
     if (ch == 'c')
       return ABC_FBA;
   }

   // Fixed n forms for k*b^n+/-c
   if (sscanf(tempHeader, fn_string, &ii_theB, &ii_theC) == 2)
   {
     if (ii_theC > 0)
       return ABC_FNP;
     if (ii_theC < 0)
       return ABC_FNM;
   }
   if (sscanf(tempHeader, fnastring, &ii_theB, &ch) == 2)
   {
     if (ch == 'c')
       return ABC_FNA;
   }

   // Any form of k*b^n+/-c
   if (sscanf(tempHeader, abc_string, &ii_theC) == 1)
   {
     if (ii_theC > 0)
       return ABC_VP;
     if (ii_theC < 0)
       return ABC_VM;
   }
   if (!strncmp(tempHeader, abcastring, strlen(abcastring))) return ABC_VA;

   // Any form of n!+/-c
   if (sscanf(tempHeader, prim_string, &ii_theC) == 1)
   {
     if (ii_theC > 0)
       return ABC_PRIMP;
     if (ii_theC < 0)
       return ABC_PRIMM;
   }
   if (!strncmp(tempHeader, primastring, strlen(primastring))) return ABC_PRIMA;

   // Any form of n#+/-c
   if (sscanf(tempHeader, fact_string, &ii_theC) == 1)
   {
     if (ii_theC > 0)
       return ABC_FACTP;
     if (ii_theC < 0)
       return ABC_FACTM;
   }
   if (!strncmp(tempHeader, factastring, strlen(factastring))) return ABC_FACTA;

   if (!strncmp(tempHeader, gfnstring, strlen(gfnstring))) return ABC_GFN;

   if (ip_Socket)
      ip_Socket->Send("ERR: ABC file format not supported [%s].\n", tempHeader);
   else
      printf("ABC file format not supported [%s].\n", tempHeader);

   return ABC_UNKNOWN;
}

int32_t  ABCParser::GetNextCandidate(string &theName, int64_t &theK, int32_t &theB, int32_t &theN, int32_t &theC)
{
   char     abcLine[100], *theMessage;
   char     tempName[BUFFER_SIZE];

   if (!ip_ABCFile && !ip_Socket)
      return false;

   if (ip_ABCFile)
   {
      if (fgets(abcLine, sizeof(abcLine), ip_ABCFile) == NULL)
      {
         fclose(ip_ABCFile);
         ip_ABCFile = 0;
         return false;
      }

      StripCRLF(abcLine);

      if (!ParseCandidateLine(abcLine))
      {
         fclose(ip_ABCFile);
         ip_ABCFile = 0;
         printf("Unable to parse line [%s] from ABC file.  Processing stopped\n", abcLine);
         return false;
      }
   }
   else
   {
      theMessage = ip_Socket->Receive();

      if (!theMessage)
         return false;

      if (!memcmp(theMessage, "End of File", 11))
         return false;

      if (!memcmp(theMessage, "sent", 4))
      {
         theName = theMessage;
         return true;
      }

      if (ii_ABCFormat == ABC_UNKNOWN)
      {
         theC = 0;
         return true;
      }

      if (!ParseCandidateLine(theMessage))
      {
         ip_Socket->Send("ERR: Unable to parse line [%s] from ABC file.  Processing stopped\n", theMessage);
         theC = 0;
         return true;
      }
   }

   switch (ii_ServerType)
   {
      case ST_SIERPINSKIRIESEL:
      case ST_FIXEDBKC:
      case ST_FIXEDBNC:
      case ST_TWIN:
         sprintf(tempName, "%"PRId64"*%d^%d%+d", il_theK, ii_theB, ii_theN, ii_theC);
         break;

      case ST_CULLENWOODALL:
         sprintf(tempName, "%d*%d^%d%+d", ii_theN, ii_theB, ii_theN, ii_theC);
         break;

      case ST_PRIMORIAL:
         sprintf(tempName, "%d#%+d", ii_theN, ii_theC);
         break;

      case ST_FACTORIAL:
         sprintf(tempName, "%d!%+d", ii_theN, ii_theC);
         break;

      case ST_GFN:
         sprintf(tempName, "%d^%d%+d", ii_theB, ii_theN, ii_theC);
         break;
   }

   theK = il_theK;
   theB = ii_theB;
   theN = ii_theN;
   theC = ii_theC;
   theName = tempName;

   return true;
}

int32_t  ABCParser::ParseCandidateLine(string abcLine)
{
   char tempLine[200];

   strcpy(tempLine, abcLine.c_str());

   switch (ii_ABCFormat)
   {
      case ABC_CW_FBP:
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         il_theK = ii_theN;
         return true;

      case ABC_CW_FBM:
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         il_theK = ii_theN;
         return true;

      case ABC_CW_FBA:
         if (sscanf(tempLine, "%d %d", &ii_theN, &ii_theC) != 2) return false;
         il_theK = ii_theN;
         return true;

      case ABC_CW_VBP:
         if (sscanf(tempLine, "%d %d", &ii_theN, &ii_theB) != 2) return false;
         il_theK = ii_theN;
         return true;

      case ABC_CW_VBM:
         if (sscanf(tempLine, "%d %d", &ii_theN, &ii_theB) != 2) return false;
         il_theK = ii_theN;
         return true;

      case ABC_CW_VBA:
         if (sscanf(tempLine, "%d %d %d", &ii_theN, &ii_theB, &ii_theC) != 3) return false;
         il_theK = ii_theN;
         return true;

      case ABC_FKP:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         return true;

      case ABC_FKM:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         return true;

      case ABC_FKA:
         if (sscanf(tempLine, "%d %d %d", &ii_theB, &ii_theN, &ii_theC) != 3) return false;
         return true;

      case ABC_FBP:
         if (sscanf(tempLine, "%"PRId64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FBM:
         if (sscanf(tempLine, "%"PRId64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FBA:
         if (sscanf(tempLine, "%"PRId64" %d %d", &il_theK, &ii_theN, &ii_theC) != 3) return false;
         return true;

      case ABC_FNP:
         if (sscanf(tempLine, "%"PRId64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FNM:
         if (sscanf(tempLine, "%"PRId64" %d", &il_theK, &ii_theB) != 2) return false;
         return true;

      case ABC_FNA:
         if (sscanf(tempLine, "%"PRId64" %d %d", &il_theK, &ii_theB, &ii_theC) != 3) return false;
         return true;

      case ABC_VP:
         if (sscanf(tempLine, "%"PRId64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         return true;

      case ABC_VM:
         if (sscanf(tempLine, "%"PRId64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         return true;

      case ABC_VA:
         return sscanf(tempLine, "%"PRId64" %d %d %d", &il_theK, &ii_theB, &ii_theN, &ii_theC) == 4;

      case ABC_PRIMM:
      case ABC_FACTM:
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         il_theK = ii_theB = ii_theN;
         return true;

      case ABC_PRIMP:
      case ABC_FACTP:
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         il_theK = ii_theB = ii_theN;
         return true;

      case ABC_PRIMA:
      case ABC_FACTA:
         if (sscanf(tempLine, "%d %d", &ii_theN, &ii_theC) != 2) return false;
         il_theK = ii_theB = ii_theN;
         return true;
 
      case ABC_GFN:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         il_theK = ii_theC = 1;
         return true;
   }
   return false;
}

