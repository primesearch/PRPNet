#ifndef _ABCParser_

#define _ABCParser_

#include "Socket.h"

enum rowtype_t { RT_CANDIDATE, RT_IGNORE, RT_BATCH, RT_EOF };

enum abctype_t {
   ABC_UNKNOWN  =   0,

   ABC_CW_FBP   =  11,
   ABC_CW_FBM   =  12,
   ABC_CW_FBA   =  13,
   ABC_CW_VBP   =  21,
   ABC_CW_VBM   =  22,
   ABC_CW_VBA   =  23,
   ABC_FKP      =  31,
   ABC_FKM      =  32,
   ABC_FKA      =  33,
   ABC_FKB      =  34,
   ABC_FBP      =  41,
   ABC_FBM      =  42,
   ABC_FBA      =  43,
   ABC_FNP      =  51,
   ABC_FNM      =  52,
   ABC_FNA      =  53,
   ABC_VP       =  61,
   ABC_VM       =  62,
   ABC_VA       =  63,
   ABC_GFN      =  71,
   ABC_XYYXP    =  81,
   ABC_XYYXM    =  82,
   ABC_LIFCHITZ =  83,
   ABC_PHI_AB   =  91,
   ABC_PHI_ABC  =  92,

   ABC_PRIMP    = 101,
   ABC_PRIMM    = 102,
   ABC_PRIMA    = 103,
   ABC_FACTP    = 111,
   ABC_FACTM    = 112,
   ABC_FACTA    = 113,
   ABC_MF       = 114,

   ABC_CK       = 121,
   ABC_WAGSTAFF = 122,

   ABC_DGT1A    = 131,
   ABC_DGT1B    = 132,
   ABC_DGT1C    = 133,
   ABC_DGT1D    = 134,

   ABC_HCW      = 141,

   ABCD_FK      = 201,
   ABCD_FN      = 202,

   NOT_ABC      = 999
};

class ABCParser
{
public:
   ABCParser(const int32_t serverType, const string fileName);

   ABCParser(Socket *theSocket, const int32_t serverType);

   ~ABCParser();

   // This will indicate if the format of the ABC file can
   // be used with this server
   bool     IsValidFormat(void) const;

   rowtype_t GetNextCandidate(string &theName, int64_t &theK, int32_t &theB, int32_t &theN, int64_t &theC, int32_t &theD);

private:
   abctype_t        ii_ABCFormat;
   const int32_t    ii_ServerType;

   Socket          *ip_Socket;

   bool             ib_ABCDFormat;

   // These are populated from the first line of the ABC file
   int64_t          il_theK;
   int32_t          ii_theB;
   int32_t          ii_theN;
   int64_t          il_theC;
   int32_t          ii_theD;

   const string     is_ABCFile;

   FILE            *ip_ABCFile;

   abctype_t        DetermineABCFormat(void);

   abctype_t        DetermineABCFormat(const string abcHeader);

   bool             ParseCandidateLine(const string abcLine);
};

#endif // #ifndef _ABCParser_

