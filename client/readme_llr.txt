
		MAIN FEATURES OF LLR Version 3.7.1c

LLR is a primality proving program for numbers of the form N = k*2^n +/- 1,
with k < 2^n. It takes its input data from a Newpgen output file, and also
from some particular ABC format files.

This version uses the recent release (24.14) of George Woltman's Gwnums
library, to do fast multiplications and squarings of large integers modulo N.

It can now work on all PC's, using the new Gwnums x87 code on non-P4 machines.

Thanks to the work of Mark Rodenkirch, executables binaries of LLR can now
be build that run on MacIntel / OS X machines!

Mark also helped me to improve the screen output of the program when used in
command line mode, and to make it accept more ABC formats.

The new Gwnums allow non zero padded Irrational Base FFTs with k's up to
20 bits, and zero padded ones with k's up to 48 bits, so this version is
a major enhancement of the first IBDWT version of LLR, which uses the
Colin Percival's method, and is really fast only for k's up to 9 bits
(However, I will continue to support it some time).

An important new feature of LLR is the primality test of Gaussian-Mersenne
norms.In order to make this prime search really efficient, it is necessary
that the prime exponent candidates would be sieved enough by prefactoring,
so I adapted the George Woltman's "factor32.asm" for 4^p+1 factoring, and
this new code is included in LLR to eliminate the candidates having a
non trivial factor, before doing the Proth test. The factoring upper limit
is dependent on the exponent size, and can reach 86 bits, as for GIMPS.
Also, an option allows to use the LLR program for a factoring-only job.


************************** VERSIONS HISTORY *****************************

Version 3.7.1c :

-Relevant binaries can now run on MacIntel / OS X machines.

-These ABC formats are now accepted :

Fixed k forms for k*b^n+/-1 : %d*$a^$b+%d, %d*$a^$b-%d, %d*$a^$b$c

Fixed b forms for k*b^n+/-1 : $a*%d^$b+%d, $a*%d^$b-%d, $a*%d^$b$c

Fixed n forms for k*b^n+/-1 : $a*$b^%d+%d, $a*$b^%d-%d, $a*$b^%d$c

Any form of k*b^n+/-1 : $a*$b^$c+%d, $a*$b^$c-%d, $a*$b^$c$d";

-The screen output in command line mode is now much less verbose!

-30/10/2007 : A slight bug in gwnums library, causing scarcely a
SUM(INPUTS) != SUM(OUTPUTS) error, has been fixed by George Woltman.
This fix has been inserted here.

Version 3.7.1 :

-06/05/2007 : A bug in gwnums library, affecting the generic reduction FFT,
has been fixed by George Woltman. This fix has been inserted here.

Version 3.7.0 :

-Following an idea suggested by Harsh Aggarwal, the primality test of the
Gaussian-Mersenne norms has been added as a new feature.
The input file for these tests must have this ABC format first line :

ABC 4^$a+1

Each following line contains only the Gaussian-Mersenne exponent, which must
be prime. The algorithm suggested by Harsh Aggarwal allows to do the Proth
test of the Gaussian-Mersenne norm N and the PRP test of (2^2p + 1)/N/5 at
the same time.
So, the program can simultaneously prove the primality (in the Z+iZ ring)
of GM(p) = (1+/-i)^p - 1, and the probable primality of numbers of the form:
GQ(p) = ((1+/-i)^p + 1)/(2+/-i), in the same ring.
In order to make easier further tests on the PRP results, the exponents p
for which (2^p+2^((p+1)/2)+1)/5 or (2^p-2^((p+1)/2)+1)/5 is PRP are
registered in additional result files named "gqplus.res" and "gqminus.res",
respectively.

Moreover, the candidates are now prefactored as described above.

-The InterimResidues and InterimFiles options have been added, and work
exactly as in Prime95/Mprime.

-The rounding error recovery has been improved.

-When an error opening output file occurs, the current result is now saved
in the Log file.

-A wrong order bug on Lucky plus and Lucky minus tests has been fixed.

Version 3.6 :

-The program is now directly linked with the George Woltman's gwnums
library archive, and the source does no more contain included gwnums
C code files.

-It has been rather much tested on no SSE2 machines, so the "Beta"
has been removed.

-The k*b^n+/-1 numbers, were the base b is a power of two, are now
converted into base two numbers before beeing processed, instead of
doing a PRP test on them.

