#ifndef _ABCParser_

#define _ABCParser_

#define ABC_UNKNOWN 0

#include "Socket.h"

enum rowtype_t { RT_CANDIDATE, RT_IGNORE, RT_BATCH, RT_EOF };

class ABCParser
{
public:
   ABCParser(int32_t serverType, string fileName);

   ABCParser(Socket *theSocket, int32_t serverType);

   ~ABCParser();

   // This will indicate if the format of the ABC file can
   // be used with this server
   bool     IsValidFormat(void);

   rowtype_t GetNextCandidate(string &theName, int64_t &theK, int32_t &theB, int32_t &theN, int64_t &theC, int32_t &theD);

private:
   int32_t  ii_ABCFormat;
   int32_t  ii_ServerType;

   Socket  *ip_Socket;

   bool     ib_ABCDFormat;

   // These are populated from the first line of the ABC file
   int64_t  il_theK;
   int32_t  ii_theB;
   int32_t  ii_theN;
   int64_t  il_theC;
   int32_t  ii_theD;

   string   is_ABCFile;

   FILE    *ip_ABCFile;

   int32_t  DetermineABCFormat(void);

   int32_t  DetermineABCFormat(string abcHeader);

   bool     ParseCandidateLine(string abcLine);
};

#endif // #ifndef _ABCParser_

