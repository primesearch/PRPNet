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
static const char *fk_string = "%" PRId64"*$a^$b%d";
static const char *fkastring = "%" PRId64"*$a^$b$%c";

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
static const char *mf_string = "$a!%d$b";

// Any form of n#+/-c (primorials)
static const char *prim_string = "$a#%d";
static const char *primastring = "$a#+$b";

static const char *gfnstring = "$a^$b+1";

static const char *phiabstring  = "Phi($a,$b)";
static const char *phiabcstring = "Phi($a,$b^$c)";

static const char *xyyxpstring = "$a^$b+$b^$a";
static const char *xyyxmstring = "$a^$b-$b^$a";

static const char *ckstring = "(%d^$a$b)^2-2";

static const char *wagstaffstring = "(2^$a+1)/3";

static const char *fkabcdstring = "%" PRId64"*%d^$a%d [%d]";
static const char *fnabcdstring = "$a*%d^%d%d [%" PRId64"]";

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
#define ABC_GFN         71
#define ABC_XYYXP       81
#define ABC_XYYXM       82
#define ABC_PHI_AB      91
#define ABC_PHI_ABC     92

#define ABC_PRIMP      101
#define ABC_PRIMM      102
#define ABC_PRIMA      103
#define ABC_FACTP      111
#define ABC_FACTM      112
#define ABC_FACTA      113
#define ABC_MF         114

#define ABC_CK         121
#define ABC_WAGSTAFF   122

#define ABCD_FK        201
#define ABCD_FN        202

#define ABC_GENERIC    998
#define NOT_ABC        999

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
          ii_ABCFormat == ABC_VP  || ii_ABCFormat == ABC_VM  || ii_ABCFormat == ABC_VA  ||
          ii_ABCFormat == ABCD_FK || ii_ABCFormat == ABCD_FN)
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
   
   if (ii_ServerType == ST_MULTIFACTORIAL) 
       if (ii_ABCFormat == ABC_MF)
          return true;

   if (ii_ServerType == ST_GFN)
      if (ii_ABCFormat == ABC_GFN)
         return true;
   
   if (ii_ServerType == ST_XYYX)
      if (ii_ABCFormat == ABC_XYYXP || ii_ABCFormat == ABC_XYYXM)
         return true;
   
   if (ii_ServerType == ST_CYCLOTOMIC)
      if (ii_ABCFormat == ABC_PHI_AB || ii_ABCFormat == ABC_PHI_ABC)
         return true;
   
   if (ii_ServerType == ST_CAROLKYNEA) 
       if (ii_ABCFormat == ABC_CK)
          return true;

   if (ii_ServerType == ST_GENERIC)
      if (ii_ABCFormat == ABC_GENERIC || ii_ABCFormat == NOT_ABC)
         return true;
   
   if (ii_ServerType == ST_WAGSTAFF) 
       if (ii_ABCFormat == ABC_WAGSTAFF)
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

   ib_firstABCDLine = false;

   strcpy(tempHeader, abcHeader.c_str());

   if (!memcmp(tempHeader, "ABCD ", 5))
   {
      strcpy(tempHeader, abcHeader.c_str() + 5);

      ib_firstABCDLine = true;

      if (sscanf(tempHeader, fkabcdstring, &il_theK, &ii_theB, &ii_theC, &ii_theN) == 4)
          return ABCD_FK;

      if (sscanf(tempHeader, fnabcdstring, &ii_theB, &ii_theN, &ii_theC, &il_theK) == 4)
          return ABCD_FN;
   }

   if (memcmp(tempHeader, "ABC ", 4))
   {
      if (ii_ServerType == ST_GENERIC)
         return NOT_ABC;

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
     if (ch == 'c')
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

   // Any form of n#+/-c
   if (sscanf(tempHeader, prim_string, &ii_theC) == 1)
   {
     if (ii_theC > 0)
       return ABC_PRIMP;
     if (ii_theC < 0)
       return ABC_PRIMM;
   }

   if (!strncmp(tempHeader, primastring, strlen(primastring))) return ABC_PRIMA;

   // Any form of n!+/-c
   if (!strstr(tempHeader, "$b") && sscanf(tempHeader, fact_string, &ii_theC) == 1)
   {
     if (ii_theC > 0)
       return ABC_FACTP;
     if (ii_theC < 0)
       return ABC_FACTM;
   }

   // Any form of $a!%d$b
   if (strstr(tempHeader, "$b") && sscanf(tempHeader, mf_string, &ii_theB) == 1)
   {
     if (ii_theB > 0)
       return ABC_MF;
   }
   
   if (!strncmp(tempHeader, wagstaffstring, strlen(wagstaffstring))) return ABC_WAGSTAFF;

   if (sscanf(tempHeader, ckstring, &ii_theB) == 1) return ABC_CK;

   if (!strncmp(tempHeader, factastring, strlen(factastring))) return ABC_FACTA;

   if (!strncmp(tempHeader, gfnstring, strlen(gfnstring))) return ABC_GFN;

   if (!strncmp(tempHeader, xyyxpstring, strlen(xyyxpstring))) return ABC_XYYXP;

   if (!strncmp(tempHeader, xyyxmstring, strlen(xyyxmstring))) return ABC_XYYXM;

   if (!strncmp(tempHeader, phiabstring, strlen(phiabstring))) return ABC_PHI_AB;

   if (!strncmp(tempHeader, phiabcstring, strlen(phiabcstring))) return ABC_PHI_ABC;

   if (ip_Socket)
      ip_Socket->Send("ERR: ABC file format not supported [%s].\n", tempHeader);
   else
      printf("ABC file format not supported [%s].\n", tempHeader);

   return ABC_UNKNOWN;
}

