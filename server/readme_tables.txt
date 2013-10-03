This document describes the tables in the PRPNet database and how they are used as well
as some descriptive information regarding the columns in them.  If you choose to write
any SQL against the database, the information here will be extremely useful to you.


First a list of the tables:
   Candidate           - The main table holding all numbers to be tested.

   CandidateTest       - A child table of Candidate that holds information about tests
                         that have been sent to clients.

   CandidateTestResult - A child table of CandidateTest that holds test results.  Note
                         that for most servertypes the rows here are 1:1 with CandidateTest.
                         For Twin and Sophie-Germain prime searches, there could be multiple
                         records in this table for each CandidateTest.

   CandidateGFNDivisor - A child table of Candidate that lists the GFN numbers that can
                         be divided by the Candidate or one of its children.  Note that
                         the Twin and Sophie-Germain prime searches can search for GFN
                         divisors for more than just the Candidate.

   GeneferROE          - A child table of Candidate that lists the versions of genefer
                         that experienced round-off errors during their PRP test.  The
                         server will use this to send PRP tests to clients that can
                         test GFN candidates with other versions of genefer or other
                         software.

   CandidateGroupStats - This table rolls up Candidate information into groups, which
                         is dependent upon the server type.

   UserStats           - Stats by user, such as number of completed tests, etc.

   UserPrimes          - A list of primes and PRPs found by user.
   

Column Notes:
   MainTestResult and Test Result are evaluated as follows:
      0 - candidate is composite
      1 - candidate is PRP
      2 - candidate is prime

   WhichTest indicates the type of secondary test done for the candidate:
      Main   - the candidate itself
      Twin   - the twin of the candidate
      SG n-1 - The Sophie-Germain number with n = n-1
      SG n+1 - the Sohpie-Germain number with n = n+1
