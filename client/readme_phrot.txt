		Main Features of Phrot Version 0.72

Phrot is a program that can perform probable prime (PRP) tests on numbers
of the form b^n+1 (GFN), b^n-1, k*b^n+1 and k*b^n-1.  It can also perform a Proth 
primality test on Proth numbers, which are of the form k*2^n+1.  

Phrot requires the use of the YEAFFT library.  This is a platform independent
library, meaning that it can be built to run on many different CPUs and OSs.
Notes on building and linking phrot with YEAFFT are below.  Because YEAFFT
is platform independent, so is phrot.

Many thanks to Phil Carmody, who originally wrote phrot and allowed me to
help him beta test it.  Since version 0.50, I have taken over the development
of this software.

On x86, phrot should be faster than LLR for some small bases that are not
powers of 2, although it will be necessary for each individual to do some 
testing to see which program is faster.

If any new Top 5000 primes are found with phrot, please give credit to the 3 P's
program http://primes.utm.edu/bios/page.php?id=953 on the Prime Pages.

************************* How to Use Phrot ************************************

To perform a PRP test on one or more numbers, the number can be passed to phrot 
directly on the command line, using the -q command line argument or through a 
file, e.g. "phrot.p3 -q 4567*68^12356-1".

Phrot also supports many ABC file formats and some NewPGen formats, e.g.
"phrot.p3 abc.in".

Some of the other command line options are:
   -a<x> (or -b=<x>) where <x> is the level of paranoia

      By setting the level of paranoia (similar to what PFGW does), phrot
      will use the next larger FFT size usable for the number.  This will
      increase the amount of time for the test, but will be decrease the
      possibility of hitting the max error threshhold

   -b<x> (or -b=<x>) where <x> is the witness

      By default, phrot will choose the witness of 3 so that its residues
      match those of LLR.  The one exception to this is when the number to
      be tested is a base 2 number.  In that case phrot will perform a Proth
      primality test.  Unless overridden withthis switch, phrot will search
      for an appropriate witness and perform a Proth test.
      
      When overriding the witness on a Proth number, phrot will verify if that 
      witness can be used for a Proth test.  If so, then it will perform a 
      Proth test with that witness, otherwise it will perform a PRP test with 
      that witness.  For example, 6849*2^3306+1 is prime, but only the 
      witnesses 5 and 17 (below 20) can be used for a Proth test.  Witnesses 
      3, 7, 11, 13, and 19 can only be used as witnesses for a PRP Test.

   -d If the number is composite, phrot will display the values of the last
      four limbs used by the FFT.  This is useful only for debugging.

   -e can be used to force error checking.  Error checking is enabled by default

   -n can be used to disable error checking.  Phrot is noticably faster without
      error checking (10% to 20%), but as many as 10% of the PRP tests will
      fail due to an overflow if error checking is disabled.

   -o will tell phrot to append the results of the test to both the terminal
      and a file called results.out

   -s will tell phrot to stop when the first PRP or prime is found in the
      input file.

   -t This is used with PRPNet to reduce the amount of ouput that phrot
      echoes to the console.

************************* Supported ABC File Formats **************************

ABC files are generated from programs such as srsieve and gcwsieve.  These are
currently the fastest sieving programs available and can be compiled and run on
any CPU architecture.  You can download sources and executables for these programs
from Geoffrey Reynold's website at http://www.geocities.com/g_w_reynolds.

Cullen/Woodall form from gcwsieve, fixed base:
	ABC $a*%d^$a+1
	ABC $a*%d^$a-1

Cullen/Woodall form from MultiSieve, variable base:
	ABC $a*%d^$a$c
	ABC $a*$b^$a+1
	ABC $a*$b^$a-1

Fixed k, variable base and n:
	%d*$a^$b+1
	%d*$a^$b-1
	%d*$a^$b$c

Fixed base, variable k and n:
	$a*%d^$b+1
	$a*%d^$b-1
	$a*%d^$b$c

Fixed n, variable k and base:
	$a*$b^%d+1
	$a*$b^%d-1
	$a*$b^%d$c