-An iteration in the "Computing U0" loop is two times more time-consuming
than a computing power one, so it is better to make a previous PRP test
before a LLR one, as soon as the k multiplier's bit length reaches 10% of
the bit length of the number to test. That is that is done in this version.
On the other hand, the Proth test is slightly faster than the PRP one, so,
it is done directly.

-To avoid any confusion, the output line for composite numbers now shows
of which algorithm the residue is the result :

RES64: "xxxxxxxxxxxxxxxx"	(PRP test)
LLR RES64: "xxxxxxxxxxxxxxxx"	(LLR test)
Proth RES64: "xxxxxxxxxxxxxxxx"	(Proth test)

Version 3.5 Beta :

-As written above, this version can work on all PC's and compatibles, but,
indeed, the new x87 code has not still be very much tested, which explains
the "Beta", which has to be taken in account especially by non-P4 users.

Version 3.4 :

-The program can now prove the primality of (2^n+/-1)-2 near-square
(ex - Carol/Kynea) numbers !
These numbers are of the k*2^n-1 form, with k < 2^n, so the LLR algorithm
can be used, but k is large, so the big k mode is the most often necessary.
For doing that, it accepts the output files generated by the Mark
Rodenkirch's Multisieve program. After a request by Mark, it accepts these
three ABC formats in the first line :

ABC (2^$a$b)^2-2	// (2^n+/-1)-2 near-square numbers
ABC $a*$b^$a$c		// Cullen/Woodall numbers
ABC $a*2^Sb+1		// Output from Jim Fougeron's FermFact sieve

-I/O improvement :

By default, the ouput lresult file will now contain only one line by test
(the prime/not prime one). The user can choose to have more informations
written by inserting a "Verbose=1" line in the .ini file.

-Appearance improvement of the GUI :

The main window is now opened when double-clicking on the LLR icon.
The program now exits when clicking on the close button of the title bar.

Version 3.3 :

-This new version can now also prove the primality of k*2^n+1 numbers !
Thanks to the new George Woltman's "Gwnums" and assembler code, to
implement the Proth theorem algorithm in this program was almost
straightforward. Moreover, the performances of these tests seem to be
even better than those of the Lucas-Lehmer-Riesel ones.
I tested successfully this new feature on all the verified Proth primes
extracted from the Chris Caldwell's database (more than 38,000 primes !).

-A bug, recently discovered, affected some tests the program choosed to
do with a generic FFT, it has been fixed here by doing always careful 
squarings for the last 25 iterations.

Version 3.2 :

-Another bug, which affected the even k's numbers has been detected in
version 3.1, and is now fixed (apparently, there were no such numbers
in my copy of Chris Caldwell's database, I was unlucky...).

-After a suggestion from Paul Jobling, I modified slightly the input
interface to improve the processing of some NewPgen output files :

For Twin/SG, Twin/CC, "Lucky Plus" and "Lucky Minus" files, if the first
expression is prp or prime, the program tests (using LLR, Proth or PRP
algorithm) all the remaining expressions, and if at least one of these
remaining expressions is prp or prime, it writes the result in the .res
output file.

Version 3.1 :

-Two serious bugs have been fixed, one in LLR code, and the second
in Gwnums code.

-I successfully tested the updated program on all the verified k*2^n-1
primes in Chris Caldwell's database.

-If you need to redo some tests vith the new version, add this line in
the .ini file :

VerifyUpToLine=<number> (if you don't know up to what, set <number>
to a large value...).

With this option, the program detects what are the numbers affected by
the LLR code bug, and redo the tests only on them (but the k's larger
than 22 bits are all tested again...).

-If k is larger than 2^n, or if the form of the number(as indicated in
the first input line) is not k*2^n+/-1, the program does a PRP test,
and behaves exactly as PRP3 (so, please, give credit to George Woltman's
PRP for the results obtained in this case).

************************************************************************

I hope this new version will not have serious problems, and will yield
many new prime discoveries.


Jean Penné 

E-mail : jpenne@free.fr

09/11/2007

-> Gwnums Library's Legal stuff

Copyright (c) 1996-2005, George Woltman

All rights reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are 
met: 

(1) Redistributing source code must contain this copyright notice,
limitations, and disclaimer. 
(2) If this software is used to find Mersenne Prime numbers, then
GIMPS will be considered the discoverer of any prime numbers found
and the prize rules at http://mersenne.org/prize.htm will apply.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE. 
  