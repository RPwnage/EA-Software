PROGRAM=sonarsrvx
CPP=g++
LIBS=-lpthread -lm -lstdc++ -luuid -lssl -lrt -lcrypto
OBJS=\
    AbstractClient.o\
    AbstractOutboundClient.o\
    AudioPayload.o\
    BackendClient.o\
    ByteBuffer.o\
    Channel.o\
    ClientConnection.o\
    HealthCheck.o\
    main.o\
    Operator.o\
    Server.o\
    TcpSocket.o\
    UUIDManager.o\
    ../SonarCommon/Common.o\
    ../SonarConnection/BaseConnection.o\
    ../SonarConnection/Connection.o\
    ../SonarConnection/Message.o\
    ../SonarCommon/Token.o\
    ../jlib/Thread.o
    
CPPFLAGS=-I../
all : release

debug32 : LINKARGS += -m32
debug32 : CPPFLAGS += -m32
debug32 : debug

release32 : LINKARGS += -m32
release32 : CPPFLAGS += -m32
release32 : release

debug : CPPFLAGS += -O0 -ggdb -std=c++11 -D_DEBUG
debug : sonarsrv

release : CPPFLAGS += -O3 -std=c++11 -DNDEBUG
release : sonarsrv

sonarsrv : $(OBJS)
	$(CPP) $(LINKARGS) $(OBJS) $(LIBS) -o ./$(PROGRAM)

%.o:	$(SOURCE)/%.cpp
	$(CPP) $(CPPFLAGS) -c $< -o ./$@

clean:
	rm -rf $(OBJS)
	rm -rf $(PROGRAM)

