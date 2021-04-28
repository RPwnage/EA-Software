/*H*************************************************************************************/
/*!
    \File    dirtynetwinrt.cpp

    \Description
        Provide a wrapper that translates the WinRT C++ \ CX network interface
        into DirtySock calls. For Windows8 and WindowsPhone8

    \Copyright
        Copyright (c) Electronic Arts 2012.  ALL RIGHTS RESERVED.

    \Notes
        \verbatim
        The WinRT implementation of dirtynet is quite different from the ones used
        on other platforms. WinRT does not provide a C-style berkeley-like socket api.
        The basic socket functionality is fully wrapped by 3 high-level managed C++
        classes: StreamSocket class, DatatgramSocket class and StreamSocketListener
        class. Those classes are providing event-based services: completion of asynchronous
        operations like connection estblishment or inbound data readiness is
        notified through callbacks performed in dedicated threads.

        The public socket api of DirtySDK being aligned with the original berkeley
        socket implementation, the challenge of this port is to convert the
        WinRT events into a polling-based api typically returning the
        equialent of "EWOULDBLOCK" on calls executed against non-blocking sockets.

        Limitations of this WinRT port:
        ------------------------------

        LIMITATION #1: SocketImport() does not support a native socket as input
        On other platforms, SocketImport takes either a SocketT pointer or 
        a native socket as input. With WinRT, SocketImport() only supports importing
        a SocketT pointer. Native sockets being managed references, they don't fit
        with the public api and they cannot be safely pass to unmanaged code.

        LIMITATION #2: SocketInfo('sock') is not supported
        Unmanaged socket references are currently never surfaced up beyond the
        DirtySDK public api.

        LIMITATION #3: Can't assume stream socket is immediately bound after calling
                       SocketBind()
        With WinRT, binding of stream socket is an implicit step of the connection phase.
        Therefore, when SocketBind() is called, we just save the inputs from the calle
        and we use them later when connecting. 

        LIMITATION #4: Connected UDP sockets require SocketInfo('stat') to detect end of
                       async connect
        With WinRT, connection of a stream socket or a "connected" udp socket is an asyn op.
        The user is expected to wait for completion of that async op before proceeding with
        sending/receiving on the socket.

        LIMITATION #5: SocketSendto() on a SOCK_DGRAM socket can return SOCKERR_NONE (0)
        Causes:
        -> The first call to SocketSendto() on a unconnected SOCK_DGRAM socket for a given dst
           addr will always return SOCKERR_NONE (meaning dgram was not sent, try again). This 
           limitation is caused by an internal delay required to obtain a dedicated output
           stream asynchronously for the destination address. Calls to SocketSendto() will 
           start succeeding only when this async op for obtaining the stream is completed.
        -> Back to back calls to SocketSendto() on a given socket will also result in some
           of these calls returning SOCKERR_NONE (meaning dgram was not sent, try again). This 
           limitation is caused by an internal delay required to allow the previous
           DataReader::StoreAsync() call to complete.
        Action Needed:
        --> Client code is expected to deal with SOCKERR_NONE return value appropriately.

        LIMITATION #6: Blocking mode not supported
        Sockets can typically be used in blocking mode or unblocking mode. Currently, only
        the unblocking mode is supported

        LIMITATION #7: Socket type SOCK_RAW not supported.
        \endverbatim

    \Version 18/10/2002 (mclouatre) First Version
*/
/*************************************************************************************H*/

#pragma warning(disable:4350) // behavior change: 'member1' called instead of 'member2'
#pragma warning(disable:4365) // conversion from 'type_1' to 'type_2', signed/unsigned mismatch
#pragma warning(disable:4371) // layout of class may have changed from a previous version of the compiler due to better packing of member
#pragma warning(disable:4571) // Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
#pragma warning(disable:4625) // 'derived class' : copy constructor could not be generated because a base class copy constructor is inaccessible
#pragma warning(disable:4626) // 'derived class' : assignment operator could not be generated because a base class assignment operator is inaccessible
#pragma warning(disable:4946) // reinterpret_cast used between related classes: 'class1' and 'class2'

/*** Include files *********************************************************************/

#include <windows.h>
#include <cstdlib>
#include "DirtySDK/platform.h"   // must be before <ppltasks.h> for char16_t
#include <ppltasks.h>

#include <collection.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtynet.h"

using namespace Concurrency;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

/*** Defines ***************************************************************************/

#define DIRTYNET_DATAWRITERSMAPKEY_MAXLEN       (24)
#define DIRTYNET_DATAWRITERSMAP_MAXSIZE         (32)

#define DIRTYNET_DGRAMINBOUNDQ_MAXSIZE          (8)     // in datagrams

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! private socketlookup structure containing extra data
typedef struct SocketLookupPrivT
{
    HostentT host;                  //!< must come first!
    int32_t iMemGroup;              //!< used to cache module mem group id
    void *pMemGroupUserData;        //!< used to cache user data associated with mem group
    DatagramSocket^ refDgramSock;   //!< managed ref to dgram socket used for resolving addr

} SocketLookupPrivT;

//! dirtysock connection socket structure
struct SocketT
{
    SocketT *pNext;             //!< link to next active
    SocketT *pKill;             //!< link to next killed socket

    int32_t iFamily;            //!< protocol family
    int32_t iType;              //!< protocol type
    int32_t iProto;             //!< protocol ident

    int32_t iLastError;         //!< last socket error

    struct sockaddr localAddr;  //!< local address (obtained by inspecting local interfaces)
    struct sockaddr remoteAddr; //!< remote address
    struct sockaddr bindAddr;   //!< bind address (address saved when SocketBind() is called)
    struct sockaddr recvAddr;   //!< receive address

    int32_t iCallMask;          //!< valid callback events
    uint32_t uCallLast;         //!< last callback tick
    uint32_t uCallIdle;         //!< ticks between idle calls
    void *pCallRef;             //!< reference calback value
    int32_t (*pCallback)(SocketT *pSocket, int32_t iFlags, void *pRef);

    int32_t iRecvErr;           //!< last receive error that occurred
    NetCritT sockCrit;          //!< critical section used to guarantee thread safety between main thread and async op system threads

    Object^ refSock;            //!< WinRT socket managed reference
    Object^ refSockEventCtx;    //!< WinRT socket event context (used for inbound datagram events and inbound stream connection events)
    DataWriter^ refDataWriter;  //!< WinRT outbound stream managed reference
    DataReader^ refDataReader;  //!< WinRT inbound stream managed reference (stream socket only)
    Platform::Collections::Map<String ^, DataWriter ^>^ refDgramDataWritersMap; 
                                //!< a map used to associate a remote peer with a DataWriter
    Platform::Collections::Vector<DatagramSocketMessageReceivedEventArgs ^>^ refInboundDgramEventsVect;
                                //!< a vector used by dgram sockets to store inbound dgram events
    Platform::Collections::Vector<StreamSocket^>^ refStreamConnectVect;
                                //!< a vector used by listening sockets to store connection until SocketAccept() is called

    uint8_t bTypeConnected;     //!< whether the socket is a connected type socket or not (stream socket or udp socket for which SocketConnect() was called)
    int8_t iOpened;             //!< negative=error, zero=not open (connecting), positive=open  (only valid when bTypeConnected is TRUE)
                                //!< (because this is a 1-byte value, we assume it can be read/written in a thread-safe manner by multiple threads)
    uint8_t bSocketBindCalled;  //!< TRUE: SocketBind() was called on this socket.
    uint8_t bConnectAsyncInProgress; //!< TRUE: a socket connect async operation is in progress
    uint8_t bStoreAsyncInProgress; //!< TRUE: a data writer store async operation is in progress

    // note: managed references / objects can't be stored in a union
    union
    {
        //! space used to save SOCK_DGRAM specific info
        struct
        {
            int32_t iRecvStat;              //!< how many queued receive bytes
            uint16_t uVirtualPort;          //!< virtual port, if set
            uint8_t bVirtual;               //!< TRUE: socket is virtual
            uint8_t bBindAsyncInProgress;   //!< TRUE: a socket bind async operation is in progress
            uint8_t bGetOutStreamAsyncInProgress; //!< TRUE: a get output stream async operation is in progress
            int8_t iMaxPendingDgrams;       //!< max number of pending dgrams in inbound queue
            uint8_t _pad[2];
            char aRecvData[SOCKET_MAXUDPRECV]; //!< receive buffer (used with dgram sockets only)
            wchar_t wstrMapKey[DIRTYNET_DATAWRITERSMAPKEY_MAXLEN+1]; //!< map key in used by output stream async operation (only valid when bGetOutStreamAsyncInProgress is TRUE)
         } dgramSpace;

        //! space used to save SOCK_STREAM specific info
        struct
        {
            int8_t iMaxPendingConnections; //!< max number of pending connections supported on a listening socket
            uint8_t bSocketListenCalled;   //!< TRUE: SocketListen() was called on this socket.
            uint8_t bLoadAsyncInProgress;  //!< TRUE: a data reader load async operation is in progress
            uint8_t bBindListenerAsyncInProgress; //!< TRUE: a async bind on listening stream socket is in progress
        } streamSpace;
    } un;
};

//! local state
typedef struct SocketStateT
{
    SocketT *pSockList;                 //!< master socket list
    SocketT *pSockKill;                 //!< list of killed sockets
    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list

    // module memory group
    int32_t iMemGroup;                  //!< module mem group id
    void *pMemGroupUserData;            //!< user data associated with mem group

    uint32_t uAdapterAddress;           //!< local interface used for SocketBind() operations if non-zero
    uint32_t uPacketLoss;               //!< packet loss simulation (debug only)
    int32_t iMaxPacket;                 //!< maximum packet size

    SocketSendCallbackT *pSendCallback; //!< global send callback
    void                *pSendCallref;  //!< user callback data
} SocketStateT;


ref class DatagramSocketRecvContext sealed
{
public:
    DatagramSocketRecvContext(DatagramSocket^ refDgramSock);
    void OnMessage(DatagramSocket^ refDgramSock, DatagramSocketMessageReceivedEventArgs^ eventArguments);

private:
     SocketT* mSocket;
    ~DatagramSocketRecvContext();
};

