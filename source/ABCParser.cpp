#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "defs.h"

#include "ABCParser.h"

// Cullen/Woodall form from gcwsieve (fixed base)
static const char * const cwfb_string = "$a*%d^$a%" PRId64"";
static const char * const cwfbastring = "$a*%d^$a$%c";

// Cullen/Woodall form from gcwsieve (variable base)
static const char * const cwvb_string = "$a*$b^$a%" PRId64"";
static const char * const cwvbastring = "$a*$b^$a$c";

// Fixed k forms for k*b^n+/-c
static const char * const fk_string = "%" PRIu64"*$a^$b%" PRId64"";
static const char * const fkastring = "%" PRIu64"*$a^$b$%c";

// Fixed k/b forms for k*b^n+/-c
static const char * const fkbstring = "%" PRIu64"*%d^$a%" PRId64"";

// Fixed b forms for k*b^n+/-c
static const char * const fb_string = "$a*%d^$b%" PRId64"";
static const char * const fbastring = "$a*%d^$b$%c";

// Fixed n forms for k*b^n+/-c
static const char * const fn_string = "$a*$b^%d%" PRId64"";
static const char * const fnastring = "$a*$b^%d$%c";

// Any form of k*b^n+/-c
static const char * const abc_string = "$a*$b^$c%" PRId64"";
static const char * const abcastring = "$a*$b^$c$" PRId64"";

// Any form of n!+/-c (factorials)
static const char * const fact_string = "$a!%" PRId64"";
static const char * const factastring = "$a!+$b";
static const char * const mf_string = "$a!%d$b";

// Any form of n#+/-c (primorials)
static const char * const prim_string = "$a#%" PRId64"";
static const char * const primastring = "$a#+$b";

static const char * const gfnstring = "$a^$b+1";

static const char * const phiabstring  = "Phi($a,$b)";
static const char * const phiabcstring = "Phi($a,$b^$c)";

static const char * const xyyxpstring = "$a^$b+$b^$a";
static const char * const xyyxmstring = "$a^$b-$b^$a";
static const char * const lifchitzstring = "$a^$a$b*$c^$c";

static const char * const ckstring = "(%d^$a$b)^2-2";

static const char * const wagstaffstring = "(2^$a+1)/3";

static const char * const fkabcdstring = "%" PRIu64"*%d^$a%" PRId64" [%d]";
static const char * const fnabcdstring = "$a*%d^%d%" PRId64" [%" PRIu64"]";

static const char * const dgt1string1 = "(%" PRIu64"*%d^$a%" PRId64")/%d [%d]";
static const char * const dgt1string2 = "(%" PRIu64"*%d^$a%" PRId64")/%d";
static const char * const dgt1string3 = "(%d^$a%" PRId64")/%d [%d]";
static const char * const dgt1string4 = "(%d^$a%" PRId64")/%d";

static const char * const hcwstring = "$a^$b*$b^$a$c";

ABCParser::ABCParser(const int32_t serverType, const string fileName) : ii_ServerType(serverType), is_ABCFile(fileName)
{
   ip_Socket = nullptr;

   ii_ABCFormat = DetermineABCFormat();

   if (ii_ABCFormat == ABC_UNKNOWN && ip_ABCFile)
   {
      fclose(ip_ABCFile);
      ip_ABCFile = nullptr;
   }
}

ABCParser::ABCParser(Socket *theSocket, const int32_t serverType) : ii_ServerType(serverType), is_ABCFile("")
{
   ip_Socket = theSocket;

   ip_ABCFile = nullptr;

   const string theMessage = ip_Socket->Receive();

   if (theMessage.empty())
   {
      ii_ABCFormat = ABC_UNKNOWN;
      return;
   }

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
}

ABCParser::~ABCParser()
{
   if (ip_ABCFile)
      fclose(ip_ABCFile);
}

