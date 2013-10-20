# specify the compiler
CPP=gcc

CPPFLAGS=-c -g -Wall

# prpclient/prpadmin link flags
LFLAGS_CLIENT=-lpthread -lstdc++ -o

# use this on most Unix systems for prpserver
LFLAGS_SERVER=-lpthread -liodbc -lstdc++ -o

# use this on MacPPC for prpserver
#LFLAGS=-framework Carbon -lpthread -liodbc -o

if test -f /usr/lib/libxnet.so; then LIBS=-lxnet $LIBS

HTMLOBJECTS=HTMLGeneratorFactory.o PrimeHTMLGenerator.o CullenWoodallHTML.o \
	SierpinskiRieselHTML.o FixedBKCHTML.o FixedBNCHTML.o GFNHTML.o \
	PrimorialHTML.o FactorialHTML.o SophieGermainHTML.o WWWWHTMLGenerator.o \
   HTMLGenerator.o
STATSOBJECTS=StatsUpdaterFactory.o StatsUpdater.o FactorialStatsUpdater.o \
	PrimorialStatsUpdater.o FixedBKCStatsUpdater.o FixedBNCStatsUpdater.o \
	GFNStatsUpdater.o SierpinskiRieselStatsUpdater.o CullenWoodallStatsUpdater.o \
	SophieGermainStatsUpdater.o PrimeStatsUpdater.o WWWWStatsUpdater.o
MAILOBJECTS=MailFactory.o Mail.o PrimeMail.o WWWWMail.o
HELPEROBJECTS=ServerHelperFactory.o PrimeServerHelper.o WWWWServerHelper.o
SOCKETOBJECTS=Socket.o ServerSocket.o MailSocket.o HelperSocket.o
DBOBJECTS=DBInterface.o SQLStatement.o
THREADOBJECTS=SharedMemoryItem.o ServerThread.o HelperThread.o WorkSender.o WorkReceiver.o \
	PrimeWorkSender.o WWWWWorkSender.o PrimeWorkReceiver.o WWWWWorkReceiver.o \
	PrimeHelperThread.o WWWWHelperThread.o CandidateTestResult.o KeepAliveThread.o
PROGRAMOBJECTS=TestingProgram.o TestingProgramFactory.o PFGWProgram.o LLRProgram.o \
	PhrotProgram.o GeneferProgram.o WWWWProgram.o
WORKEROBJECTS=ServerHandler.o Worker.o PrimeWorker.o WWWWWorker.o
WORKUNITOBJECTS=WorkUnitTestFactory.o WorkUnitTest.o MainWorkUnitTest.o \
	TwinWorkUnitTest.o SophieGermainWorkUnitTest.o WWWWWorkUnitTest.o PrimeWorkUnitTest.o

all:  prpclient prpserver

prpclient.o: prpclient.cpp defs.h Log.h Socket.h Worker.h
	$(CPP) $(CPPFLAGS) prpclient.cpp

prpserver.o: prpserver.cpp defs.h server.h Log.h Socket.h Mail.h
	$(CPP) $(CPPFLAGS) prpserver.cpp

HTMLGenerator.o: HTMLGenerator.cpp defs.h
	$(CPP) $(CPPFLAGS) HTMLGenerator.cpp

%HTML.o: %HTML.cpp %HTML.h %HTMLGenerator.h Socket.h Candidate.h
	$(CPP) $(CPPFLAGS) $<

%Program.o: %Program.cpp %Program.h TestingProgram.h
	$(CPP) $(CPPFLAGS) $<

%WorkUnitTest.o: %WorkUnitTest.cpp %WorkUnitTest.h WorkUnitTest.h
	$(CPP) $(CPPFLAGS) $<

%.o: %.cpp %.h Log.h Socket.h
	$(CPP) $(CPPFLAGS) $<

prpadmin: prpadmin.o Log.o Socket.o ClientSocket.o ABCParser.o SharedMemoryItem.o
	$(CPP) $(LFLAGS_CLIENT) prpadmin prpadmin.o Log.o Socket.o ClientSocket.o ABCParser.o SharedMemoryItem.o

prpclient: prpclient.o Log.o Socket.o ClientSocket.o SharedMemoryItem.o \
	$(PROGRAMOBJECTS) $(WORKUNITOBJECTS) $(WORKEROBJECTS)
	$(CPP) $(LFLAGS_CLIENT) prpclient prpclient.o Log.o Socket.o ClientSocket.o  SharedMemoryItem.o  \
	$(WORKUNITOBJECTS) $(PROGRAMOBJECTS) $(WORKEROBJECTS)

prpserver: prpserver.o Log.o ABCParser.o LengthCalculator.o \
	$(DBOBJECTS) $(THREADOBJECTS) $(HTMLOBJECTS) $(STATSOBJECTS) $(SOCKETOBJECTS) $(HELPEROBJECTS) $(MAILOBJECTS)
	$(CPP) $(LFLAGS_SERVER) prpserver prpserver.o Log.o ABCParser.o LengthCalculator.o \
	$(DBOBJECTS) $(THREADOBJECTS) $(HTMLOBJECTS) $(STATSOBJECTS) $(SOCKETOBJECTS) $(HELPEROBJECTS) $(MAILOBJECTS)

clean:
	rm *.o prpclient prpserver prpadmin
