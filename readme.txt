Welcome to PRPNet Client/Server Version 5.3.2 (July 2014)

What is PRPNet?
    PRPNet is a distributed Client/Server application that can used to manage
    and perform probable prime (PRP) tests on a list of candidate numbers.

What types of numbers does PRPNet support?
    At this time PRPNet only supports numbers in the form k*b^n+1, k*b^n-1,
    primorials (n#+1, n#-1), factorials (n!+1, n!-1) and GFNs (b^(2^n)+1).

What is the PRPNet Client?
    The PRPNet Client is a program that uses genefer, PFGW, LLR, or phrot to
    perform a PRP test on a number.  As a client, this program communicates
    with a centralized server (see "What is the PRPNet Server?" below) which
    doles out work to the client.  The client will run a PRP test and report
    back to the server the result of the test.

What do I need to do to get the client/server running?
    Specific instructions for configuring and running the client or server can
    be found in corresponding directories.

What is the PRPNet Server?
    The PRPNet Server is a server program that manages a list of numbers for which
    the primality is not known.  When a client (see "What is the PRPNet Client?" 
    above) requests work, the server will use a set of predifined rules to determine 
    which candidate number needs a PRP test.  It will then responds to that client
    with the candidate.  When the test is done, it will log the results of the test
    (PRP, prime, or composite).  The server has the ability to perform double-checks
    to verify that tests were run on stable hardware. 

What is the PRPNet Administration Tool?
    The administration tool is used by the adminstrator of the PRPNet server to
    modify server settings while it is running.  At this time, the only modifications
    supported by this tool are the ability to add new candidates from an ABC file
    and remove candidates based upon factors found.  Neither the PRP Admin tool
    nor the server will validate factors applied through this process.

What else do I need?
    You will also need MySQL or PostgreSQL for the PRPNet server.  Both are open
    source relational database that have support for transactions.  There are
    table creation scripts for each database in the source directory.

What else must I do to get started?
    You will need to use the PRPNet Administration Tool to load candidate numbers
    into the server.  Once loaded, clients can connect and get work.

What ABC file formats are supported?
    The same formats that are supported by LLR and phrot, but also the formats
    output by fsieve and psieve, which would be used by PFGW.

What does a factor file look like?
    Factors are expected to be in the form:
       <factor> | <candidate>
    which is the output of files found by any of Geoff Reynold's sieving programs.

How do I compile/run these programs?
    If you are running Windows, then you can use the provided executables.  They
    were compiled with Visual Studio 2008.  For other environments, these programs can
    be easily compiled with the supplied makefile.  The default makefile can be 
    used with Cygwin under Windows (with the MinGW g++ compiler) and with most unix
    boxes.  There is also a project solution (prpnet.sln) for use with Visual Studio.
    Once the executable you need is built, you will need to modify the cfg file in the 
    appropriate directory.  Please refer to that cfg file to understand how to set 
    the various configuration parameters.

How do stats work?
    To access stats, just access the server on its defined port, but append the
    name of a webpage you want to see at the end of that.  At this time the server 
    supports three stats pages:
       server_stats.html provides a complete web page of the current server.
       user_stats.html provides a complete web page of stats rolled up by user
       ordered by score.
       user_primes.html provides a list of primes found by user.
       pending_tests.html provides a list of tests sent to candidates that
       have not been returned to the server.

Can I add support for more pages?
    Absolutely.  I certainly would appreciate any input for the future development of
    PRPNet.  The current page formats are not set in stone and I am open to making
    improvements.

Can the supported pages be more dynamic, such as supporting XSLT?
    I've thought about it, but that would require a lot of documentation.  If someone
    wants to begin such development I would support it.  Of note, it must not require
    the use of licensed software or software that is platform dependent, i.e. software
    that only runs on Windows.

How are user stats computed?
    The score for each completed test is computed as such:
       (decimal length of the candidate / 10000) ^ 2

Why did you use that method for user stats?
    For simplicity.  There is no need to know the CPU the software is run on or how
    long it takes to perform a test.  If one user tests a number that is twice as
    long as another, then they get four times as many points because it takes four
    times as long to test.  That presumes that both tests are run on the same hardware
    with the same software.  I divide by 10000 to keep the numbers relatively small.

Can I change how user stats are computed?
    Certainly.  You need to modify UpdateUserStats() in StatsUpdater.cpp.  The caveat
    is that you will need to keep track of any changes to the source as new releases
    of PRPNet come out.  I am open to updating my source, but only if someone can
    provide a better formula that is easy to implement and more equitable.

Who are you and how do I contact you if I find or bug or have any questions?
    My name is Mark Rodenkirch.  I can be contacted at rogue@wi.rr.com.

