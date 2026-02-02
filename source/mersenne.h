#include <stdint.h>

typedef struct {
   uint32_t n;
   uint32_t e;
   uint64_t rangeSize;
} dmd_t;

// This is a list of Mersenne Primes supported by PRPNet for Double-Mersenne divisor
// servers.  This table has defaults for the range sizes per Mersenne Prime.  The
// range size in this table also indicates that kMin in DMDRange must be evenly divisible
// by this number.  This is done to ensure that no matter how many ranges are loaded
// in the server that all ranges are the same size.  The values in this table were selected
// based upon https://www.doublemersennes.org/productivity.php with the intent to have
// ranges that can be done in no more than a day.
// The smallest Mersenne Primes are only in this table so divisors can be searched
// with PRPNet for testing purposes.  It is best to search for divisors for those Mesenne
// Primes outside of PRPNet.
// Support for higher Mesenne Primes will be added in the future.
dmd_t  dmdList[] = {
   {       13,  9, 100000000000},   // 1e11
   {       17,  9, 100000000000},   // 1e11
   {       19,  9, 100000000000},   // 1e11
   {       31,  9, 100000000000},   // 1e11
   {       61,  9, 100000000000},   // 1e11
   {       89,  9, 100000000000},   // 1e11
   {      107,  9, 100000000000},   // 1e11
   {      127, 11, 100000000000},   // 1e11
   {      521, 10,  10000000000},   // 1e10
   {      607, 10,  10000000000},   // 1e10
   {     1279,  9,   1000000000},   // 1e9
   {     2203,  9,   1000000000},   // 1e9
   {     2281,  9,   1000000000},   // 1e9
   {     3217,  8,    100000000},   // 1e8
   {     4253,  8,    100000000},   // 1e8
   {     4423,  8,    100000000},   // 1e8
   {        0,  0,            0} 
};
