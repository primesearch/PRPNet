# specify the compiler
CC=g++

CPPFLAGS=-c -O2 -m64 -Wall

ifeq ($(OS),Windows_NT)
   LFLAGS_CLIENT=-lpthread -lstdc++
   LFLAGS_SERVER=-lpthread -liodbc -lstdc++
else
   UNAME_S := $(shell uname -s)

   ifeq ($(UNAME_S),Darwin)
      LFLAGS_CLIENT=-lpthread
      LFLAGS_SERVER=-framework Carbon -lpthread -liodbc -lstdc++
      if test -f /usr/lib/libxnet.so; then LIBS=-lxnet $LIBS
   else
      LFLAGS_CLIENT=-pthread -lpthread -lstdc++
      LFLAGS_SERVER=-pthread -lpthread -lodbc -lstdc++
   endif
endif

HTMLOBJECTS=HTMLGeneratorFactory.o $(patsubst %.cpp,%.o,$(wildcard *HTMLGenerator.cpp)) $(patsubst %.cpp,%.o,$(wildcard *HTML.cpp))
STATSOBJECTS=StatsUpdaterFactory.o $(patsubst %.cpp,%.o,$(wildcard *StatsUpdater.cpp))
MAILOBJECTS=MailFactory.o $(patsubst %.cpp,%.o,$(wildcard *Mail.cpp))
HELPEROBJECTS=ServerHelperFactory.o $(patsubst %.cpp,%.o,$(wildcard *ServerHelper.cpp))
SOCKETOBJECTS=Socket.o ServerSocket.o MailSocket.o HelperSocket.o
DBOBJECTS=DBInterface.o SQLStatement.o
THREADOBJECTS=SharedMemoryItem.o ServerThread.o CandidateTestResult.o KeepAliveThread.o \
	$(patsubst %.cpp,%.o,$(wildcard *WorkSender.cpp)) $(patsubst %.cpp,%.o,$(wildcard *WorkReceiver.cpp)) \
	$(patsubst %.cpp,%.o,$(wildcard *HelperThread.cpp))
PROGRAMOBJECTS=TestingProgramFactory.o $(patsubst %.cpp,%.o,$(wildcard *Program.cpp))
WORKEROBJECTS=ServerHandler.o $(patsubst %.cpp,%.o,$(wildcard *Worker.cpp))
WORKUNITOBJECTS=WorkUnitTestFactory.o $(patsubst %.cpp,%.o,$(wildcard *WorkUnitTest.cpp))


ADMINOBJECTS=prpadmin.o Log.o Socket.o ClientSocket.o ABCParser.o SharedMemoryItem.o
CLIENTOBJECTS=prpclient.o Log.o Socket.o ClientSocket.o SharedMemoryItem.o \
   $(PROGRAMOBJECTS) $(WORKUNITOBJECTS) $(WORKEROBJECTS)
SERVEROBJECTS=prpserver.o Log.o ABCParser.o LengthCalculator.o \
   $(DBOBJECTS) $(THREADOBJECTS) $(HTMLOBJECTS) $(STATSOBJECTS) $(SOCKETOBJECTS) $(HELPEROBJECTS) $(MAILOBJECTS)

all:  prpclient prpserver prpadmin

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
	$(CC) -o $@ $^ $(LFLAGS_CLIENT)

prpclient: $(CLIENTOBJECTS)
	$(CC) -o $@ $^ $(LFLAGS_CLIENT)

prpserver: $(SERVEROBJECTS)
	$(CC) -o $@ $^ $(LFLAGS_SERVER)

clean_objs:
	rm -f $(ADMINOBJECTS) $(CLIENTOBJECTS) $(SERVEROBJECTS) *.log

clean:
	rm -f *.o prpclient prpserver prpadmin