bool  ABCParser::IsValidFormat(void) const
{
   if (ii_ABCFormat == ABC_UNKNOWN)
      return false;

   if (ii_ServerType == ST_SIERPINSKIRIESEL || 
       ii_ServerType == ST_FIXEDBKC         ||
       ii_ServerType == ST_FIXEDBNC         ||
       ii_ServerType == ST_TWIN             ||
       ii_ServerType == ST_SOPHIEGERMAIN    ||
       ii_ServerType == ST_TWINANDSOPHIE)
      if (ii_ABCFormat == ABC_FKP || ii_ABCFormat == ABC_FKM || ii_ABCFormat == ABC_FKA ||
          ii_ABCFormat == ABC_FBP || ii_ABCFormat == ABC_FBM || ii_ABCFormat == ABC_FBA ||
          ii_ABCFormat == ABC_FNP || ii_ABCFormat == ABC_FNM || ii_ABCFormat == ABC_FNA ||
          ii_ABCFormat == ABC_VP  || ii_ABCFormat == ABC_VM  || ii_ABCFormat == ABC_VA  ||
          ii_ABCFormat == ABCD_FK || ii_ABCFormat == ABCD_FN || ii_ABCFormat == ABC_FKB)
         return true;

   if (ii_ServerType == ST_SIERPINSKIRIESEL ||
       ii_ServerType == ST_FIXEDBKC ||
       ii_ServerType == ST_FIXEDBNC)
      if (ii_ABCFormat == ABC_DGT1A || ii_ABCFormat == ABC_DGT1B ||
          ii_ABCFormat == ABC_DGT1C || ii_ABCFormat == ABC_DGT1D)
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
   
   if (ii_ServerType == ST_LEYLAND)
      if (ii_ABCFormat == ABC_XYYXP || ii_ABCFormat == ABC_XYYXM)
         return true;

   if (ii_ServerType == ST_LIFCHITZ)
      if (ii_ABCFormat == ABC_LIFCHITZ)
         return true;

   if (ii_ServerType == ST_CYCLOTOMIC)
      if (ii_ABCFormat == ABC_PHI_AB || ii_ABCFormat == ABC_PHI_ABC)
         return true;
   
   if (ii_ServerType == ST_CAROLKYNEA) 
       if (ii_ABCFormat == ABC_CK)
          return true;

   if (ii_ServerType == ST_HYPERCW) 
       if (ii_ABCFormat == ABC_HCW)
          return true;

   if (ii_ServerType == ST_GENERIC)
      if (ii_ABCFormat == NOT_ABC)
         return true;
   
   if (ii_ServerType == ST_WAGSTAFF) 
       if (ii_ABCFormat == ABC_WAGSTAFF)
          return true;

   return false;
}

// Determine which ABC file format is being used
abctype_t   ABCParser::DetermineABCFormat(void)
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

   const abctype_t serverType = DetermineABCFormat(abcLine);

   if (il_theK < 0)
   {
      printf("ABC file [%s] has k > 63 bits\n", is_ABCFile.c_str());
      return ABC_UNKNOWN;
   }

   if (ii_theB < 0)
   {
      printf("ABC file [%s] has b > 31 bits\n", is_ABCFile.c_str());
      return ABC_UNKNOWN;
   }

   if (ii_theN < 0)
   {
      printf("ABC file [%s] has n > 31 bits\n", is_ABCFile.c_str());
      return ABC_UNKNOWN;
   }

   return serverType;
}

