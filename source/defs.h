#if defined(_MSC_VER) && defined(MEMLEAK)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#ifndef _defs_

#define _defs_

#define PRPNET_VERSION "5.4.7"

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
   #include <inttypes.h>
   #define UNIX
#else
   #define PRId64 "I64d"
   #define PRIu64 "I64u"
#endif

enum result_t { R_UNKNOWN = -1, R_COMPOSITE, R_PRP, R_PRIME };
enum sgtype_t { SG_NM1 = 1, SG_NP1 } ;

#define MAIN_PREFIX "Main"
#define TWIN_PREFIX "Twin"
#define SGNM1_PREFIX "SG n-1"
#define SGNP1_PREFIX "SG n+1"
#define NO_TEAM "noteam"

#define BUFFER_SIZE        100000
#define CANDIDATE_SIZE     1000
#define NAME_LENGTH        50
#define ID_LENGTH          200
#define RESIDUE_LENGTH     20

// Supported server types
#define ST_SIERPINSKIRIESEL  1
#define ST_CULLENWOODALL     2
#define ST_FIXEDBKC          3
#define ST_FIXEDBNC          4
#define ST_PRIMORIAL         5
#define ST_FACTORIAL         6
#define ST_TWIN              7
#define ST_GFN               8
#define ST_SOPHIEGERMAIN     9
#define ST_XYYX             10
#define ST_MULTIFACTORIAL   11
#define ST_WAGSTAFF         12

#define ST_WIEFERICH        21
#define ST_WILSON           22
#define ST_WALLSUNSUN       23
#define ST_WOLSTENHOLME     24

#define ST_CYCLOTOMIC       31
#define ST_CAROLKYNEA       32

#define ST_GENERIC          99

#define MAX_GENEFERS         4

#define GENEFER_cuda          "genefercuda"
#define GENEFER_OpenCL        "geneferocl"
#define GENEFER_x64           "genefx64"
#define GENEFER               "genefer"
#define GENEFER_80bit         "genefer80"

#include <iostream>
#include <string>
#include <string.h>
using namespace std;

#ifdef WIN32
   #define unlink _unlink
   #define int32_t  signed int
   #define uint32_t unsigned int
   #define int64_t  signed long long
   #define uint64_t unsigned long long
   // This is defined in prpclient.cpp
   void SetQuitting(int a);
#if defined(_MSC_VER) && defined(MEMLEAK)
   #define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
   #define new DEBUG_NEW
#endif
#endif

#ifdef UNIX
   #include <errno.h>
   #include <netdb.h>
   #include <unistd.h>

   #define Sleep(x) usleep((x)*1000)
#endif

typedef struct
{
   char      userID[80];
   uint32_t  testsPerformed;
   uint32_t  prpsFound;
   uint32_t  primesFound;
   uint32_t  gfnDivisorsFound;
   double    totalScore;
} user_t;

#define PRP_OR_PRIME(x) ((x) == R_PRP || (x) == R_PRIME)

inline result_t  ConvertResidueToResult(string residue)
{
   if (residue == "PRP")   return R_PRP;
   if (residue == " PRIME") return R_PRIME;

   return R_COMPOSITE;
}

inline void  StripCRLF(char *string)
{
   long  x;

   if (!string)
      return;

   if (*string == 0)
      return;

   x = (long) strlen(string);
   while (x > 0 && (string[x-1] == 0x0a || string[x-1] == 0x0d))
   {
      string[x-1] = 0x00;
      x--;
   }
}

#endif // _defs_

