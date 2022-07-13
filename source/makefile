# specify the compiler
CC=g++

CPPFLAGS=-c -O2 -m64 -Wall

ifeq ($(OS),Windows_NT)
   LFLAGS_CLIENT=-lpthread -lstdc++ -o
   LFLAGS_SERVER=-lpthread -liodbc -lstdc++ -o
else
   UNAME_S := $(shell uname -s)
   
   ifeq ($(UNAME_S),Darwin)
      LFLAGS_CLIENT=-lpthread -lstdc++ -o
      LFLAGS_SERVER=-framework Carbon -lpthread -liodbc -lstdc++ -o
      if test -f /usr/lib/libxnet.so; then LIBS=-lxnet $LIBS
   else
      LFLAGS_CLIENT=-pthread -lpthread -lstdc++ -o
      LFLAGS_SERVER=-pthread -lpthread -liodbc -lstdc++ -o
   endif
endif

HTMLOBJECTS=HTMLGeneratorFactory.o PrimeHTMLGenerator.o CullenWoodallHTML.o \
	SierpinskiRieselHTML.o FixedBKCHTML.o FixedBNCHTML.o GFNHTML.o XYYXHTML.o \
	PrimorialHTML.o FactorialHTML.o MultiFactorialHTML.o SophieGermainHTML.o WWWWHTMLGenerator.o \
   CyclotomicHTML.o GenericHTML.o HTMLGenerator.o CarolKyneaHTML.o WagstaffHTML.o
STATSOBJECTS=StatsUpdaterFactory.o StatsUpdater.o MultiFactorialStatsUpdater.o FactorialStatsUpdater.o \
	PrimorialStatsUpdater.o FixedBKCStatsUpdater.o FixedBNCStatsUpdater.o \
	GFNStatsUpdater.o SierpinskiRieselStatsUpdater.o CullenWoodallStatsUpdater.o \
	SophieGermainStatsUpdater.o PrimeStatsUpdater.o WWWWStatsUpdater.o XYYXStatsUpdater.o \
	CyclotomicStatsUpdater.o GenericStatsUpdater.o CarolKyneaStatsUpdater.o WagstaffStatsUpdater.o
MAILOBJECTS=MailFactory.o Mail.o PrimeMail.o WWWWMail.o
HELPEROBJECTS=ServerHelperFactory.o PrimeServerHelper.o WWWWServerHelper.o
SOCKETOBJECTS=Socket.o ServerSocket.o MailSocket.o HelperSocket.o
DBOBJECTS=DBInterface.o SQLStatement.o
THREADOBJECTS=SharedMemoryItem.o ServerThread.o HelperThread.o WorkSender.o WorkReceiver.o \
	PrimeWorkSender.o WWWWWorkSender.o PrimeWorkReceiver.o WWWWWorkReceiver.o \
	PrimeHelperThread.o WWWWHelperThread.o CandidateTestResult.o KeepAliveThread.o
PROGRAMOBJECTS=TestingProgram.o TestingProgramFactory.o PFGWProgram.o LLRProgram.o \
	PhrotProgram.o GeneferProgram.o WWWWProgram.o CycloProgram.o
WORKEROBJECTS=ServerHandler.o Worker.o PrimeWorker.o WWWWWorker.o
WORKUNITOBJECTS=WorkUnitTestFactory.o WorkUnitTest.o MainWorkUnitTest.o \
	TwinWorkUnitTest.o SophieGermainWorkUnitTest.o WWWWWorkUnitTest.o PrimeWorkUnitTest.o


ADMINOBJECTS=prpadmin.o Log.o Socket.o ClientSocket.o ABCParser.o SharedMemoryItem.o
CLIENTOBJECTS=prpclient.o Log.o Socket.o ClientSocket.o SharedMemoryItem.o \
   $(PROGRAMOBJECTS) $(WORKUNITOBJECTS) $(WORKEROBJECTS)
SERVEROBJECTS=prpserver.o Log.o ABCParser.o LengthCalculator.o \
   $(DBOBJECTS) $(THREADOBJECTS) $(HTMLOBJECTS) $(STATSOBJECTS) $(SOCKETOBJECTS) $(HELPEROBJECTS) $(MAILOBJECTS)

all:  prpclient prpserver

prpclient.o: prpclient.cpp defs.h Log.h Socket.h Worker.h
	$(CC) $(CPPFLAGS) prpclient.cpp

prpserver.o: prpserver.cpp defs.h server.h Log.h Socket.h Mail.h
	$(CC) $(CPPFLAGS) prpserver.cpp

HTMLGenerator.o: HTMLGenerator.cpp defs.h
	$(CC) $(CPPFLAGS) HTMLGenerator.cpp

%HTML.o: %HTML.cpp %HTML.h %HTMLGenerator.h Socket.h Candidate.h
	$(CC) $(CPPFLAGS) $<

%Program.o: %Program.cpp %Program.h TestingProgram.h
	$(CC) $(CPPFLAGS) $<

%WorkUnitTest.o: %WorkUnitTest.cpp %WorkUnitTest.h WorkUnitTest.h
	$(CC) $(CPPFLAGS) $<

%.o: %.cpp %.h Log.h Socket.h
	$(CC) $(CPPFLAGS) $<

prpadmin: $(ADMINOBJECTS)
	$(CC) $(LFLAGS_CLIENT) -o $@ $^

prpclient: $(CLIENTOBJECTS)
	$(CC) $(LFLAGS_CLIENT) -o $@ $^

prpserver: $(SERVEROBJECTS) 
	$(CC) $(LFLAGS_SERVER) -o $@ $^

clean_objs:
	rm -f $(ADMINOBJECTS) $(CLIENTOBJECTS) $(SERVEROBJECTS) *.log
   
clean:
	rm *.o prpclient prpserver prpadmin