abctype_t   ABCParser::DetermineABCFormat(const string abcHeader)
{
   char     ch;
   char     tempHeader[200];

   ib_ABCDFormat = false;
   il_theK = 0;
   ii_theB = ii_theN = ii_theD = 0;
   il_theC = 0;

   if (abcHeader.length() > sizeof(tempHeader) - 1)
   {
      if (ip_Socket)
         ip_Socket->Send("ABC header [%s] is too long", abcHeader.c_str());
      else
         printf("ABC header [%s] is too long\n", abcHeader.c_str());

      return ABC_UNKNOWN;
   }

   strcpy(tempHeader, abcHeader.c_str());

   if (!memcmp(tempHeader, "ABCD ", 5))
   {
      strcpy(tempHeader, abcHeader.c_str() + 5);

      ib_ABCDFormat = true;

      if (sscanf(tempHeader, dgt1string1, &il_theK, &ii_theB, &il_theC, &ii_theD, &ii_theN) == 5)
         return ABC_DGT1A;

      if (sscanf(tempHeader, dgt1string3, &ii_theB, &il_theC, &ii_theD, &ii_theN) == 4)
      {
         il_theK = 1;
         return ABC_DGT1C;
      }

      ii_theD = 1;

      if (sscanf(tempHeader, fkabcdstring, &il_theK, &ii_theB, &il_theC, &ii_theN) == 4)
          return ABCD_FK;

      if (sscanf(tempHeader, fnabcdstring, &ii_theB, &ii_theN, &il_theC, &il_theK) == 4)
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

   strcpy(tempHeader, abcHeader.c_str() + 4);
   char * const pos = strchr(tempHeader, ' ');
   if (pos) *pos = 0;

   if (sscanf(tempHeader, dgt1string2, &il_theK, &ii_theB, &il_theC, &ii_theD) == 4) return ABC_DGT1B;
   if (sscanf(tempHeader, dgt1string4, &ii_theB, &il_theC, &ii_theD) == 3)
   {
      il_theK = 1;
      return ABC_DGT1D;
   }

   // d != 1 is not supportd for any forms below
   ii_theD = 1;

   if (strstr(tempHeader, "$b") == NULL)
   {
      // Cullen/Woodall form (fixed base), typically from gcwsieve
      if (sscanf(tempHeader, cwfb_string, &ii_theB, &il_theC) == 2)
      {
         if (il_theC > 0)
            return ABC_CW_FBP;
         if (il_theC < 0)
            return ABC_CW_FBM;
      }

      if (sscanf(tempHeader, cwfbastring, &ii_theB, &ch) == 2)
      {
         if (ch == 'c')
            return ABC_CW_FBA;
      }
   }

   // Cullen/Woodall form (variable base)
   if (sscanf(tempHeader, cwvb_string, &il_theC) == 1)
   {
      if (il_theC > 0)
         return ABC_CW_VBP;
      if (il_theC < 0)
         return ABC_CW_VBM;
   }

   if (!strncmp(tempHeader, cwvbastring, strlen(cwvbastring))) return ABC_CW_VBA;
   if (!strncmp(tempHeader, hcwstring, strlen(hcwstring))) return ABC_HCW;

   // Fixed k forms for k*b^n+/-c
   if (sscanf(tempHeader, fk_string, &il_theK, &il_theC) == 2)
   {
      if (il_theC > 0)
         return ABC_FKP;
      if (il_theC < 0)
         return ABC_FKM;
   }

   if (sscanf(tempHeader, fkastring, &il_theK, &ch) == 2)
   {
      if (ch == 'c')
         return ABC_FKA;
   }

   if (sscanf(tempHeader, fkbstring, &il_theK, &ii_theB, &il_theC) == 3)
      return ABC_FKB;

   // Fixed b forms for k*b^n+/-c
   if (sscanf(tempHeader, fb_string, &ii_theB, &il_theC) == 2)
   {
      if (il_theC > 0)
        return ABC_FBP;
      if (il_theC < 0)
         return ABC_FBM;
   }

   if (sscanf(tempHeader, fbastring, &ii_theB, &ch) == 2)
   {
      if (ch == 'c')
         return ABC_FBA;
   }

   // Fixed n forms for k*b^n+/-c
   if (sscanf(tempHeader, fn_string, &ii_theB, &il_theC) == 2)
   {
      if (il_theC > 0)
         return ABC_FNP;
      
      if (il_theC < 0)
         return ABC_FNM;
   }

   if (sscanf(tempHeader, fnastring, &ii_theB, &ch) == 2)
   {
      if (ch == 'c')
         return ABC_FNA;
   }

   // Any form of k*b^n+/-c
   if (sscanf(tempHeader, abc_string, &il_theC) == 1)
   {    
      if (il_theC > 0)
         return ABC_VP;
     
      if (il_theC < 0)
         return ABC_VM;
   }

   if (!strncmp(tempHeader, abcastring, strlen(abcastring))) return ABC_VA;

   // Any form of n#+/-c
   if (sscanf(tempHeader, prim_string, &il_theC) == 1)
   {
      if (il_theC > 0)
         return ABC_PRIMP;
      
      if (il_theC < 0)
         return ABC_PRIMM;
   }

   if (!strncmp(tempHeader, primastring, strlen(primastring))) return ABC_PRIMA;

   // Any form of n!+/-c
   if (!strstr(tempHeader, "$b") && sscanf(tempHeader, fact_string, &il_theC) == 1)
   {
      if (il_theC > 0)
         return ABC_FACTP;
      if (il_theC < 0)
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

   if (!strncmp(tempHeader, lifchitzstring, strlen(lifchitzstring))) return ABC_LIFCHITZ;

   if (!strncmp(tempHeader, phiabstring, strlen(phiabstring))) return ABC_PHI_AB;

   if (!strncmp(tempHeader, phiabcstring, strlen(phiabcstring))) return ABC_PHI_ABC;

   if (ip_Socket)
      ip_Socket->Send("ERR: ABC file format not supported [%s].\n", tempHeader);
   else
      printf("ABC file format not supported [%s].\n", tempHeader);

   return ABC_UNKNOWN;
}

rowtype_t  ABCParser::GetNextCandidate(string &theName, int64_t &theK, int32_t &theB, int32_t &theN, int64_t &theC, int32_t &theD)
{
   char     abcLine[200];
   char     tempName[BUFFER_SIZE];

   if (!ip_ABCFile && !ip_Socket)
      return RT_EOF;

   if (!ib_ABCDFormat)
   {
      if (ip_ABCFile)
      {
         if (fgets(abcLine, sizeof(abcLine), ip_ABCFile) == NULL)
         {
            fclose(ip_ABCFile);
            ip_ABCFile = nullptr;
            return RT_EOF;
         }

         StripCRLF(abcLine);

         if (!memcmp(abcLine, "ABC ", 4) || !memcmp(abcLine, "ABCD ", 5))
         {
            if (DetermineABCFormat(abcLine) != ii_ABCFormat)
            {
               printf("Mixed ABC formats is not supported\n");
               return RT_EOF;
            }

            // Return if this is ABC format
            if (!memcmp(abcLine, "ABC ", 4))
               return RT_IGNORE;
         }
         else
         {
            if (!ParseCandidateLine(abcLine))
            {
               fclose(ip_ABCFile);
               ip_ABCFile = nullptr;
               printf("Unable to parse line [%s] from ABC file.  Processing stopped\n", abcLine);
               return RT_EOF;
            }
         }
      }
      else
      {
         const char * const theMessage = ip_Socket->Receive();

         if (!theMessage)
            return RT_EOF;

         if (!memcmp(theMessage, "End of File", 11))
            return RT_EOF;

         if (!memcmp(theMessage, "sent", 4))
         {
            theName = theMessage;
            return RT_BATCH;
         }

         if (!memcmp(theMessage, "ABC ", 4) || !memcmp(theMessage, "ABCD " , 5))
         {
            if (DetermineABCFormat(theMessage) != ii_ABCFormat)
            {
               ip_Socket->Send("ERR: Mixed ABC formats is not supported");
               return RT_EOF;
            }

            theName = theMessage;

            // Return if this is ABC format
            if (!memcmp(abcLine, "ABC ", 4))
               return RT_IGNORE;
         }

         if (ii_ABCFormat == ABC_UNKNOWN)
         {
            theC = 0;
            theD = 0;
            return RT_EOF;
         }

         // If is is an ABCD line from the input, then we don't need to parse it
         // as the first Candidate is derived from that line.
         if (!ib_ABCDFormat && !ParseCandidateLine(theMessage))
         {
            ip_Socket->Send("ERR: Unable to parse line [%s] from ABC file.  Processing stopped\n", theMessage);
            theC = 0;
            theD = 0;
            return RT_EOF;
         }
      
         if (ii_ABCFormat == NOT_ABC)
         {
            snprintf(abcLine, sizeof(abcLine), "%s", theMessage);
            il_theK = 0;
            ii_theB = 0;
            ii_theN = 0;
            il_theC = 0;
            ii_theD = 0;
         }
      }
   }

   ib_ABCDFormat = false;

   switch (ii_ServerType)
   {
      case ST_SIERPINSKIRIESEL:
      case ST_FIXEDBKC:
      case ST_FIXEDBNC:
      case ST_SOPHIEGERMAIN:
      case ST_TWIN:
      case ST_TWINANDSOPHIE:
         if (ii_theD > 1)
         {
            if (il_theK > 1)
               snprintf(tempName, BUFFER_SIZE, "(%" PRIu64"*%d^%d%+" PRId64")/%d", il_theK, ii_theB, ii_theN, il_theC, ii_theD);
            else
               snprintf(tempName, BUFFER_SIZE, "(%d^%d%+" PRId64")/%d", ii_theB, ii_theN, il_theC, ii_theD);
         }
         else
            snprintf(tempName, BUFFER_SIZE, "%" PRIu64"*%d^%d%+" PRId64"", il_theK, ii_theB, ii_theN, il_theC);
         break;

      case ST_CULLENWOODALL:
         snprintf(tempName, BUFFER_SIZE, "%d*%d^%d%+" PRId64"", ii_theN, ii_theB, ii_theN, il_theC);
         break;

      case ST_PRIMORIAL:
         snprintf(tempName, BUFFER_SIZE, "%d#%+" PRId64"", ii_theN, il_theC);
         break;

      case ST_FACTORIAL:
         snprintf(tempName, BUFFER_SIZE, "%d!%+" PRId64"", ii_theN, il_theC);
         break;
         
      case ST_MULTIFACTORIAL:
         snprintf(tempName, BUFFER_SIZE, "%d!%d%+" PRId64"", ii_theN, ii_theB, il_theC);
         break;

      case ST_GFN:
         snprintf(tempName, BUFFER_SIZE, "%d^%d%+" PRId64"", ii_theB, ii_theN, il_theC);
         break;

      case ST_LEYLAND:
         snprintf(tempName, BUFFER_SIZE, "%d^%d%c%d^%d", ii_theB, ii_theN, ((il_theC == 1) ? '+' : '-'), ii_theN, ii_theB);
         break;

      case ST_LIFCHITZ:
         snprintf(tempName, BUFFER_SIZE, "%d^%d%c%d^%d", ii_theB, ii_theB, ((il_theC == 1) ? '+' : '-'), ii_theN, ii_theN);
         break;

      case ST_HYPERCW:
         snprintf(tempName, BUFFER_SIZE, "%d^%d*%d^%d%+" PRId64"", ii_theB, ii_theN, ii_theN, ii_theB, il_theC);
         break;
         
      case ST_CYCLOTOMIC:
         if (ii_theN == 1)
            snprintf(tempName, BUFFER_SIZE, "Phi(%" PRIu64",%d)", il_theK, ii_theB);
         else
            snprintf(tempName, BUFFER_SIZE, "Phi(%" PRIu64",%d^%d)", il_theK, ii_theB, ii_theN);
         break;

      case ST_CAROLKYNEA:
         snprintf(tempName, BUFFER_SIZE, "(%d^%d%+" PRId64")^2-2", ii_theB, ii_theN, il_theC);
         break;
         
      case ST_WAGSTAFF:
         snprintf(tempName, BUFFER_SIZE, "(2^%d+1)/3", ii_theN);
         break;

       case ST_GENERIC:
          if (ii_ABCFormat == NOT_ABC)
             snprintf(tempName, BUFFER_SIZE, "%s", abcLine);
   }

   theK = il_theK;
   theB = ii_theB;
   theN = ii_theN;
   theC = il_theC;
   theD = ii_theD;
   theName = tempName;

   return RT_CANDIDATE;
}

bool  ABCParser::ParseCandidateLine(const string abcLine)
{
   char tempLine[200];
   int32_t value32;
   int64_t value64;
   
   strcpy(tempLine, abcLine.c_str());

   if (ii_ABCFormat == NOT_ABC)
      return true;

   // This is only used for Sierpinski/Riesel forms, but numerous ABC/ABCD formats are used for
   // Sierpinski/Riesel, so we use this for a catch all when we know that d = 1.
   if (ii_ABCFormat != ABC_DGT1A && ii_ABCFormat != ABC_DGT1B &&
       ii_ABCFormat != ABC_DGT1C && ii_ABCFormat != ABC_DGT1D)
      ii_theD = 1;

   switch (ii_ABCFormat)
   {
      case ABCD_FN:
         if (sscanf(tempLine, "%" PRId64"", &value64) != 1) return false;
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
         if (sscanf(tempLine, "%d %" PRId64"", &ii_theN, &il_theC) != 2) return false;
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
         if (sscanf(tempLine, "%d %d %" PRId64"", &ii_theN, &ii_theB, &il_theC) != 3) return false;
         il_theK = ii_theN;
         return true;

      case ABC_FKP:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         return true;

      case ABC_FKM:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         return true;

      case ABC_FKA:
         if (sscanf(tempLine, "%d %d %" PRId64"", &ii_theB, &ii_theN, &il_theC) != 3) return false;
         return true;

      case ABC_DGT1C:
         il_theK = 1;
      case ABC_DGT1A:
         if (sscanf(tempLine, "%d", &value32) != 1) return false;
         ii_theN += value32;
         return true;

      case ABCD_FK:
         ii_theD = 1;
         if (sscanf(tempLine, "%d", &value32) != 1) return false;
         ii_theN += value32;
         return true;

      case ABC_DGT1D:
         il_theK = 1;
      case ABC_DGT1B:
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         return true;

      case ABC_FKB:
         ii_theD = 1;
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         return true;

      case ABC_FBP:
         if (sscanf(tempLine, "%" PRIu64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FBM:
         if (sscanf(tempLine, "%" PRIu64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FBA:
         if (sscanf(tempLine, "%" PRIu64" %d %" PRId64"", &il_theK, &ii_theN, &il_theC) != 3) return false;
         return true;

      case ABC_FNP:
         if (sscanf(tempLine, "%" PRIu64" %d", &il_theK, &ii_theN) != 2) return false;
         return true;

      case ABC_FNM:
         if (sscanf(tempLine, "%" PRIu64" %d", &il_theK, &ii_theB) != 2) return false;
         return true;

      case ABC_FNA:
         if (sscanf(tempLine, "%" PRIu64" %d %" PRId64"", &il_theK, &ii_theB, &il_theC) != 3) return false;
         return true;

      case ABC_VP:
         if (sscanf(tempLine, "%" PRIu64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         return true;

      case ABC_VM:
         if (sscanf(tempLine, "%" PRIu64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         return true;

      case ABC_VA:
         return sscanf(tempLine, "%" PRIu64" %d %d %" PRId64"", &il_theK, &ii_theB, &ii_theN, &il_theC) == 4;

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
         if (sscanf(tempLine, "%d %" PRId64"", &ii_theN, &il_theC) != 2) return false;
         il_theK = ii_theB = ii_theN;
         return true;
 
      case ABC_MF:
         if (sscanf(tempLine, "%d %" PRId64"", &ii_theN, &il_theC) != 2) return false;
         il_theK = ii_theN;
         return true;

      case ABC_GFN:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         il_theK = il_theC = 1;
         return true;

      case ABC_XYYXP:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         il_theK = 1;
         il_theC = 1;
         return true;

      case ABC_XYYXM:
         if (sscanf(tempLine, "%d %d", &ii_theB, &ii_theN) != 2) return false;
         il_theK = 1;
         il_theC = -1;
         return true;

      case ABC_LIFCHITZ:
         if (sscanf(tempLine, "%d %" PRIu64" %d", &ii_theB, &il_theC, &ii_theN) != 3) return false;
         il_theK = 1;
         return true;

      case ABC_HCW:
         if (sscanf(tempLine, "%d %d %" PRIu64"", &ii_theB, &ii_theN, &il_theC) != 3) return false;
         il_theK = 1;
         return true;

      case ABC_PHI_AB:
         if (sscanf(tempLine, "%" PRIu64" %d", &il_theK, &ii_theB) != 2) return false;
         il_theC = ii_theN = 1;
         return true;

      case ABC_PHI_ABC:
         if (sscanf(tempLine, "%" PRIu64" %d %d", &il_theK, &ii_theB, &ii_theN) != 3) return false;
         il_theC = 1;
         return true;

      case ABC_CK:
         if (sscanf(tempLine, "%d %" PRId64"", &ii_theN, &il_theC) != 2) return false;
         il_theK = 1;
         return true;

      case ABC_WAGSTAFF:
         if (sscanf(tempLine, "%d", &ii_theN) != 1) return false;
         il_theK = il_theC = ii_theB = 1;
         return true;

      case NOT_ABC:
         return true;

      default:
         return false;
   }
}
