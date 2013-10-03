These are the minimal steps to get the PRPNet server running.

1)  Build the prpserver executable by using "make" or by using the prpnet.sln
    project workspace for Visual Studio.  Note that if building on *nix, that
    changes to the makefile might be necessary.

2)  Modify the prpserver.ini and modify the following:
       email=       Your email address.  E-mail will be sent to this address
                    when a PRP is found.
       port=        The port that the server will listen to waiting for connections
                    from clients or slave servers.
       servertype=  The servertype is important for reasons stated below.

    Most of the remaining values in prpclient.ini have defaults.  There is
    a description for each in that file that explains what they are used for.

3)  Create a MySQL/PostgreSQL database:

    a.  If necessary d/l MySQL from http://www.mysql.com or PostgreSQL from
        http://www.postgresql.org/.
    b.  Also d/l and install MySQL Connector/ODBC or psqlODBC for PostgreSQL.
    c.  On Linux, you might need to install iodbc and myodbc libraries.
    d.  On Linux, you might need to add an entry to the odbcinst.ini file
        for the MySQL/PostgreSQL drivers.
    e.  For mysql create a new database using the mysqladmin tool.  For
        PostgreSQL you can use the pgAdmin tool or command line utilities.      
    f.  Connect to your new database.
    g.  Create the PRPNet tables using the create_tables_mysql.sql or
        create_tables_postgre.sql script
        found in the source directory.
    h.  Load the database with an ABC file via the prpadmin tool.

4)  Modify the datbase.ini file to point to your database.  Note the PRPNet can
    connect directly through the ODBC driver or a DSN.

5)  Edit the prpserver.delay file.

Questions:

How is servertype used?
    The setting of servertype is important for a number of things, including:
       a) how stats are generated
       b) whether or not the client can perform work managed by the server
       c) it is very useful for the various Sierpinski/Riesel projects
