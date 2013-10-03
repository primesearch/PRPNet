These are the minimal steps to get the PRPNet client running.

1)  Build the prpclient executable by using "make" or by using the prpnet.sln
    project workspace for Visual Studio.

2)  Modify the prpclient.ini and modify the following:
       email=       Your email address.  E-mail will be sent to this address
                    when a number is found to be PRP.
       clientid=    This identifies a specific instance of the client
                    running for the specified e-mail.
       server=      This points to an PRPNet server from which the client
                    will get work.  Some known PRPNet servers and their
                    descriptions can be found in prpnet_servers.txt.
       llrexe=      This is the LLR executable that will be used to perform
                    a PRP test.
       geneferexe=  This is the genefer execuatable that will be used to perform
                    a PRP test.  Up to three versions of can be specified, genefx64,
                    genefer, and genefer80.  They run at different speeds, but
                    the faster ones are more error prone.  The client will try
                    to use the fastest one then revert to a slower one if the faster
                    one encounters an error.
       phrotexe=    This is the phrot execuatable that will be used to perform
                    a PRP test.
       pfgwexe=     This is the PFGW executble that will be used to
                    perform a PRP test or test for GFN divisibility.

    If both llrexe and pfgwexe are specified, then llrexe will be used when
    the base is a power of 2 and pfgwexe will be used for all other bases.

    Most of the remaining values in prpclient.ini have defaults.  There is
    a description for each in that file that explains what they are used for.

Questions:

What hardware will these programs run on?
    LLR and PFGW will run on any hardware that has an x86 CPU.  That includes Windows,
    Unix, and MacIntel.  Phrot can be compiled and run on any hardware although it
    has only been tested on x86 and PPC.  Versions of genefer can run on Windows,
    other x86 OSes and PPC.

Where can I get LLR?
    http://jpenne.free.fr/index2.html

    Note: PRPNet only supports the command line version of LLR.

Where can I get phrot?
    The minimal version of phrot to use with PRPNet is 0.72.  Although phrot can be
    built on multiple platforms, the x86 versions of LLR and PFGW are faster.  phrot
    is relegated to non-x86, where it is very competitive speedwise on that hardware.

    http://home.roadrunner.com/~mrodenkirch/phrot_072.zip

Where can I get PFGW?
    Builds of PFGW for Windows, Linux, and OS X can be downloaded from
    http://sourceforge.net/projects/openpfgw/files/.

    Note: PRPNet only supports the command line version of PFGW.

Why does the client use PFGW for primality tests instead of LLR?
    That isn't entirely true.  LLR does primality tests for base 2 numbers and
    phrot does primality tests for Proth numbers (k*2^n+1).  I have resisted
    using LLR 3.8 for primality testing of other bases because the residues
    from LLR 3.8's primality tests do not match those from PFGW or phrot.  Yes,
    it would save time to always use LLR, but throwing away double checks from
    phrot/PFGW would lead to wasted time as triple checks would be needed.