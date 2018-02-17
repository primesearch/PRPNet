#ifndef _ABCParser_

#define _ABCParser_

#define ABC_UNKNOWN 0

#include "Socket.h"

class ABCParser
{
public:
   ABCParser(string fileName);

   ABCParser(Socket *theSocket, int32_t serverType);

   ~ABCParser();

   // This will indicate if the format of the ABC file can
   // be used with this server
   int32_t  IsValidFormat(void);

   int32_t  GetNextCandidate(string &theName, int64_t &theK, int32_t &theB, int32_t &theN, int32_t &theC);

private:
   int32_t  ii_ABCFormat;
   int32_t  ii_ServerType;

   Socket  *ip_Socket;

   bool     ib_firstABCDLine;

   // These are populated from the first line of the ABC file
   int64_t  il_theK;
   int32_t  ii_theB;
   int32_t  ii_theN;
   int32_t  ii_theC;

   string   is_ABCFile;

   FILE    *ip_ABCFile;

   int32_t  DetermineABCFormat(void);

   int32_t  DetermineABCFormat(string abcHeader);

   bool     ParseCandidateLine(string abcLine);
};

#endif // #ifndef _ABCParser_

