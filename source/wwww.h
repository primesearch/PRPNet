#ifndef _wwww_

#define _wwww_

enum wwwwtype_t { WWWW_PRIME = 1, WWWW_SPECIAL };

typedef struct
{
   void       *m_NextWWWW;
   wwwwtype_t  t_WWWWType;     
   int64_t     l_Prime;
   int32_t     i_Remainder;
   int32_t     i_Quotient;
} wwww_t;

#endif // #ifndef _wwww_