int32_t  ABCParser::GetNextCandidate(string &theName, int64_t &theK, int32_t &theB, int32_t &theN, int32_t &theC)
{
   char     abcLine[200], *theMessage;
   char     tempName[BUFFER_SIZE];

   if (!ip_ABCFile && !ip_Socket)
      return false;

   if (!ib_firstABCDLine)
   {
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

         if (!memcmp(theMessage, "ABC ", 4) || !memcmp(theMessage, "ABCD " , 5))
         {
            if (DetermineABCFormat(theMessage) != ii_ABCFormat)
               return false;

            theName = theMessage;

            // Return if this is not ABCD format
            if (!ib_firstABCDLine)
               return true;
         }

         if (ii_ABCFormat == ABC_UNKNOWN)
         {
            theC = 0;
            return true;
         }

         // If is is an ABCD line from the input, then we don't need to parse it
         // as the first Candidate is derived from that line.
         if (!ib_firstABCDLine && !ParseCandidateLine(theMessage))
         {
            ip_Socket->Send("ERR: Unable to parse line [%s] from ABC file.  Processing stopped\n", theMessage);
            theC = 0;
            return true;
         }
      
         if (ii_ABCFormat == NOT_ABC)
         {
            sprintf(abcLine, "%s", theMessage);
            il_theK = 0;
            ii_theB = 0;
            ii_theN = 0;
            ii_theC = 0;
         }
      }
   }

   ib_firstABCDLine = false;

   switch (ii_ServerType)
   {
      case ST_SIERPINSKIRIESEL:
      case ST_FIXEDBKC:
      case ST_FIXEDBNC:
      case ST_TWIN:
         sprintf(tempName, "%" PRId64"*%d^%d%+d", il_theK, ii_theB, ii_theN, ii_theC);
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
         
      case ST_MULTIFACTORIAL:
         sprintf(tempName, "%d!%d%+d", ii_theN, ii_theB, ii_theC);
         break;

      case ST_GFN:
         sprintf(tempName, "%d^%d%+d", ii_theB, ii_theN, ii_theC);
         break;

      case ST_XYYX:
         sprintf(tempName, "%d^%d%c%d^%d", ii_theB, ii_theN, ((ii_theC == 1) ? '+' : '-'), ii_theN, ii_theB);
         break;
         
      case ST_CYCLOTOMIC:
         if (ii_theN == 1)
            sprintf(tempName, "Phi(%" PRId64",%d)", il_theK, ii_theB);
         else
            sprintf(tempName, "Phi(%" PRId64",%d^%d)", il_theK, ii_theB, ii_theN);
         break;

      case ST_CAROLKYNEA:
         sprintf(tempName, "(%d^%d%+d)^2-2", ii_theB, ii_theN, ii_theC);
         break;
         
      case ST_WAGSTAFF:
         sprintf(tempName, "(2^%d+1)/3", ii_theN);
         break;

       case ST_GENERIC:
          if (ii_ABCFormat == NOT_ABC)
             sprintf(tempName, "%s", abcLine);
   }

   theK = il_theK;
   theB = ii_theB;
   theN = ii_theN;
   theC = ii_theC;
   theName = tempName;

   return true;
}