ref class StreamSocketListenerContext sealed
{
public:
    StreamSocketListenerContext(StreamSocketListener^ refListenSock);
    void OnConnection(StreamSocketListener^ refListenSock, StreamSocketListenerConnectionReceivedEventArgs^ object);

private:
     SocketT* mSocket;
    ~StreamSocketListenerContext();
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! module state ref
static SocketStateT *_Socket_pState = NULL;

// Public variables


/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function    _XlatError

    \Description
        Translate a SocketErrorStatus to a dirtysock error

    \Input err      - SocketErrorStatus value to be translated

    \Output
        int32_t     - dirtysock error

    \Version 10/28/2012 (mclouatre)
*/
/************************************************************************************F*/
static int32_t _XlatError(SocketErrorStatus err)
{
    int32_t iTranslatedErr;

    switch (err)
    {
        case SocketErrorStatus::NetworkIsUnreachable:
        case SocketErrorStatus::UnreachableHost:
            iTranslatedErr = SOCKERR_UNREACH;
            break;

        case SocketErrorStatus::ConnectionRefused:
            iTranslatedErr = SOCKERR_REFUSED;
            break;

        case SocketErrorStatus::ConnectionResetByPeer:
            iTranslatedErr = SOCKERR_CONNRESET;
            break;

        default:
            iTranslatedErr = SOCKERR_OTHER;
            break;
    }

    return(iTranslatedErr);
}


/*F*************************************************************************************/
/*!
    \Function    _SocketGetHostNameRefFromSockaddrIn

    \Description
        Returns a managed reference to a Windows.Networking.Sockets.Hostname object
        created from the input struct addr.

    \Input pName        - pointer to struct addr

    \Output
        HostName^       - managed reference to a Windows::Networking::Sockets::HostName
                          null if input ip addr was 0

    \Version 10/30/2012 (mclouatre)
*/
/*************************************************************************************/
static HostName^ _SocketGetHostNameRefFromSockaddrIn(const struct sockaddr *pName)
{
    if (SockaddrInGetAddr(pName))
    {
        char strAddrText[16];
        wchar_t strAddrWText[16];

        // get wchar string remote host name (ip addr)
        SockaddrInGetAddrText((struct sockaddr *)pName, strAddrText, sizeof(strAddrText));
        std::mbstowcs(strAddrWText, strAddrText, sizeof(strAddrWText));

        auto refRemoteHostNameStr = ref new String(strAddrWText);
        return(ref new HostName(refRemoteHostNameStr));
    }
    else
    {
        return(nullptr);
    }
}

/*F*************************************************************************************/
/*!
    \Function    _SocketGetServiceNameRefFromSockaddrIn

    \Description
        Returns a managed reference to a Windows.Networking.Sockets.Hostname object
        created from the input struct addr.

    \Input pName        - pointer to struct addr

    \Output
        String^         - managed reference to a Platform::String
                          empty string if port was 0

    \Version 10/30/2012 (mclouatre)
*/
/*************************************************************************************/
static String^ _SocketGetServiceNameRefFromSockaddrIn(const struct sockaddr *pName)
{
    if (SockaddrInGetPort(pName))
    {
        char strPortText[8];
        wchar_t strPortWText[8];

        // get wchar string remote service name (udp port)
        ds_snzprintf(strPortText, sizeof(strPortText), "%d", SockaddrInGetPort(pName));
        std::mbstowcs(strPortWText, strPortText, sizeof(strPortWText));

        return(ref new String(strPortWText));
    }
    else
    {
        return(ref new String(L""));
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _SocketSetSockaddrInAddrFromHostName

    \Description
        Set Internet address component of sockaddr struct from a Windows::Networking::HostName

    \Input *pAddr       - pointer to sockaddr structure to be filled
    \Input ^refHostName - hostname (ip address)

    \Output
        int32_t         - zero=no error, negative=error

    \Version 10/30/2012 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _SocketSetSockaddrInAddrFromHostName(struct sockaddr *pAddr, HostName ^refHostName)
{
    int32_t iResult = 0; // default to success

    // there are 4 different possible hostname types on winrt: ipv4, ipv6, domainname, bluetooth
    // make sure we deal with an ipv4 type here
    if (refHostName->Type == HostNameType::Ipv4)
    {
        char strIpAddr[20];

        String^ ipString = refHostName->CanonicalName;
        memset(strIpAddr, 0, sizeof(strIpAddr));
        std::wcstombs(strIpAddr, ipString->Data(), sizeof(strIpAddr));

        SockaddrInSetAddrText(pAddr, strIpAddr);
    }
    else
    {
        NetPrintf(("dirtynetwinrt: warning - _SocketSetSockaddrInAddrFromHostName() used with unsupported hostname type\n"));
        iResult = -1;   // failure
    }

    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function    _SocketSetSockaddrInPortFromServiceName

    \Description
        Set port component of sockaddr_in struct from a Platform::String

    \Input *pAddr       - pointer to sockaddr structure to be filled
    \Input ^refServName - service name (port)

    \Output
        int32_t         - zero=no error, negative=error

    \Version 10/30/2012 (mclouatre)
*/
/*************************************************************************************************F*/
static int32_t _SocketSetSockaddrInPortFromServiceName(struct sockaddr *pAddr, String ^refServName)
{
    int32_t iResult = 0; // default to success

    if (!(refServName->IsEmpty()))
    {
        char strPort[10];
        std::wcstombs(strPort, refServName->Data(), sizeof(strPort));
        SockaddrInSetPort(pAddr, atoi(strPort));
    }
    else
    {
        NetPrintf(("dirtynetwinrt: warning - _SocketSetSockaddrInPortFromServiceName used against an empty service name\n"));
        iResult = -1;   // failure
    }

    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketIsReadyForKill

    \Description
        Check whether socket can be safely freed or not

    \Input *pSocket - pointer to socket

    \Output
        int32_t    - TRUE or FALSE

    \Version 10/28/2012 (mclouatre)
*/
/************************************************************************************F*/
static int32_t _SocketIsReadyForKill(SocketT *pSocket)
{
    if (pSocket->iType == SOCK_DGRAM)
    {
        return(!pSocket->bConnectAsyncInProgress && 
               !pSocket->bStoreAsyncInProgress &&
               !pSocket->un.dgramSpace.bBindAsyncInProgress &&
               !pSocket->un.dgramSpace.bGetOutStreamAsyncInProgress);
    }
    else if (pSocket->iType == SOCK_STREAM)
    {
        return(!pSocket->bConnectAsyncInProgress &&
               !pSocket->bStoreAsyncInProgress &&
               !pSocket->un.streamSpace.bLoadAsyncInProgress && 
               !pSocket->un.streamSpace.bBindListenerAsyncInProgress);
    }
    else
    {
        return(TRUE);
    }
}

/*F*************************************************************************************/
/*!
    \Function    _SocketRecvData

    \Description
        Called when data is received on socket. Informs high-level layers that registered
        a callback that data was received.

    \Input *pSocket - pointer to socket that has new data

    \Version 10/28/2012 (mclouatre)
*/
/************************************************************************************F*/
static void _SocketRecvData(SocketT *pSocket)
{
    // see if we should issue callback
    if ((pSocket->uCallLast != (unsigned)-1) && (pSocket->pCallback != NULL) && (pSocket->iCallMask & CALLB_RECV))
    {
        pSocket->uCallLast = (unsigned)-1;
        (pSocket->pCallback)(pSocket, 0, pSocket->pCallRef);
        pSocket->uCallLast = NetTick();
    }
}

/*F*************************************************************************************/
/*!
    \Function    _SocketSendData

    \Description
        Pass the outbound data to the data writer.

    \Input *pSocket         - pointer to socket to which the data writer belongs
    \Input *pBuf            - pointer to buffer containing data to be sent
    \Input iLen             - number of byte to be sent
    \Input ^refDataWriter   - data writer to pass data to

    \Version 10/28/2012 (mclouatre)
*/
/************************************************************************************F*/
static void _SocketSendData(SocketT *pSocket, const char *pBuf, int32_t iLen, DataWriter ^refDataWriter)
{
    try
    {
        Array<uint8_t>^ refBuf = ref new Array<uint8_t>((uint8_t*)pBuf, (uint32_t)iLen);
        refDataWriter->WriteBytes(refBuf);

        // return bytes sent
        pSocket->iLastError = iLen;
    }
    catch (Exception^ exception)
    {
        pSocket->iLastError = _XlatError(SocketError::GetStatus(exception->HResult));
        NetPrintf(("dirtynetwinrt: [%p] DataWriter::WriteBytes() failed: %S\n", pSocket, exception->ToString()->Data()));
    }

    if (pSocket->iLastError > 0)
    {
        // write the locally buffered data to the network.
        pSocket->bStoreAsyncInProgress = TRUE;
        task<unsigned int>(refDataWriter->StoreAsync()).then([pSocket] (task<unsigned int> previousTask)
        {
            try
            {
                // try getting an exception.
                previousTask.get();
            }
            catch (Exception^ exception)
            {
                NetPrintf(("dirtynetwinrt: [%p] DataWriter::StoreAsync() failed: %S\n", pSocket, exception->ToString()->Data()));

                // mark socket in failure only if stream socket
                if (pSocket->iType == SOCK_STREAM)
                {
                    pSocket->iOpened = -1;
                }
            }

            pSocket->bStoreAsyncInProgress = FALSE;
        });
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DatagramSocketConsumeInboundMsgEvent

    \Description
        Consume an inbound messsage event from the inbound queue

    \Input *pSocket - socket reference

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Version 11/20/2012 (mclouatre)
*/
/************************************************************************************F*/
static int32_t _DatagramSocketConsumeInboundMsgEvent(SocketT *pSocket)
{
    int32_t iResult = SOCKERR_NONE;

    try
    {
        // acquire socket critical section
        NetCritEnter(&pSocket->sockCrit);

        if (pSocket->refInboundDgramEventsVect->Size)
        {
            DatagramSocketMessageReceivedEventArgs^ refEventArgs = pSocket->refInboundDgramEventsVect->GetAt(0);

            if ((uint32_t)refEventArgs->GetDataReader()->UnconsumedBufferLength <= sizeof(pSocket->un.dgramSpace.aRecvData))
            {
                uint32_t uReadSize = refEventArgs->GetDataReader()->UnconsumedBufferLength;

                refEventArgs->GetDataReader()->ReadBytes(ArrayReference<uint8_t>((uint8_t *)&pSocket->un.dgramSpace.aRecvData, uReadSize));
                pSocket->un.dgramSpace.iRecvStat = (int32_t)uReadSize;

                // save address of sender
                SockaddrInit(&pSocket->recvAddr, AF_INET);
                _SocketSetSockaddrInAddrFromHostName(&pSocket->recvAddr, refEventArgs->RemoteAddress);
                _SocketSetSockaddrInPortFromServiceName(&pSocket->recvAddr, refEventArgs->RemotePort);

                // save receive timestamp (ideally this timestamp should be saved in the OnMessage handler)
                SockaddrInSetMisc(&pSocket->recvAddr, NetTick());

                // remove that datagram entry from the vector
                pSocket->refInboundDgramEventsVect->RemoveAt(0);
            }
            else
            {
                NetPrintf(("dirtynetwinrt: [%p] _DatagramSocketConsumeInboundMsgEvent() dropped dgram because too large (%d bytes)\n",
                    pSocket, refEventArgs->GetDataReader()->UnconsumedBufferLength));
            }

        }

        // release socket critical section
        NetCritLeave(&pSocket->sockCrit);
    }
    catch (Exception^ exception)
    {
        iResult = _XlatError(SocketError::GetStatus(exception->HResult));
        NetPrintf(("dirtynetwinrt: [%p] _DatagramSocketConsumeInboundMsgEvent() failed: %S\n", pSocket, exception->ToString()->Data()));
    }

    return(iResult);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS   // defined in .dxy file
/*F*************************************************************************************/
/*!
    \Function    DatagramSocketRecvContext::DatagramSocketRecvContext

    \Input ^refDgramSock - socket object reference

    \Description
        Constructor

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
DatagramSocketRecvContext::DatagramSocketRecvContext(DatagramSocket^ refDgramSock)
{
    SocketStateT *pState = _Socket_pState;
    SocketT *pSocket;

    NetCritEnter(NULL);

    // walk socket list and find SocketT struct to which this refDgramSock belongs
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        if (ReferenceEquals(pSocket->refSock, refDgramSock))
        {
            DatagramSocketRecvContext::mSocket = pSocket;
            break;
        }
    }

    NetCritLeave(NULL);
};

/*F*************************************************************************************/
/*!
    \Function    DatagramSocketRecvContext::~DatagramSocketRecvContext

    \Description
        Destructor

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
DatagramSocketRecvContext::~DatagramSocketRecvContext()
{
}

/*F*************************************************************************************/
/*!
    \Function    DatagramSocketRecvContext::OnMessage

    \Description
        Handler registered with the system and invoked upon reception of inbound udp datagrams.

    \Input ^refDgramSock    - system socket on which datagram was received
    \Input ^refEventArgs    - parameters associated with the received data

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
void DatagramSocketRecvContext::OnMessage(DatagramSocket^ refDgramSock, DatagramSocketMessageReceivedEventArgs^ refEventArgs)
{
    // acquire socket critical section
    NetCritEnter(&mSocket->sockCrit);

    if (mSocket->refInboundDgramEventsVect->Size < (uint8_t)mSocket->un.dgramSpace.iMaxPendingDgrams)
    {
        // add new event to the vector of pending message events
        mSocket->refInboundDgramEventsVect->Append(refEventArgs);

        // forward data to socket callback
        _SocketRecvData(mSocket);
    }
    else
    {
        NetPrintf(("dirtynetwinrt: [%p] DatagramSocketRecvContext::OnMessage() inbound dgram dropped because max (%d) pending dgrams reached\n",
            mSocket, mSocket->un.dgramSpace.iMaxPendingDgrams));
    }

    // release socket critical section
    NetCritLeave(&mSocket->sockCrit);
}

/*F*************************************************************************************/
/*!
    \Function    StreamSocketListenerContext::StreamSocketListenerContext

    \Input ^refListenSock   - socket object reference, or nullptr

    \Description
        Constructor

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
StreamSocketListenerContext::StreamSocketListenerContext(StreamSocketListener^ refListenSock)
{
    SocketStateT *pState = _Socket_pState;
    SocketT *pSocket;

    NetCritEnter(NULL);

    // walk socket list and find SocketT struct to which this refDgramSock belongs
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        if (ReferenceEquals(pSocket->refSock, refListenSock))
        {
            StreamSocketListenerContext::mSocket = pSocket;
            break;
        }
    }

    NetCritLeave(NULL);
};

/*F*************************************************************************************/
/*!
    \Function    StreamSocketListenerContext::~StreamSocketListenerContext

    \Description
        Destructor

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
StreamSocketListenerContext::~StreamSocketListenerContext()
{
}

/*F*************************************************************************************/
/*!
    \Function    StreamSocketListenerContext::OnConnection

    \Description
        Handler registered with the system and invoked upon reception of inbound udp datagrams.

    \Input ^refListenSock   - system socket on which the connection event occured
    \Input ^refEventArgs    - parameters associated with the inbound connection event

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
void StreamSocketListenerContext::OnConnection(StreamSocketListener^ refListenSock, StreamSocketListenerConnectionReceivedEventArgs^ refEventArgs)
{
    // acquire socket critical section
    NetCritEnter(&mSocket->sockCrit);

    if (mSocket->refStreamConnectVect->Size < (uint8_t)mSocket->un.streamSpace.iMaxPendingConnections)
    {
        // add new connected sockets to the connections vector
        mSocket->refStreamConnectVect->Append(refEventArgs->Socket);

        NetPrintf(("dirtynetwinrt: [%p] StreamSocketListenerContext::OnConnection() count of pending connections incremented to %d\n",
            mSocket, mSocket->refStreamConnectVect->Size));
    }
    else
    {
        delete refEventArgs->Socket;

        NetPrintf(("dirtynetwinrt: [%p] StreamSocketListenerContext::OnConnection() pending connection destroyed because max (%d) pending connections reached\n",
            mSocket, mSocket->un.streamSpace.iMaxPendingConnections));
    }

    // release socket critical section
    NetCritLeave(&mSocket->sockCrit);
}
#endif // DOXYGEN_SHOULD_SKIP_THIS


/*F*************************************************************************************/
/*!
    \Function    _SocketOpen

    \Description
        Allocates a SocketT.  If socket is nullptr, a WinSock socket ref is created,
        otherwise s is used.

    \Input ^refSock     - socket to use, or nullptr
    \Input iFamily      - address family
    \Input iType        - type (SOCK_DGRAM, SOCK_STREAM, ...)
    \Input iProto       - protocol
    \Input iOpened      - -1=failure; 0=not open (connecting), 1=open

    \Output
        SocketT         - pointer to new socket, or NULL

    \Version 10/30/2012 (mclouatre)
*/
/*************************************************************************************/
static SocketT *_SocketOpen(Object^ refSock, int32_t iFamily, int32_t iType, int32_t iProto, int32_t iOpened)
{
    SocketStateT *pState = _Socket_pState;
    SocketT *pSocket;

    // allocate memory
    pSocket = (SocketT *)DirtyMemAlloc(sizeof(*pSocket), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(pSocket, 0, sizeof(*pSocket));

    if (iType == SOCK_DGRAM)
    {
        if (refSock == nullptr)
        {
            // create datagram socket object and
            refSock = ref new DatagramSocket();
        }

        pSocket->un.dgramSpace.iMaxPendingDgrams = DIRTYNET_DGRAMINBOUNDQ_MAXSIZE;

        // allocate the vector used to store inbound datagrams
        pSocket->refInboundDgramEventsVect = ref new Platform::Collections::Vector<DatagramSocketMessageReceivedEventArgs ^>();
    }
    else if (iType == SOCK_STREAM)
    {
        if (refSock == nullptr)
        {
            // create stream socket object
            refSock = ref new StreamSocket();
        }

        pSocket->bTypeConnected = TRUE; // a stream socket is always a "connected" type socket
    }
    else
    {
        NetPrintf(("dirtynetwinrt: critical error in _SocketOpen() - socket type %d not supported\n", pSocket->iType));
        pSocket->iLastError = SOCKERR_UNSUPPORT;
        return(NULL);
    }

    // save the socket
    pSocket->refSock = refSock;
    pSocket->iLastError = SOCKERR_NONE;

    // set family/proto info
    pSocket->iFamily = iFamily;
    pSocket->iType = iType;
    pSocket->iProto = iProto;
    pSocket->iOpened = iOpened;

    // inititalize critical section
    NetCritInit(&pSocket->sockCrit, "socket-crit");

    // install into list
    NetCritEnter(NULL);
    pSocket->pNext = pState->pSockList;
    pState->pSockList = pSocket;
    NetCritLeave(NULL);

    // because the constructor of DatagramSocketRecvContext assumes that the newly created
    // pSocket is in the pSockList, this block of code needs to be executed after list insertion
    if (iType == SOCK_DGRAM)
    {
        // register for inbound datagram events
        pSocket->refSockEventCtx = ref new DatagramSocketRecvContext(dynamic_cast<DatagramSocket^>(refSock));
        dynamic_cast<DatagramSocket^>(refSock)->MessageReceived += ref new TypedEventHandler<DatagramSocket^, DatagramSocketMessageReceivedEventArgs^>(dynamic_cast<DatagramSocketRecvContext ^>(pSocket->refSockEventCtx), &DatagramSocketRecvContext::OnMessage);
    }

    NetPrintf(("dirtynetwinrt: [%p] new %s socket\n", pSocket, ((pSocket->iType==SOCK_DGRAM) ? "SOCK_DGRAM" : "SOCK_STREAM")));

    // return the socket
    return(pSocket);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketClose

    \Description
        Disposes of a SocketT, including removal from the global socket list and
        disposal of the SocketT allocated memory.  Does NOT dispose of the winrt
        socket ref.

    \Input *pSocket     - socket to close
    \Input bShutdown    - if TRUE, shutdown and close socket ref

    \Output
        int32_t         - negative=failure, zero=success

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
static int32_t _SocketClose(SocketT *pSocket, uint32_t bShutdown)
{
    SocketStateT *pState = _Socket_pState;
    uint8_t bSockInList = FALSE;
    SocketT **ppSocket;

    // for access to g_socklist and g_sockkill
    NetCritEnter(NULL);

    // remove sock from linked list
    for (ppSocket = &pState->pSockList; *ppSocket != NULL; ppSocket = &(*ppSocket)->pNext)
    {
        if (*ppSocket == pSocket)
        {
            *ppSocket = pSocket->pNext;
            bSockInList = TRUE;
            break;
        }
    }

    // release before NetIdleDone
    NetCritLeave(NULL);

    // make sure the socket is in the socket list (and therefore valid)
    if (bSockInList == FALSE)
    {
        NetPrintf(("dirtynetwinrt: [%p] warning, trying to close socket that is not in the socket list\n", pSocket));
        return(-1);
    }

    // finish any idle call
    NetIdleDone();

     // mark socket as no longer connected
    pSocket->iOpened = -1;

    // release managed references
    // assumption is that this will result in abortion of any pending async operation with this exception:
    // The I/O operation has been aborted because of either a thread exit or an application request.
    if (pSocket->refDataReader != nullptr)
    {
        delete pSocket->refDataReader;
        pSocket->refDataReader = nullptr;
    }

    if (pSocket->refDataWriter != nullptr)
    {
        delete pSocket->refDataWriter;
        pSocket->refDataWriter = nullptr;
    }

    if (pSocket->refSockEventCtx != nullptr)
    {
        delete pSocket->refSockEventCtx;
        pSocket->refSockEventCtx = nullptr;
    }

    if (pSocket->refSock != nullptr)
    {
        delete pSocket->refSock;
        pSocket->refSock = nullptr;
    }

    /*
    put into killed list
    used to postpone two things:
      1- delete all managed references
      2- free of pSocket->sockCrit
      3- free of socket ref
    until no async operation depending on it is in progress
    */
    NetCritEnter(NULL);
    pSocket->pKill = pState->pSockKill;
    pState->pSockKill = pSocket;
    NetCritLeave(NULL);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    _StreamSocketAsyncRecv

    \Description
        Attempts to start async recv on stream socket

    \Input *pSocket    - socket to read

    \Notes
        \verbatim
        A data reader is used in the receive path of a stream socket. The data is obtained
        asynchronously from the socket with the DataReader::LoadAsync() method and it gets
        cached in the DataReader's internal buffer. Later in time, when SocketRecvfrom()
        is invoked from the main thread, the data is copied from the DataReader's cache into
        the user-provided buffer using DataReader::ReadBytes().

        This mechanism is assumed to be thread-safe because the data reader class has
        the following attributes (ref: http://msdn.microsoft.com/library/windows/apps/BR208119)
            MarshalingBehaviorAttribute: Agile
            ThreadingAttribute: Both

        Class attributes are explained here:
        http://msdn.microsoft.com/en-us/library/windows/apps/hh771042.aspx
        \endverbatim

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
static void _StreamSocketAsyncRecv(SocketT *pSocket)
{
    if (pSocket->iType != SOCK_STREAM)
    {
        NetPrintf(("dirtynetwinrt: critical error - _StreamSocketAsyncRecv() can only be used with type SOCK_STREAM\n"));
        return;
    }

    // if socket is connected  and there is not already a LoadAsync in progress
    if ((pSocket->iOpened > 0) && (pSocket->un.streamSpace.bLoadAsyncInProgress == FALSE))
    {
        pSocket->un.streamSpace.bLoadAsyncInProgress = TRUE;
        task<unsigned int>(pSocket->refDataReader->LoadAsync(512)).then([pSocket] (unsigned int iBytesLoaded)
        {
            if (iBytesLoaded == 0)
            {
                // the underlying socket was closed before we were able to read the whole data.
                pSocket->iOpened = -1;

                // acquire socket critical section
                NetCritEnter(&pSocket->sockCrit);

                pSocket->iRecvErr = SOCKERR_CLOSED;

                // release socket critical section
                NetCritLeave(&pSocket->sockCrit);

                cancel_current_task();
            }
        }).then([pSocket] (task<void> previousTask)
        {
            try
            {
                // try getting all exceptions from the continuation chain above this point.
                previousTask.get();

                // acquire socket critical section
                NetCritEnter(&pSocket->sockCrit);

                // forward data to socket callback
                _SocketRecvData(pSocket);

                // release socket critical section
                NetCritLeave(&pSocket->sockCrit);
            }
            catch(Platform::Exception^ exception)
            {
                NetPrintf(("dirtynetwinrt: [%p] DataReader::LoadAsync() failed: %S\n", pSocket, exception->ToString()->Data()));
                pSocket->iOpened = -1;

                // acquire socket critical section
                NetCritEnter(&pSocket->sockCrit);

                pSocket->iRecvErr = _XlatError(SocketError::GetStatus(exception->HResult));

                // release socket critical section
                NetCritLeave(&pSocket->sockCrit);
            }
            catch (task_canceled&)
            {
                NetPrintf(("dirtynetwinrt: [%p] DataReader::LoadAsync() cancelled\n", pSocket));
            }

            pSocket->un.streamSpace.bLoadAsyncInProgress = FALSE;
        });
    }
}

/*F*************************************************************************************/
/*!
    \Function    _DatagramSocketGetMapKeyFromSockaddrIn

    \Description
        Generates the map key from a Sockaddr struct. Key format is "<ipaddr>:<port>"

    \Input *pSocket - socket reference
    \Input *pAddr   - peer address

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
static void _DatagramSocketGetMapKeyFromSockaddrIn(SocketT *pSocket, const struct sockaddr *pAddr)
{
    char strAddrText[16];
    char strPortText[8];

    SockaddrInGetAddrText((struct sockaddr *)pAddr, strAddrText, sizeof(strAddrText));
    _itoa_s(SockaddrInGetPort(pAddr), strPortText, sizeof(strPortText), 10);

    std::string strPeer = strAddrText;
    strPeer.append(":");
    strPeer.append(strPortText);

    std::mbstowcs(pSocket->un.dgramSpace.wstrMapKey, strPeer.data(), DIRTYNET_DATAWRITERSMAPKEY_MAXLEN);
}

/*F*************************************************************************************/
/*!
    \Function    _DatagramSocketAsyncGetOutputStream

    \Description
        Starts async recv operation to obtain the peer-specific output stream
        object of an unconnected datagram socket

    \Input *pSocket     - socket pointer
    \Input *pTo         - peer address

    \Output
        DataWriter^     - data writer associated with peer, nullptr if not yet available

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
static DataWriter^ _DatagramSocketAsyncGetOutputStream(SocketT *pSocket, const struct sockaddr *pTo)
{
    if (pSocket->un.dgramSpace.bGetOutStreamAsyncInProgress == FALSE)
    {
         int8_t bHasKey;

        _DatagramSocketGetMapKeyFromSockaddrIn(pSocket, pTo);

        auto refPeerStr = ref new String(pSocket->un.dgramSpace.wstrMapKey);

        if (pSocket->refDgramDataWritersMap == nullptr)
        {
            pSocket->refDgramDataWritersMap = ref new Platform::Collections::Map<String ^, DataWriter ^>();
        }

        // acquire socket critical section
        NetCritEnter(&pSocket->sockCrit);

        bHasKey = pSocket->refDgramDataWritersMap->HasKey(refPeerStr);

        // release socket critical section
        NetCritLeave(&pSocket->sockCrit);

        // if we already have a DataWriter for this pTo use it, if not get one
        if (bHasKey)
        {
            // acquire socket critical section
            NetCritEnter(&pSocket->sockCrit);

            DataWriter ^refDataWriter = pSocket->refDgramDataWritersMap->Lookup(refPeerStr);

            // release socket critical section
            NetCritLeave(&pSocket->sockCrit);

            return(refDataWriter);
        }
        else
        {
            auto refRemoteHostName = _SocketGetHostNameRefFromSockaddrIn(pTo);
            auto refRemoteServNameStr = _SocketGetServiceNameRefFromSockaddrIn(pTo);

            // get output stream using GetOutputStreamAsync(Hostname)
            pSocket->un.dgramSpace.bGetOutStreamAsyncInProgress = TRUE;
            task<IOutputStream^>(dynamic_cast<DatagramSocket^>(pSocket->refSock)->GetOutputStreamAsync(refRemoteHostName, refRemoteServNameStr)).then([pSocket] (IOutputStream^ stream)
            {
                auto refPeerStr = ref new String(pSocket->un.dgramSpace.wstrMapKey);

                // acquire socket critical section
                NetCritEnter(&pSocket->sockCrit);

                // make sure the maximum number of output stream per socket is not reached
                if (pSocket->refDgramDataWritersMap->Size < DIRTYNET_DATAWRITERSMAP_MAXSIZE)
                {
                    pSocket->refDgramDataWritersMap->Insert(refPeerStr, ref new DataWriter(stream));
                }

                // release socket critical section
                NetCritLeave(&pSocket->sockCrit);

            }).then([pSocket] (task<void> previousTask)
            {
                try
                {
                    // try getting an exception.
                    previousTask.get();
                }
                catch (Exception^ exception)
                {
                    // acquire socket critical section
                    NetCritEnter(&pSocket->sockCrit);

                    pSocket->iRecvErr = _XlatError(SocketError::GetStatus(exception->HResult));

                    // release socket critical section
                    NetCritLeave(&pSocket->sockCrit);

                    NetPrintf(("dirtynetwinrt: [%p] DatagramSocket::GetOuptutStreamAsync(EndPointPair) failed: %S\n", pSocket, exception->ToString()->Data()));
                }

                pSocket->un.dgramSpace.bGetOutStreamAsyncInProgress = FALSE;
            });
        }
    }

    return(nullptr);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketKill

    \Description
        Release resources used by the socket and free the SocketT.

    \Input *pSocket     - socket pointer

    \Version 11/14/2012 (mclouatre)
*/
/************************************************************************************F*/
static void _SocketKill(SocketT *pSocket)
{
    SocketStateT *pState = _Socket_pState;

    NetCritKill(&pSocket->sockCrit); // kill critical section

    pState->pSockKill = pSocket->pKill;

    NetPrintf(("dirtynetwinrt: [%p] socket killed!\n", pSocket));

    // free the socket memory
    DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketIdle

    \Description
        Call idle processing code to give connections time.

    \Input *pModuleState  - module state

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
static void _SocketIdle(void *pModuleState)
{
    SocketStateT *pState = (SocketStateT *)pModuleState;
    SocketT *pSocket;
    uint32_t uTick = NetTick();

    // for access to g_socklist and g_sockkill
    NetCritEnter(NULL);

    // walk socket list and perform any callbacks or try to initiate an async recv
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        // see if we should do callback
        if ((pSocket->uCallIdle != 0) && 
            (pSocket->pCallback != NULL) && 
            (pSocket->uCallLast != (unsigned)-1) && 
            (uTick-pSocket->uCallLast > pSocket->uCallIdle))
        {
            pSocket->uCallLast = (unsigned)-1;
            (pSocket->pCallback)(pSocket, 0, pSocket->pCallRef);
            pSocket->uCallLast = uTick = NetTick();
        }

        // see if we should initiate an async stream socket read
        if (pSocket->iType == SOCK_STREAM)
        {
            _StreamSocketAsyncRecv(pSocket);
        }
    }

    // delete any killed sockets
    while ((pSocket = pState->pSockKill) != NULL)
    {
        if (_SocketIsReadyForKill(pSocket))
        {
            _SocketKill(pSocket);
        }
    }

    // for access to g_socklist and g_sockkill
    NetCritLeave(NULL);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *pHost   - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
static int32_t _SocketLookupDone(HostentT *pHost)
{
    // return current status
    return(pHost->done);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketLookupFree

    \Description
        Release resources used for name resolution

    \Input *pHost   - pointer to host lookup record

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
static void _SocketLookupFree(HostentT *pHost)
{
    SocketStateT *pState = _Socket_pState;

    // pHost->thread is accessed atomically and used to determine who
    // has to free pHost, the main thread or the async op completion handler
    if (InterlockedExchange((volatile LONG *)&pHost->thread, 1) != 0)
    {
        // pHost->thread was already marked by the async op completion handler
        // this means that the handler assumed that the main thread would free pHost
        DirtyMemFree(pHost, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
}


/*** Public Functions ******************************************************************/

/*F*************************************************************************************/
/*!
    \Function    SocketCreate

    \Description
        Create new instance of socket interface module.  Initializes all global
        resources and makes module ready for use.

    \Input iThreadPrio        - priority to start threads with
    \Input iThreadStackSize   - stack size to start threads with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start threads with

    \Output
        int32_t               - negative=error, zero=success

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintf(("dirtynetwinrt: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetwinrt: DirtySock SDK v%d.%d.%d.%d\n",
        (DIRTYVERS>>24)&0xff, (DIRTYVERS>>16)&0xff,
        (DIRTYVERS>> 8)&0xff, (DIRTYVERS>> 0)&0xff));

    // alloc and init state ref
    if ((pState = (SocketStateT *)DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetwinrt: unable to allocate module state\n"));
        return(-2);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;

    // save global module ref
    _Socket_pState = pState;

    // startup network libs
    NetLibCreate(iThreadPrio, 0, 0);

    // add our idle handler
    NetIdleAdd(&_SocketIdle, pState);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    SocketDestroy

    \Description
        Release resources and destroy module.

    \Input uShutdownFlags   - shutdown flags (unused)

    \Output
        int32_t             - negative=error, zero=success

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketDestroy(uint32_t uShutdownFlags)
{
    SocketStateT *pState = _Socket_pState;
    uint32_t uStartTick;
    int32_t iTickDiff;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtynetwinrt: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetwinrt: shutting down\n"));

    // kill idle callbacks
    NetIdleDel(&_SocketIdle, pState);

    // let any idle event finish
    NetIdleDone();

    // close all sockets
    while (pState->pSockList != NULL)
    {
        SocketClose(pState->pSockList);
    }

    uStartTick = NetTick();

    // clear the kill list
    while (pState->pSockKill != NULL)
    {
        _SocketIdle(pState);
        NetSleep(10);
    }

    if ((iTickDiff = NetTickDiff(NetTick(), uStartTick)) > 50)
    {
        NetPrintf(("dirtynetwinrt: warning SocketDestroy() stalled for more than 50 ms (%d ms) waiting for kill list clean up to complet\n", iTickDiff));
    }

    // free the memory and clear global module ref
    _Socket_pState = NULL;
    DirtyMemFree(pState, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // shut down network libs
    NetLibDestroy(0);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.

    \Input iFamily  - address family (AF_INET)
    \Input iType    - socket type (SOCK_STREAM or SOCK_DGRAM)
    \Input iProto   - unused

    \Output
        SocketT     - socket reference

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
SocketT *SocketOpen(int32_t iFamily, int32_t iType, int32_t iProto)
{
    return(_SocketOpen(nullptr, iFamily, iType, iProto, 0));
}

/*F*************************************************************************************/
/*!
    \Function    SocketClose

    \Description
        Close a socket. Performs a graceful shutdown of connection oriented protocols.

    \Input *pSocket - socket reference

    \Output
        int32_t     - zero

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketClose(SocketT *pSocket)
{
    return(_SocketClose(pSocket, TRUE));
}

/*F*************************************************************************************/
/*!
    \Function    SocketImport

    \Description
        Import a socket.  The given socket shall be a SocketT, in which case a
        SocketT pointer to the ref is returned. Unlike the SocketImport() for other
        platforms, the given socket cannot be an actual Windows socket ref.

    \Input uSockRef     - socket reference

    \Output
        SocketT *       - pointer to imported socket, or NULL

    \Notes
        $todo  - evaluate the need/feasibility for creating a new prototype of
                 SocketImport() that can take managed ref as input

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
SocketT *SocketImport(intptr_t uSockRef)
{
    SocketStateT *pState = _Socket_pState;
    SocketT *pSock;

    // see if this socket is already in our socket list
    NetCritEnter(NULL);
    for (pSock = pState->pSockList; pSock != NULL; pSock = pSock->pNext)
    {
        if (pSock == (SocketT *)uSockRef)
        {
            break;
        }
    }
    NetCritLeave(NULL);

    return(pSock);
}

/*F*************************************************************************************/
/*!
    \Function SocketRelease

    \Description
        Release an imported socket. NO-OP on WinRT.

    \Input *pSocket - pointer to socket

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
void SocketRelease(SocketT *pSocket)
{
}

/*F*************************************************************************************/
/*!
    \Function    SocketShutdown

    \Description
        No-op on WinRT

    \Input *pSocket - socket reference
    \Input iHow     - SOCK_NOSEND and/or SOCK_NORECV

    \Output
        int32_t     - zero

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketShutdown(SocketT *pSocket, int32_t iHow)
{
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    SocketBind

    \Description
        Bind a local address/port to a datagram socket.

    \Input *pSocket - socket reference
    \Input *pName   - local address/port
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        \verbatim
        If either address or port is zero, then they will be filled in automatically by
        the system.

        With WinRT binding is an asynchronous operation that is only explicitly supported
        on DatagramSockets. For StreamSockets, the binding is an implicit operation of the
        connection phase. Because our SocketBind() is assumed to be a synchronous operation
        (meaning that the socket is expected to be bound right after the call returning),
        wrapping the winrt api is not straighforward. Here is how we do it:
        For DGRAM sockets:  We initiate the async bind, and we then block waiting for the
                            bind to complete. Assumption: datagram socket binding will 
                            always complete within a minimal delay (experimentation showed 
                            ~12 ms). This allows us to keep the synchronous bind() metaphore
                            with our public api.
        For STREAM sockets: We save the inputs from the user and we will user them later
                            when connecting. We always return success. Limitation: client
                            code can no longer assume that the socket is bound when
                            SocketBind() returns.

        $todo evaluate if we can benefit of DatagramSocket::BindServiceNameAsync() which
              binds to all interfaces on the system
        \endverbatim

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketBind(SocketT *pSocket, const struct sockaddr *pName, int32_t iNameLen)
{
    SocketStateT *pState = _Socket_pState;

    pSocket->iLastError = SOCKERR_NONE;

    // save bind address
    memcpy(&pSocket->bindAddr, pName, sizeof(pSocket->bindAddr));

    // bind to specific address?
    if ((SockaddrInGetAddr(pName) == 0) && (pState->uAdapterAddress != 0))
    {
        SockaddrInSetAddr(&pSocket->bindAddr, pState->uAdapterAddress);
    }

    if (pSocket->iType == SOCK_DGRAM)
    {
        // early exit if socket is already bound
        if (dynamic_cast<DatagramSocket^>(pSocket->refSock)->Information->LocalAddress != nullptr)
        {
            pSocket->iLastError = SOCKERR_INVALID;
            return(pSocket->iLastError);
        }

        uint16_t uPort = (uint16_t)SockaddrInGetPort(&pSocket->bindAddr);
        DatagramSocket^ datagramSocket = dynamic_cast<DatagramSocket^>(pSocket->refSock);

        // check if the port is already virtual port
        if (uPort != 0)
        {
            int32_t iPort;

            // find virtual port in list
            for (iPort = 0; (iPort < SOCKET_MAXVIRTUALPORTS) && (pState->aVirtualPorts[iPort] != uPort); iPort++)
                ;
            if (iPort < SOCKET_MAXVIRTUALPORTS)
            {
                // close winsock socket
                NetPrintf(("dirtynetwinrt: [%p] making socket bound to port %d virtual\n", pSocket, uPort));
                if (pSocket->refSock != nullptr)
                {
                    delete pSocket->refSock;
                    pSocket->refSock = nullptr;
                }
                pSocket->un.dgramSpace.uVirtualPort = uPort;
                pSocket->un.dgramSpace.bVirtual = TRUE;
            }
        }

        if (!pSocket->un.dgramSpace.bVirtual)
        {
            auto refLocalServNameStr = _SocketGetServiceNameRefFromSockaddrIn(&pSocket->bindAddr);
            auto refLocalHostName = _SocketGetHostNameRefFromSockaddrIn(&pSocket->bindAddr);

            // start async bind
            pSocket->un.dgramSpace.bBindAsyncInProgress = TRUE;
            task<void>(datagramSocket->BindEndpointAsync(refLocalHostName, refLocalServNameStr)).then([pSocket] (task<void> previousTask)
            {
                /* 
                no thread-safety concerns here: because we wait for the async op to complete before
                returning from SocketBind(), there is no risk of contention between async op thread
                and main thread.
                */
                try
                {
                    // try getting an exception.
                    previousTask.get();
                }
                catch (Exception^ exception)
                {
                    pSocket->iLastError = _XlatError(SocketError::GetStatus(exception->HResult));
                    NetPrintf(("dirtynetwinrt: [%p] DatagramSocket::BindEndpointAsync() failed: %S\n", pSocket, exception->ToString()->Data()));
                }

                pSocket->un.dgramSpace.bBindAsyncInProgress = FALSE;
            });
        }

        // block until the binding completes
        while (pSocket->un.dgramSpace.bBindAsyncInProgress == TRUE)
        {
            NetSleep(10);
        }
    }
    else if (pSocket->iType == SOCK_STREAM)
    {
        // early exit if socket is already bound
        if (dynamic_cast<StreamSocket^>(pSocket->refSock)->Information->LocalAddress != nullptr)
        {
            pSocket->iLastError = SOCKERR_INVALID;
            return(pSocket->iLastError);
        }

        // with winrt streamsocket class, binding is an implicit operation performed during StreamSocket::ConnectAsync().
        // the bind address was saved in pSocket->bindAddr and will be used when connecting (see notes in function header)
    }
    else
    {
        NetPrintf(("dirtynetwinrt: [%p] critical error in SocketBind() - socket type %d not supported\n", pSocket, pSocket->iType));
        pSocket->iLastError = SOCKERR_UNSUPPORT;
    }

    if (pSocket->iLastError == SOCKERR_NONE)
    {
        pSocket->bSocketBindCalled = TRUE;
    }
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketConnect

    \Description
        For SOCK_STREAM sockets, SocketConnect() initiates the connection establisment with 
        the remote host. For SOCK_DGRAM sockets, SocketConnect() defines the remote peer
        where datagrams will be sent, it also restrics remote IP addresses of packets
        that will be accepted to the remote peer ip address.

    \Input *pSocket - socket reference
    \Input *pName   - pointer to name of socket to connect to
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        For both SOCK_DGRAM and SOCK_STREAM sockets, this operation is asynchronous and
        the caller is expected to poll for completion with SocketInfo('stat') before
        proceeding with sending/receiving on the socket.

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketConnect(SocketT *pSocket, struct sockaddr *pName, int32_t iNameLen)
{
    int32_t iResult;
    struct sockaddr tempAddr;

    pSocket->iLastError = SOCKERR_NONE;

    SockaddrInit(&tempAddr, AF_INET);

    /* execute an explicit bind - this allows us to specify a non-zero local address
       or port (see SocketBind()).  a SOCKERR_INVALID result here means the socket has
       already been bound, so we ignore that particular error */
    if (((iResult = SocketBind(pSocket, &tempAddr, sizeof(tempAddr))) < 0) && (iResult != SOCKERR_INVALID))
    {
        pSocket->iLastError = iResult;
        return(pSocket->iLastError);
    }

    // save connect address
    memcpy(&pSocket->remoteAddr, pName, sizeof(pSocket->remoteAddr));

    // mark the socket as being a connected socket
    pSocket->bTypeConnected = TRUE;

    // mark the socket connecting
    pSocket->iOpened = 0;

    auto refRemoteHostName = _SocketGetHostNameRefFromSockaddrIn(&pSocket->remoteAddr);
    auto refRemoteServNameStr = _SocketGetServiceNameRefFromSockaddrIn(&pSocket->remoteAddr);

    if (pSocket->iType == SOCK_DGRAM)
    {
        /*
        connect using ConnectAsync(Hostname)
        ConnectAsync(Endpoint) implies an implicit bind, which we don't want
        with our tech, user has to call SocketBind() if he wants to bind a SOCK_DGRAM socket
        */
        pSocket->bConnectAsyncInProgress = TRUE;
        task<void>(dynamic_cast<DatagramSocket^>(pSocket->refSock)->ConnectAsync(refRemoteHostName, refRemoteServNameStr)).then([pSocket] (task<void> previousTask)
        {
            try
            {
                // try getting an exception.
                previousTask.get();
                pSocket->refDataWriter = ref new DataWriter(dynamic_cast<DatagramSocket^>(pSocket->refSock)->OutputStream);

                // mark the socket as opened
                pSocket->iOpened = 1;
            }
            catch (Exception^ exception)
            {
                NetPrintf(("dirtynetwinrt: [%p] (datagram socket) DatagramSocket::ConnectAsync(HostName) failed: %S\n", pSocket, exception->ToString()->Data()));

                // mark the socket in failure
                pSocket->iOpened = -1;
            }

            pSocket->bConnectAsyncInProgress = FALSE;
        });
    }
    else if (pSocket->iType == SOCK_STREAM)
    {
        if (pSocket->bSocketBindCalled)
        {
            /*
            connect using ConnectAsync(Endpoing) passing in the bindAddr prepared in SocketBind()
            */
            
            auto refLocalHostName = _SocketGetHostNameRefFromSockaddrIn(&pSocket->bindAddr);
            auto refLocalServNameStr = _SocketGetServiceNameRefFromSockaddrIn(&pSocket->bindAddr);
            auto refEndPointPair = ref new EndpointPair(refLocalHostName, refLocalServNameStr, refRemoteHostName, refRemoteServNameStr);

            pSocket->bConnectAsyncInProgress = TRUE;
            task<void>(dynamic_cast<StreamSocket^>(pSocket->refSock)->ConnectAsync(refEndPointPair, SocketProtectionLevel::PlainSocket)).then([pSocket] (task<void> previousTask)
            {
                try
                {
                    // try getting an exception.
                    previousTask.get();
                    pSocket->refDataWriter = ref new DataWriter(dynamic_cast<StreamSocket^>(pSocket->refSock)->OutputStream);
                    pSocket->refDataReader = ref new DataReader(dynamic_cast<StreamSocket^>(pSocket->refSock)->InputStream);

                    // set "partial" option: the asynchronous read operation should complete as soon as a minimum number of bytes are available.
                    pSocket->refDataReader->InputStreamOptions = InputStreamOptions::Partial;

                    // mark the socket as opened
                    pSocket->iOpened = 1;
                }
                catch (Exception^ exception)
                {
                    NetPrintf(("dirtynetwinrt: [%p] (stream socket) StreamSocket::ConnectAsync(EndPointPair) failed: %S\n", pSocket, exception->ToString()->Data()));

                    // mark the socket in failure
                    pSocket->iOpened = -1;
                }

                pSocket->bConnectAsyncInProgress = FALSE;
            });
        }
        else
        {
            /*
            connect using ConnectAsync(Hostname) and let the system select the locally bound ip/port
            */

            pSocket->bConnectAsyncInProgress = TRUE;
            task<void>(dynamic_cast<StreamSocket^>(pSocket->refSock)->ConnectAsync(refRemoteHostName, refRemoteServNameStr, SocketProtectionLevel::PlainSocket)).then([pSocket] (task<void> previousTask)
            {
                try
                {
                    // try getting an exception.
                    previousTask.get();
                    pSocket->refDataWriter = ref new DataWriter(dynamic_cast<StreamSocket^>(pSocket->refSock)->OutputStream);
                    pSocket->refDataReader = ref new DataReader(dynamic_cast<StreamSocket^>(pSocket->refSock)->InputStream);

                    // set "partial" option: the asynchronous read operation should complete as soon as a minimum number of bytes are available.
                    pSocket->refDataReader->InputStreamOptions = InputStreamOptions::Partial;

                    // mark the socket as opened
                    pSocket->iOpened = 1;
                }
                catch (Exception^ exception)
                {
                    NetPrintf(("dirtynetwinrt: [%p] (stream socket) StreamSocket::ConnectAsync(HostName) failed: %S\n", pSocket, exception->ToString()->Data()));

                    // mark the socket in failure
                    pSocket->iOpened = -1;
                }

                pSocket->bConnectAsyncInProgress = FALSE;
            });
        }
    }
    else
    {
        NetPrintf(("dirtynetwinrt: [%p] critical error - SocketConnect() does not support type %d\n", pSocket, pSocket->iType));
        pSocket->iLastError = SOCKERR_UNSUPPORT;
    }

    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketListen

    \Description
        Start listening for an incoming connection on the socket.  The socket must already
        be bound and a stream oriented connection must be in use.

    \Input *pSocket - socket reference to bound socket (see SocketBind())
    \Input iBacklog - number of pending connections allowed

    \Output
        int32_t         - standard network error code (SOCKERR_xxx)

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketListen(SocketT *pSocket, int32_t iBacklog)
{
    pSocket->iLastError = SOCKERR_NONE;

    if ((pSocket->iType == SOCK_STREAM) && (pSocket->un.streamSpace.bSocketListenCalled == FALSE))
    {
        pSocket->un.streamSpace.bSocketListenCalled = TRUE;
        pSocket->un.streamSpace.iMaxPendingConnections = iBacklog;

        // allocate the vector used to store incoming connections
        pSocket->refStreamConnectVect = ref new Platform::Collections::Vector<StreamSocket ^>();

        //replace the StreamSock with a StreamSockListener
        delete pSocket->refSock;
        pSocket->refSock = nullptr;

        // create stream socket listener object
        auto refListenSock = ref new StreamSocketListener();
        pSocket->refSock = refListenSock;

        // register for inbound connection events
        pSocket->refSockEventCtx = ref new StreamSocketListenerContext(refListenSock);
        refListenSock->ConnectionReceived += ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>(dynamic_cast<StreamSocketListenerContext ^>(pSocket->refSockEventCtx), &StreamSocketListenerContext::OnConnection);

        auto refLocalServNameStr = _SocketGetServiceNameRefFromSockaddrIn(&pSocket->bindAddr);
        auto refLocalHostName = _SocketGetHostNameRefFromSockaddrIn(&pSocket->bindAddr);

        // start async bind and listen
        pSocket->un.streamSpace.bBindListenerAsyncInProgress = TRUE;
        task<void>(refListenSock->BindEndpointAsync(refLocalHostName, refLocalServNameStr)).then([pSocket] (task<void> previousTask)
        {
            try
            {
                // try getting all exceptions from the continuation chain above this point.
                previousTask.get();
            }
            catch (Exception^ exception)
            {
                NetPrintf(("dirtynetwinrt: [%p] StreamSocketListener::BindEndpointAsync() failed: %S\n", pSocket, exception->ToString()->Data()));
            }

            pSocket->un.streamSpace.bBindListenerAsyncInProgress = FALSE;
        });
    }
    else
    {
        pSocket->iLastError = SOCKERR_INVALID;
    }

    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketAccept

    \Description
        Accept an incoming connection attempt on a socket.

    \Input *pSocket     - socket reference to socket in listening state (see SocketListen())
    \Input *pAddr       - pointer to storage for address of the connecting entity, or NULL
    \Input *pAddrLen    - pointer to storage for length of address, or NULL

    \Output
        SocketT *   - the accepted socket, or NULL if not available

    \Notes
        The integer pointed to by addrlen should on input contain the number of characters
        in the buffer addr.  On exit it will contain the number of characters in the
        output address.

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *pAddr, int32_t *pAddrLen)
{
    SocketT *pAcceptedSocket = NULL;

    pSocket->iLastError = SOCKERR_NONE;

    if ((pSocket->iType == SOCK_STREAM) && 
        (pSocket->un.streamSpace.bSocketListenCalled == TRUE) &&
        (pAddr != NULL) &&
        (*pAddrLen >= sizeof(struct sockaddr)))
    {
        // acquire socket critical section
        NetCritEnter(&pSocket->sockCrit);

        uint32_t uVectorSize = pSocket->refStreamConnectVect->Size;

        // release socket critical section
        NetCritLeave(&pSocket->sockCrit);

        // make sure there are connections waiting to be accepted 
        if (uVectorSize != 0)
        {
            // create a SocketT and assign the first StreamSocket in the vector to it (last argument is 1 to flag the socket as "opened/connected")
            pAcceptedSocket = _SocketOpen(pSocket->refStreamConnectVect->GetAt(0), pSocket->iFamily, pSocket->iType, pSocket->iProto, 1);

            pAcceptedSocket->refDataWriter = ref new DataWriter(dynamic_cast<StreamSocket^>(pAcceptedSocket->refSock)->OutputStream);
            pAcceptedSocket->refDataReader = ref new DataReader(dynamic_cast<StreamSocket^>(pAcceptedSocket->refSock)->InputStream);

            // set "partial" option: the asynchronous read operation should complete as soon as a minimum number of bytes are available.
            pAcceptedSocket->refDataReader->InputStreamOptions = InputStreamOptions::Partial;

            if (pAcceptedSocket != NULL)
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->sockCrit);

                // remove that connection entry from the vector
                pSocket->refStreamConnectVect->RemoveAt(0);

                NetPrintf(("dirtynetwinrt: [%p, %p] SocketAccept() successful, count of pending connections decremented to %d\n",
                    pSocket, pAcceptedSocket, pSocket->refStreamConnectVect->Size));

                // release socket critical section
                NetCritLeave(&pSocket->sockCrit);
            }
        }
    }
    else
    {
        pSocket->iLastError = SOCKERR_INVALID;
    }

    return(pAcceptedSocket);
}

/*F*************************************************************************************/
/*!
    \Function    SocketSendto

    \Description
        Send data to a remote host. The destination address is supplied along with
        the data. Should only be used with datagram sockets as stream sockets always
        send to the connected peer.

    \Input *pSocket - socket reference
    \Input *pBuf    - the data to be sent
    \Input iLen     - size of data
    \Input iFlags   - unused
    \Input *pTo     - the address to send to (NULL=use connection address)
    \Input iToLen   - length of address

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        For unconnected DGRAM sockets, the first call to SocketSendto for a given peer
        address will always return SOCKERR_NONE because there is implicit internal delay
        required to obtain a dedicated output stream asynchronously. The following calls
        to SocketSendto() will start succeeding when this async op is completed.

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketSendto(SocketT *pSocket, const char *pBuf, int32_t iLen, int32_t iFlags, const struct sockaddr *pTo, int32_t iToLen)
{
    SocketStateT *pState = _Socket_pState;

    pSocket->iLastError = SOCKERR_NONE;

    if (pSocket->bStoreAsyncInProgress)
    {
        return(pSocket->iLastError); // SOCKERR_NONE - no byte sent
    }

    // if installed, give socket callback right of first refusal
    if (pState->pSendCallback != NULL)
    {
        int32_t iResult;
        if ((iResult = pState->pSendCallback(pSocket, pSocket->iType, (const uint8_t *)pBuf, iLen, pTo, pState->pSendCallref)) > 0)
        {
            return(iResult);
        }
    }

    // make sure socket ref is valid
    if (pSocket->refSock == nullptr)
    {
        NetPrintf(("dirtynetwinrt: [%p] attempting to send on invalid socket\n", pSocket));
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // use appropriate version
    if (pTo == NULL)
    {
        // make sure the socket is connected
        if ((pSocket->bTypeConnected) && (pSocket->iOpened > 0))
        {
            _SocketSendData(pSocket, pBuf, iLen, pSocket->refDataWriter);
        }
        else
        {
            NetPrintf(("dirtynetwinrt: [%p] SocketSendto() with a null pTo incorrectly used with an unconnected socket\n", pSocket));
            pSocket->iLastError = SOCKERR_NOTCONN;
        }
    }
    else
    {
        if ((pSocket->iType == SOCK_DGRAM) && !pSocket->bTypeConnected)
        {
            DataWriter ^refDataWriter = _DatagramSocketAsyncGetOutputStream(pSocket, pTo);

            // use data writer if valid, else try again later
            if (refDataWriter != nullptr)
            {
                _SocketSendData(pSocket, pBuf, iLen, refDataWriter);
            }
        }
        else
        {
            NetPrintf(("dirtynetwinrt: [%p] SocketSendto() with a non-null pTo can only be used with an unconnected SOCK_DGRAM socket\n", pSocket));
            pSocket->iLastError = SOCKERR_INVALID;
        }
    }

    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketRecvfrom

    \Description
        Receive data from a remote host. If socket is a connected stream, then data can
        only come from that source. A datagram socket can receive from any remote host.

    \Input *pSocket     - socket reference
    \Input *pBuf        - buffer to receive data
    \Input iLen         - length of recv buffer
    \Input iFlags       - unused
    \Input *pFrom       - address data was received from (NULL=ignore)
    \Input *pFromLen    - length of address

    \Output
        int32_t         - positive=data bytes received, else standard error code

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pFrom, int32_t *pFromLen)
{
    pSocket->iLastError = SOCKERR_NONE;

    if (pSocket->iType == SOCK_DGRAM)
    {
        // see if we need to consume a pending datagram from the inbound queue
        if (pSocket->un.dgramSpace.iRecvStat == 0)
        {
            pSocket->iLastError = _DatagramSocketConsumeInboundMsgEvent(pSocket);
        }

        // see if we have any data
        if ((pSocket->un.dgramSpace.iRecvStat > 0) && (iLen > 0))
        {
            pSocket->iLastError = pSocket->un.dgramSpace.iRecvStat;

            // copy sender
            if (pFrom != NULL)
            {
                memcpy(pFrom, &pSocket->recvAddr, sizeof(*pFrom));
                *pFromLen = sizeof(*pFrom);
            }

            // make sure we fit in the buffer
            if (pSocket->iLastError > iLen)
            {
                pSocket->iLastError = iLen;
            }
            memcpy(pBuf, pSocket->un.dgramSpace.aRecvData, (unsigned)pSocket->iLastError);

            // mark data as consumed
            pSocket->un.dgramSpace.iRecvStat= 0;
        }
    }
    else if (pSocket->iType == SOCK_STREAM)
    {
        if (pSocket->refDataReader->UnconsumedBufferLength)
        {
            uint32_t uReadSize;
            if (pSocket->refDataReader->UnconsumedBufferLength > (unsigned)iLen)
            {
                uReadSize = (uint32_t)iLen;
            }
            else
            {
                uReadSize = pSocket->refDataReader->UnconsumedBufferLength;
            }

            try
            {
                pSocket->refDataReader->ReadBytes(ArrayReference<uint8_t>((uint8_t *)pBuf, uReadSize));

                // copy sender
                if (pFrom != NULL)
                {
                    memcpy(pFrom, &pSocket->remoteAddr, sizeof(*pFrom));
                    *pFromLen = sizeof(*pFrom);
                }

                // return number of bytes read
                pSocket->iLastError = (int32_t)uReadSize;
            }
            catch (Exception^ exception)
            {
                pSocket->iLastError = _XlatError(SocketError::GetStatus(exception->HResult));
                NetPrintf(("dirtynetwinrt: [%p] DataReader::ReadBytes() failed: %S\n", pSocket, exception->ToString()->Data()));
            }
        }
    }
    else
    {
        NetPrintf(("dirtynetwinrt: [%p] critical error - SocketRecvfrom() does not support type %d\n", pSocket, pSocket->iType));
        pSocket->iLastError = SOCKERR_UNSUPPORT;
    }

    // simulate packet loss
    #if DIRTYCODE_DEBUG
    if ((_Socket_pState->uPacketLoss != 0) && (pSocket->iType == SOCK_DGRAM) && (pSocket->iLastError > 0) && (SocketSimulatePacketLoss(_Socket_pState->uPacketLoss) != 0))
    {
        pSocket->iLastError = SOCKERR_NONE;
    }
    #endif

    // acquire socket critical section
    NetCritEnter(&pSocket->sockCrit);

    // return error code from async op
    if (pSocket->iLastError == SOCKERR_NONE && pSocket->iRecvErr < 0)
    {
        pSocket->iLastError = pSocket->iRecvErr;
    }

    // release socket critical section
    NetCritLeave(&pSocket->sockCrit);

    // return the error code
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketInfo

    \Description
        Return information about an existing socket.

    \Input *pSocket - socket reference
    \Input iInfo    - selector for desired information
    \Input iData    - selector specific
    \Input *pBuf    - [out] return buffer
    \Input iLen     - buffer length

    \Output
        int32_t     - size of returned data or error code (negative value)

    \Notes
        iInfo can be one of the following:

        \verbatim
            'addr' - return local address
            'conn' - who we are connecting to
            'bind' - return bind data (if pSocket == NULL, get socket bound to given port)
            'bndu' - return bind data (only with pSocket=NULL, get SOCK_DGRAM socket bound to given port)
            'ladr' - get local address previously specified by user for subsequent SocketBind() operations
            'maxp' - return max packet size
            'peer' - peer info (only valid if connected)
            'sdcf' - get installed global send callback function pointer
            'sdcu' - get installed global send callback userdata pointer
            'serr' - last socket error
            'sock' - return windows socket associated with the specified DirtySock socket
            'stat' - TRUE if connected, else FALSE
            'virt' - TRUE if socket is virtual, else FALSE
        \endverbatim

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;

    // always zero results by default
    if ( (pBuf != NULL) && (iInfo != 'ladr') )
    {
        memset(pBuf, 0, (unsigned)iLen);
    }

    // handle global socket options
    if (pSocket == NULL)
    {
        if (iInfo == 'addr')
        {
            return((signed)SocketGetLocalAddr());
        }
        // get socket bound to given port
        if ( (iInfo == 'bind') || (iInfo == 'bndu') )
        {
            struct sockaddr BindAddr;
            int32_t iFound = -1;

            // for access to socket list
            NetCritEnter(NULL);

            // walk socket list and find matching socket
            for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
            {
                // if iInfo is 'bndu', only consider sockets of type SOCK_DGRAM
                // note: 'bndu' stands for "bind udp"
                if ( (iInfo == 'bind') || ((iInfo == 'bndu') && (pSocket->iType == SOCK_DGRAM)) )
                {
                    // get socket info
                    SocketInfo(pSocket, 'bind', 0, &BindAddr, sizeof(BindAddr));
                    if (SockaddrInGetPort(&BindAddr) == iData)
                    {
                        *(SocketT **)pBuf = pSocket;
                        iFound = 0;
                        break;
                    }
                }
            }

            // for access to g_socklist and g_sockkill
            NetCritLeave(NULL);
            return(iFound);
        }
        // get local address previously specified by user for subsequent SocketBind() operations
        if (iInfo == 'ladr')
        {
            // If 'ladr' had not been set previously, address field of output sockaddr buffer
            // will just be filled with 0.
            SockaddrInSetAddr((struct sockaddr *)pBuf, pState->uAdapterAddress);
            return(0);
        }
        // return max packet size
        if (iInfo == 'maxp')
        {
            return(pState->iMaxPacket);
        }

        // get global send callback function pointer
        if (iInfo == 'sdcf')
        {
            if ((pBuf != NULL) && (iLen == sizeof(pState->pSendCallback)))
            {
                memcpy(pBuf, &pState->pSendCallback, sizeof(pState->pSendCallback));
                return(0);
            }
        }
        // get global send callback user data pointer
        if (iInfo == 'sdcu')
        {
            if ((pBuf != NULL) && (iLen == sizeof(pState->pSendCallref)))
            {
                memcpy(pBuf, &pState->pSendCallref, sizeof(pState->pSendCallref));
                return(0);
            }
        }

        NetPrintf(("dirtynetwinrt: [%p] unhandled SocketInfo() selector '%C'\n", pSocket, iInfo));

        return(-1);
    }

    // return local bind data
    if (iInfo == 'bind')
    {
        int32_t iResult = 0; // default to success
        struct sockaddr *pAddr = (struct sockaddr *)pBuf;

        if (pSocket->iType == SOCK_DGRAM)
        {
            if (pSocket->un.dgramSpace.bVirtual == TRUE)
            {
                SockaddrInit((struct sockaddr *)pBuf, AF_INET);
                SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->un.dgramSpace.uVirtualPort);
            }
            else
            {
                if (dynamic_cast<DatagramSocket^>(pSocket->refSock)->Information->LocalAddress != nullptr)
                {
                    _SocketSetSockaddrInAddrFromHostName(pAddr, dynamic_cast<DatagramSocket^>(pSocket->refSock)->Information->LocalAddress);
                    _SocketSetSockaddrInPortFromServiceName(pAddr, dynamic_cast<DatagramSocket^>(pSocket->refSock)->Information->LocalPort);
                }
                else
                {
                    // socket not yet bound
                    NetPrintf(("dirtynetwinrt: [%p] 'bind' selector used on an unbound datagram socket\n", pSocket));
                    iResult = -1; // failure
                }
            }
        }
        else if (pSocket->iType == SOCK_STREAM)
        {
            if (pSocket->un.streamSpace.bSocketListenCalled)
            {
                if (!dynamic_cast<StreamSocketListener^>(pSocket->refSock)->Information->LocalPort->IsEmpty())
                {
                    _SocketSetSockaddrInPortFromServiceName(pAddr, dynamic_cast<StreamSocketListener^>(pSocket->refSock)->Information->LocalPort);
                }
                else
                {
                    // socket not yet bound
                    NetPrintf(("dirtynetwinrt: [%p] 'bind' selector used un an unbound stream socket (listening)\n", pSocket));
                    iResult = -1; // failure
                }
            }
            else
            {
                if (dynamic_cast<StreamSocket^>(pSocket->refSock)->Information->LocalAddress != nullptr)
                {
                    _SocketSetSockaddrInAddrFromHostName(pAddr, dynamic_cast<StreamSocket^>(pSocket->refSock)->Information->LocalAddress);
                    _SocketSetSockaddrInPortFromServiceName(pAddr, dynamic_cast<StreamSocket^>(pSocket->refSock)->Information->LocalPort);
                }
                else
                {
                    // socket not yet bound
                    NetPrintf(("dirtynetwinrt: [%p] 'bind' selector used un an unbound stream socket\n", pSocket));
                    iResult = -1; // failure
                }
            }
        }
        else
        {
            NetPrintf(("dirtynetwinrt: [%p] socket type %d not supported with 'bind'\n", pSocket, pSocket->iType));
            iResult = -1; // failure
        }

        return(iResult);
    }

    // return socket protocol
    if (iInfo == 'prot')
    {
        return(pSocket->iProto);
    }

    // return whether the socket is virtual or not
    if (iInfo == 'virt')
    {
        if (pSocket->iType == SOCK_DGRAM)
        {
            return(pSocket->un.dgramSpace.bVirtual);
        }
        else
        {
            return(FALSE);
        }
    }

    // make sure we are connected
    if (pSocket->refSock == nullptr)
    {
        pSocket->iLastError = SOCKERR_OTHER;
        return(pSocket->iLastError);
    }

    // return peer info (only valid if connected)
    if ((iInfo == 'conn') || (iInfo == 'peer'))
    {
        int32_t iResult = 0; // default to success
        if (pSocket->bTypeConnected)
        {
            if (pSocket->iOpened > 0)
            {
                memcpy(pBuf, &pSocket->remoteAddr, (unsigned)iLen);
            }
            else
            {
                NetPrintf(("dirtynetwinrt: [%p] 'conn'/'peer' failed because socket not yet connected\n", pSocket));
                iResult = -1;
            }
        }
        else
        {
            NetPrintf(("dirtynetwinrt: [%p] 'conn'/'peer' used on an'unconnected' socket\n", pSocket));
            iResult = -1;
        }
        return(iResult);
    }

    // return last socket error
    if (iInfo == 'serr')
    {
        return(pSocket->iLastError);
    }

    // return windows socket identifier
    if (iInfo == 'sock')
    {
        // can't pass managed reference to unmanaged code
        NetPrintf(("dirtynetwinrt: [%p] 'sock' info not supported on WinRT\n", pSocket));
        return(-1);
    }

    // return socket status
    if (iInfo == 'stat')
    {
        int32_t iResult = 1;  // default to connected

        // if not a connected socket, always return 1
        if (pSocket->bTypeConnected)
        {
            if ((pSocket->iType == SOCK_STREAM) && (pSocket->iOpened > 0) &&
                (dynamic_cast<StreamSocket^>(pSocket->refSock)->Information->RemoteAddress == nullptr))
            {
                NetPrintf(("dirtynetwinrt: [%p] 'stat' selector marked stream socket as disconneted\n", pSocket));
                iResult = pSocket->iOpened = -1;
            }
            else
            {
                iResult = pSocket->iOpened;
            }
        }

        return(iResult);
    }

    // unhandled
    NetPrintf(("dirtynetwinrt: [%p] unhandled SocketInfo() selector '%C'\n", pSocket, iInfo));

    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function    SocketCallback

    \Description
        Register a callback routine for notification of socket events.  Also includes
        timeout support.

    \Input *pSocket     - socket reference
    \Input iMask        - valid callback events (CALLB_NONE, CALLB_SEND, CALLB_RECV)
    \Input iIdle        - if nonzero, specifies the number of ticks between idle calls
    \Input *pRef        - user data to be passed to proc
    \Input *pProc       - user callback

    \Output
        int32_t     - zero

    \Notes
        A callback will reset the idle timer, so when specifying a callback and an
        idle processing time, the idle processing time represents the maximum elapsed
        time between calls.

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
#pragma optimize("", off) // make sure this block of code is not reordered
int32_t SocketCallback(SocketT *pSocket, int32_t iMask, int32_t iIdle, void *pRef, int32_t (*pProc)(SocketT *pSocket, int32_t iFlags, void *pRef))
{
    pSocket->uCallIdle = (unsigned)iIdle;
    pSocket->iCallMask = iMask;
    pSocket->pCallRef = pRef;
    pSocket->pCallback = pProc;
    return(0);
}
#pragma optimize("", on)

/*F*************************************************************************************/
/*!
    \Function    SocketControl

    \Description
        Process a control message (type specific operation)

    \Input *pSocket - socket to control, or NULL for module-level option
    \Input iOption  - the option to pass
    \Input iData1   - message specific parm
    \Input *pData2  - message specific parm
    \Input *pData3  - message specific parm

    \Output
        int32_t     - message specific result (-1=unsupported message, -2=no such module)

    \Notes
        iOption can be one of the following:

        \verbatim
            'keep' - enable/disablet TCP keep-alive (STREAM sockets only) (PC only)
            'ladr' - set local address for subsequent SocketBind() operations
            'loss' - simulate packet loss (debug only)
            'maxp' - set max udp packet size
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'poll' - execute blocking wait on given socket list (pData2) or all sockets (pData2=NULL), 64 max sockets
            'push' - push data into given socket (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'sdcb' - set send callback (pData2=callback, pData3=callref)
            'vadd' - add a port to virtual port list
            'vdel' - del a port from virtual port list
        \endverbatim

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketControl(SocketT *pSocket, int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    SocketStateT *pState = _Socket_pState;

    // warn about selectors not supported on winrt
    if ( (iOption == 'keep') || (iOption == 'nbio') || (iOption == 'ndly') || (iOption == 'poll') || 
         (iOption == 'radr') || (iOption == 'rbuf') || (iOption == 'sbuf') )
    {
        NetPrintf(("dirtynetwinrt: [%p] '%C' control not supported on WinRT\n", pSocket, iOption));
        return(-1);
    }

    // set local address? (used to select between multiple network interfaces)
    if (iOption == 'ladr')
    {
        pState->uAdapterAddress = (unsigned)iData1;
        return(0);
    }
        // set up packet loss simulation
    #if DIRTYCODE_DEBUG
    if (iOption == 'loss')
    {
        pState->uPacketLoss = (unsigned)iData1;
        NetPrintf(("dirtynetwinrt: packet loss simulation %s (param=0x%08x)\n", pState->uPacketLoss ? "enabled" : "disabled", pState->uPacketLoss));
        return(0);
    }
    #endif

    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetwinrt: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }

    // push data into receive buffer
    if (iOption == 'push')
    {
        if (pSocket != NULL && (pSocket->iType == SOCK_DGRAM))
        {
            // acquire socket critical section
            NetCritEnter(&pSocket->sockCrit);

            // don't allow data that is too large (for the buffer) to be pushed
            if (iData1 > (signed)sizeof(pSocket->un.dgramSpace.aRecvData))
            {
                NetPrintf(("dirtynetwinrt: [%p] request to push %d bytes of data discarded (max=%d)\n",
                    pSocket, iData1, sizeof(pSocket->un.dgramSpace.aRecvData)));
                return(-1);
            }
            
            // warn if there is already data present
            if (pSocket->un.dgramSpace.iRecvStat > 0)
            {
                NetPrintf(("dirtynetwinrt: [%p] warning - overwriting packet data with SocketControl('push')\n", pSocket));
            }

            // save the size and copy the data
            pSocket->un.dgramSpace.iRecvStat = iData1;
            memcpy(pSocket->un.dgramSpace.aRecvData, pData2, (unsigned)pSocket->un.dgramSpace.iRecvStat);

            // save the address
            memcpy(&pSocket->recvAddr, pData3, sizeof(pSocket->recvAddr));
            // save receive timestamp
            SockaddrInSetMisc(&pSocket->recvAddr, NetTick());

            // release socket critical section
            NetCritLeave(&pSocket->sockCrit);

            // see if we should issue callback
            if ((pSocket->pCallback != NULL) && (pSocket->iCallMask & CALLB_RECV))
            {
                pSocket->pCallback(pSocket, 0, pSocket->pCallRef);
            }
        }
        else
        {
            NetPrintf(("dirtynetwinrt: [%p] warning - call to SocketControl('push') ignored because pSocket is NULL or socket type is not SOCK_DGRAM\n", pSocket));
            return(-1);
        }
        return(0);
    }
    // set global send callback
    if (iOption == 'sdcb')
    {
        pState->pSendCallback = (SocketSendCallbackT *)pData2;
        pState->pSendCallref = pData3;
        return(0);
    }
    // mark a port as virtual
    if (iOption == 'vadd')
    {
        int32_t iPort;

        // find a slot to add virtual port
        for (iPort = 0; pState->aVirtualPorts[iPort] != 0; iPort++)
            ;
        if (iPort < SOCKET_MAXVIRTUALPORTS)
        {
            pState->aVirtualPorts[iPort] = (uint16_t)iData1;
            return(0);
        }
    }
    // remove port from virtual port list
    if (iOption == 'vdel')
    {
        int32_t iPort;

        // find virtual port in list
        for (iPort = 0; (iPort < SOCKET_MAXVIRTUALPORTS) && (pState->aVirtualPorts[iPort] != (uint16_t)iData1); iPort++)
            ;
        if (iPort < SOCKET_MAXVIRTUALPORTS)
        {
            pState->aVirtualPorts[iPort] = 0;
            return(0);
        }
    }

    // unhandled
    NetPrintf(("dirtynetwinrt: [%p] unhandled control option '%C'\n", pSocket, iOption));

    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function    SocketGetLocalAddr

    \Description
        Returns the "external" local address (ie, the address as a machine "out on
        the Internet" would see as the local machine's address).

    \Output
        uint32_t        - local address

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
uint32_t SocketGetLocalAddr(void)
{
    struct sockaddr inetAddr, hostAddr;

    // create a remote internet address
    memset(&inetAddr, 0, sizeof(inetAddr));
    inetAddr.sa_family = AF_INET;
    SockaddrInSetPort(&inetAddr, 79);
    SockaddrInSetAddr(&inetAddr, 0xc0a80101);

    // ask socket to give us local address that one can connect to
    memset(&hostAddr, 0, sizeof(hostAddr));
    SocketHost(&hostAddr, sizeof(hostAddr), &inetAddr, sizeof(inetAddr));

    return((uint32_t)SockaddrInGetAddr(&hostAddr));
}

/*F*************************************************************************************/
/*!
    \Function    SocketLookup

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *pText    - pointer to null terminated address string
    \Input iTimeout  - number of milliseconds to wait for completion

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
HostentT *SocketLookup(const char *pText, int32_t iTimeout)
{
    SocketStateT *pState = _Socket_pState;
    SocketLookupPrivT *pPriv;
    int32_t iAddr;
    HostentT *pHost;

    // dont allow negative timeouts
    if (iTimeout < 0)
    {
        return(NULL);
    }

    // create new structure
    pPriv = (SocketLookupPrivT *)DirtyMemAlloc(sizeof(*pPriv), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(pPriv, 0, sizeof(*pPriv));
    pHost = &pPriv->host;

    // we cache the memgroup and user data in case the async lookup completion handler
    // ends up using them after the socket module is destroyed
    pPriv->iMemGroup = pState->iMemGroup;
    pPriv->pMemGroupUserData = pState->pMemGroupUserData;

    // setup callbacks
    pHost->Done = &_SocketLookupDone;
    pHost->Free = &_SocketLookupFree;

    // check for dot notation
    if ((iAddr = SocketInTextGetAddr(pText)) != 0)
    {
        // we've got a dot-notation address
        pHost->addr = iAddr;
        pHost->done = 1;
        // return completed record
        return(pHost);
    }

    // copy over the target address
    ds_strnzcpy(pHost->name, pText, sizeof(pHost->name));

    // initialize host name (ip addr) name managed object
    std::string strName;
    strName.assign(pHost->name);
    std::wstring wstrName; 
    wstrName.assign(strName.begin(), strName.end());

    auto refRemoteHostNameStr = ref new String(wstrName.data());
    auto refRemoteHostName = ref new HostName(refRemoteHostNameStr);

    // start async get endpoint pair
    pPriv->refDgramSock = ref new DatagramSocket();
    task<Windows::Foundation::Collections::IVectorView<EndpointPair^>^>(pPriv->refDgramSock->GetEndpointPairsAsync(refRemoteHostName, L"0")).then([pPriv](
    task<Windows::Foundation::Collections::IVectorView<EndpointPair^>^> previousTask)
    {
        try
        {
            // loop through the result vector
            auto it = previousTask.get()->First();
            while (it->HasCurrent)
            {
                if (it->Current->RemoteHostName != nullptr)
                {
                    // there are 4 different possible hostname types on winrt: ipv4, ipv6, domainname, bluetooth
                    // make sure we deal with an ipv4 type here
                    if (it->Current->RemoteHostName->Type == HostNameType::Ipv4)
                    {
                        char strIpAddr[20];

                        String^ ipString = it->Current->RemoteHostName->CanonicalName;
                        memset(strIpAddr, 0, sizeof(strIpAddr));
                        std::wcstombs(strIpAddr, ipString->Data(), sizeof(strIpAddr));

                        pPriv->host.addr = (unsigned)SocketInTextGetAddr(strIpAddr);

                        // mark success
                        pPriv->host.done = 1;

                        break;
                    }
                }
                it->MoveNext();
            }

            if (pPriv->host.done == 1)
            {
                NetPrintf(("dirtynetwinrt: %s=%a\n", pPriv->host.name, pPriv->host.addr));
            }
            else
            {
                NetPrintf(("dirtynetwinrt: failed to resolve %s=\n", pPriv->host.name));
                pPriv->host.done = -1;
            }
        }
        catch (Exception^ exception)
        {
            pPriv->host.done = -1;
            NetPrintf(("dirtynetwinrt: DatagramSocket::GetEndpointPairAsync(%s) failed: %S\n", pPriv->host.name, exception->ToString()->Data()));
        }
        catch (task_canceled&)
        {
            pPriv->host.done = -1;
            NetPrintf(("dirtynetwinrt: DatagramSocket::GetEndpointPairAsync(%s) was canceled\n", pPriv->host.name));
        }

        delete pPriv->refDgramSock;
        pPriv->refDgramSock = nullptr;

        // pPriv->host.thread is accessed atomically and used to determine who
        // has to free pPriv, the main thread or the async op completion handler
        if (InterlockedExchange((volatile LONG *)&pPriv->host.thread, 1) != 0)
        {
            // pPriv->host.thread was already marked by the main thread
            // this means that the main thread assumed that we would free pPriv
            DirtyMemFree(pPriv, SOCKET_MEMID, pPriv->iMemGroup, pPriv->pMemGroupUserData);
        }
    });

    // return the host reference
    return(pHost);
}

/*F*************************************************************************************/
/*!
    \Function    SocketHost

    \Description
        Return the host address that would be used in order to communicate with the given
        destination address.

    \Input *pHost    - local sockaddr struct
    \Input iHostLen  - length of structure (sizeof(host))
    \Input *pDest    - remote sockaddr struct
    \Input iDestLen  - length of structure (sizeof(dest))

    \Output
        int32_t     - zero=success, negative=error

    \Version 10/30/2012 (mclouatre)
*/
/************************************************************************************F*/
int32_t SocketHost(struct sockaddr *pHost, int32_t iHostLen, const struct sockaddr *pDest, int32_t iDestLen)
{
    SocketStateT *pState = _Socket_pState;

    // must be same kind of addresses
    if (iHostLen != iDestLen)
    {
        return(-1);
    }

    // do family specific lookup
    if (pDest->sa_family == AF_INET)
    {
        // make them match initially
        memcpy(pHost, pDest, (unsigned)iHostLen);

        // respect adapter override
        if (pState->uAdapterAddress != 0)
        {
            SockaddrInSetAddr(pHost, pState->uAdapterAddress);
            return(0);
        }

        // zero the address portion
        pHost->sa_data[2] = pHost->sa_data[3] = pHost->sa_data[4] = pHost->sa_data[5] = 0;

        auto hostNames = NetworkInformation::GetHostNames();
        for (auto it = hostNames->First(); it->HasCurrent; it->MoveNext())
        {
            if (it->Current->IPInformation != nullptr)
            {
                // there are 4 different possible hostname types on winrt: ipv4, ipv6, domainname, bluetooth
                // make sure we deal with an ipv4 type here
                if (it->Current->Type == HostNameType::Ipv4)
                {
                    _SocketSetSockaddrInAddrFromHostName(pHost, it->Current);
                    break;
                }
            }
        }
        return(0);
    }

    // unsupported family
    memset(pHost, 0, (unsigned)iHostLen);
    return(-2);
}
