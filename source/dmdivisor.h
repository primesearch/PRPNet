#ifndef _dmdivisor_

#define _dmdivisor_

typedef struct
{
   void       *m_NextDMDivisor;
   uint32_t    n;
   uint64_t    k;
   char        s_Divisor[100];
} dmdivisor_t;

#endif // #ifndef _dmdivisor_