Variable k, base, and n:
	$a*$b^$c+1
	$a*$b^$c-1
	$a*$b^$c$d

************************* Miscellaneous Notes *********************************

Phrot will checkpoint every 4096 iterations.  This means that it can be
terminated with a ^C and restarted without losing intermediate results.

All phrot residues are compatible with LLR residues, except when overridden
on the command line and for numbers of the form k*2^n-1 as phrot does not
have the ability to perform a Lucas-Lehmer-Riesel (LLR) test on numbers of
that form.

************************* Notes on Building YEAFFT ****************************

When configuring glucas, use the command:

  ./configure --disable-ysse2

This will tell glucas to disable SSE2 (Y_USE_SSE2).  Phrot does not work with
SSE2 extensions.  This might be resolved in the future.

You can also configure YEAFFT with -enable-ydefines_file.  It is not required,
but it will display the YEAFFT compile options when YEAFFT is compiled.  Phrot
will automatically determine if this option was used and build accordingly.

After "making" glucas, go to the src directory and use this command:

   ar rc libyeafft.a [drty]*.o

The library it creates will be linked with phrot.

************************* Notes on Building Phrot *****************************

You must use the same settings for building phrot as were used to build the
yeafft library.  This is why phrot uses the compiler options created by glucas.
If there is a mismatch, phrot will compile, but test results will be invalid.

In the phrot makefile, set the ARCH to the correct value.  Phrot should work
on other CPU families.  It would just be a matter of adding a few lines to the
makefile.  If it is a RISC architecture, (PowerPC is RISC whereas x86 is CISC),
it most likely has at least 32 registers.  If so, then you should consider
modifying phrot.c so that UNROLLED_MR is defined.  I would recommend testing
both methods and report to me if UNROLLED_MR is faster.  If so I will change
the source and re-distribute phrot.

************************* Version History *************************************

Phrot 0.72 - January 22, 2009
 1) Added support for PRP testing of b^n+/-1 forms, which includes GFNs.  Note
    that this is not as fast as genefer, but can handle higher bases.  Residues
    from phrot match those from PFGW.

Phrot 0.71 - August 30, 2009
 1) Removed default for always error checking.  Phrot now checks the first 30
    and last 30 iterations of all tests as well as every 64th iteration of each
    test.  With this change, all tests in the test suite now produce correct
    results, at least on PPC and Core 2 Duo.
 2) Phrot now keeps track of total time spent on a test, so that if stopped and
    restarted, the time for the test will now represent the total time spent
    on a test, not just the time since phrot was started.

Phrot 0.70 - April 26, 2009
 1) Allow b and n to be unsigned, but limit b to 5 million.  Even with such high
    b, some tests will encounter the FFT rouding error.
 2) Limit k to 2^54 on x86 and 2^44 on non-x86.  It is possible that other large
    k will cause problems, but testing so far has not indicated that this change
    is safe are larger k do seem to test without a problem.

Phrot 0.69 - April 23, 2009 (not released)
 1) Default to always error checking as phrot can exhibit rounding errors
    for all bases.
 2) Added -n switch to disable error checking with a caveat (see above).
    Modified use of -a switch.
 3) Added a test suite of PRP numbers to be tested with phrot release.

Phrot 0.68 - March 1, 2009
 1) Added -t switch.  By using this switch, phrot will only output the status
    of the current test and its results.

Phrot 0.67 - February 16, 2009
 1) Fixed a bug that where phrot did not check for an FPU overflow, thus
    giving results that were invalid.
 2) Restored automatic error checking when b or k is a power of 2.  It is 
    extremely rare than an overflow occurs under these conditions.  The only
    known case where this happens is 2*3^229712-1.  There are undoubtably
    others, but until the root cause is found, this additional checking
    will remain.

Phrot 0.66 - January 27, 2009
 1) First build with MinGW (thus no requirement for CygWin).
 2) Add code so that MinGW builds do not use ANSI escape characters.
 3) Removed limit on max base limit as testing implies it is safe.
 4) Modified makefile to work with other OSs/CPUs.
 5) Updated readme with additional notes on building phrot.

