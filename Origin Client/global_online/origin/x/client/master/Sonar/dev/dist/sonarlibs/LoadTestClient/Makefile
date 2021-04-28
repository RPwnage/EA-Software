PROGRAM=loadtestclient
CPP=g++
LIBS=-lpthread -lm -lrt -lstdc++ -lssl -lcrypto
OBJS= \
    main.o \
    SwarmConnection.o \
    Swarm.o \
    ../TestUtils/TokenSigner.o \
    ../SonarCommon/Common.o \
    ../SonarConnection/BaseConnection.o \
    ../SonarConnection/Connection.o \
    ../SonarConnection/Message.o \
    ../SonarCommon/Token.o \
    ../SonarClient/win32/providers.o \
    ../jlib/Thread.o

CPPFLAGS=-I../ -std=c++11 -D_LIB -DNDEBUG -O3 -DSONAR_LOGGING_ENABLED=0 -DENABLE_CONNECTION_TRACE=0
all : loadtestclient

debug : CPPFLAGS += -O0 -ggdb -D_DEBUG
debug : loadtestclient

loadtestclient : $(OBJS)
	$(CPP) $(LINKARGS) $(OBJS) $(LIBS) -o ./$(PROGRAM)

%.o:	$(SOURCE)/%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o ./$@

clean:
	rm -rf $(OBJS)
	rm -rf $(PROGRAM)