bool  ABCParser::ParseCandidateLine(string abcLine)
{
   char tempLine[200];
   int32_t value32;
   int64_t value64;
   
   strcpy(tempLine, abcLine.c_str());

   if (ii_ABCFormat == NOT_ABC)
      return true;

   switch (ii_ABCFormat)
   {
      case ABCD_FK:
         if (sscanf(tempLine, "%d", &value32) != 1) return false;
         ii_theN += value32;
         return true;

      case ABCD_FN:
         if (sscanf(tempLine, "%lld", &value64) != 1) return false;
         il_theK += value64;
         return true;

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
         if (sscanf(tempLine, "%" PRId64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FBM:
         if (sscanf(tempLine, "%" PRId64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FBA:
         if (sscanf(tempLine, "%" PRId64" %d %d", &il_theK, &ii_theN, &ii_theC) != 3) return false;
         return true;

      case ABC_FNP:
         if (sscanf(tempLine, "%" PRId64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FNM:
         if (sscanf(tempLine, "%" PRId64" %d", &il_theK, &ii_theB) != 2) return false;
         return true;

      case ABC_FNA:
         if (sscanf(tempLine, "%" PRId64" %d %d", &il_theK, &ii_theB, &ii_theC) != 3) return false;
         return true;

      case ABC_VP:
         if (sscanf(tempLine, "%" PRId64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         return true;

      case ABC_VM:
         if (sscanf(tempLine, "%" PRId64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         return true;

      case ABC_VA:
         return sscanf(tempLine, "%" PRId64" %d %d %d", &il_theK, &ii_theB, &ii_theN, &ii_theC) == 4;

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
 
      case ABC_MF:
         if (sscanf(tempLine, "%d %d", &ii_theN, &ii_theC) != 2) return false;
         il_theK = ii_theN;
         return true;

      case ABC_GFN:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         il_theK = ii_theC = 1;
         return true;

      case ABC_XYYXP:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         il_theK = 1;
         ii_theC = 1;
         return true;

      case ABC_XYYXM:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         il_theK = 1;
         ii_theC = -1;
         return true;

      case ABC_PHI_AB:
         if (sscanf(tempLine, "%" PRId64" %d", &il_theK, &ii_theB) != 2) return false;
         ii_theC = ii_theN = 1;
         return true;

      case ABC_PHI_ABC:
         if (sscanf(tempLine, "%" PRId64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         ii_theC = 1;
         return true;

      case ABC_CK:
         if (sscanf(tempLine, "%d %d", &ii_theN, &ii_theC) != 2) return false;
         il_theK = 1;
         return true;

      case ABC_WAGSTAFF:
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         il_theK = ii_theB = ii_theC = 1;
         return true;
   }
   return false;
}