Phrot 0.65 - January 12, 2009
 1) Removed requirements to modify any glucas source code before building
    phrot.
 2) Added more documentation to this readme that describe how glucas
    needs to be built before linking with phrot.
 3) Fixed some ABC file format strings so that those files could be processed.
 4) Start phrot with below normal authority so that it doesn't suck all of 
    the CPU on a single-core/single-CPU computer.
 5) Removed unused headers.

Phrot 0.64 - January 8, 2009
 1) Fixed a bug where the incorrect residue was calculated for Proth
    when the candidate is composite.
 2) Fixed the printing of the residue for MinGW.
    
Phrot 0.63 - January 4, 2009
 1) Include this file in the distribution.
 2) Modified makefile and source to remove reliance on sources 
    not found in distribution of phrot or in YEAFFT.
 3) Raised max base limit to 1500000 as testing implies that it is safe.

Phrot 0.62 - January 2, 2009
 1) Address a gcc bug (on PPC only) in getResidue() that
    caused loop to terminate on second iteration, thus producing
    an incorrect residue.  Oddly this only seems to happen for
    bases that are powers of 2.
 2) Can now perform a Proth primality test for bases that are
    powers of 2.
 3) Added dynamic selection of witness (if not specified on the
    command line) so that tests of Proth numbers can be primality
    tests instead of PRP tests.  Phrot should now select the same 
    witness that LLR would select so that the residues match.
 4) Add 1 to the residue of Proth primality tests so that they
    are compatible with LLR residues.
 5) Fixed issue with variable base Cullen/Woodall ABC files.
 6) Removed automatic turn on of error checking when the base
    is a power of 2 as testing has shown that the problem was
    resolved in version 0.52.
 7) Limit the max base for a test to 2^20 (1048576) because
    higher values are unlikely to have valid results.

Phrot 0.61
 1) Fixed restarting from phrot.ini file as logic as accidentally
    removed but not replaced in 0.60.
 2) Fixed Cullen/Woodall checks since sscanf returns as soon
    as variables are scanned.

Phrot 0.60
 1) Renamed source as phrot.c
 2) Removed support for MODULAR_REDUCE_2 and MODULAR_REDUCE_3 as they
    weren't used.  This code can still be found in glprov.c, which
    should be included with the distribution of phrot.
 3) Removed old history from phrot.c
 4) Support many more ABC file formats
 5) Support some NewPGen file format.
 6) Default to -b=3 to always output LLR compatible residue.
 7) Added "-d" option.  Without it, phrot will not output lowest
    limbs after PRP test.
 8) Add "make clean" to makefile
 9) Added a few new messages to catch issues that prevent phrot
    from performing a PRP test

Phrot 0.53 (not released)
 1) Added "-e" option to force error checking for all tests.  Without -e
    phrot will only do error checking if k or b is a power of 2.  This
    effectively removes the need for the error checking build of phrot.

Phrot 0.52
 1) Added phrot.ckpt so that tests can be stopped and restarted
 2) Added "-s" option to stop when PRP is found
 3) Fixed an integer overflow in doTest() that would cause phrot
    to choose a k/b combination too high for the PRP test, thus
    leading to an invalid result without triggering maxerr.

Phrot 0.51
 1) Unrolled modular reduction code for MODULAR_REDUCE_1 (see UNROLLED_MR)
 2) Added phrot.ini so tests can resume from an input file
 3) Added -o option to write all test results to results.out
 4) Fixed "+-1" issue in output strings

*********************** The future of Phrot *********************************

Perform a BLS primality test on numbers of the form k*b^n+1. (easy)

Perform an LLR primality test on numbers of the form k*2^n-1. (very hard)

Perform a BLS primality test on numbers of the form k*b^n-1. (very hard)

Perform a PRP test on generic forms, such as primorials. (very hard)

*****************************************************************************

Mark Rodenkirch

E-mail : rogue@wi.rr.com

April 23, 2009

Phrot (c) 2004,2005,2006 Phil Carmody,
(c) 2008,2009 Mark Rodenkirch
Distributed under the (intention of satisfying the) terms of the
GNU General Public License.

