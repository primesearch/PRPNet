#ifndef _WORKUNIT_

#define _WORKUNIT_

class WorkUnitTest;

// This represents what the server sent and a few other client side pieces of information
typedef struct workunit_t
{
   int64_t     l_TestID;
   char        s_Name[50];
   int64_t     l_k;
   int32_t     i_b;
   int32_t     i_n;
   int32_t     i_c;
   int64_t     l_LowerLimit;
   int64_t     l_UpperLimit;
   int32_t     i_DecimalLength;
   bool        b_SRSkipped;
   WorkUnitTest *m_FirstWorkUnitTest;
   struct workunit_t   *m_NextWorkUnit;
} workunit_t;

#endif // _WORKUNIT_
