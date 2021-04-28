///////////////////////////////////////////////////////////////////////////////
// IGOIPCUtils.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOIPCUtils.h"


#if defined(ORIGIN_PC)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define FileDirectoryInformation 1   
#define STATUS_NO_MORE_FILES 0x80000006L   

typedef struct   
{    
	USHORT Length;  
	USHORT MaximumLength;  
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;   

typedef struct   
{    
	LONG Status;
	ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;   

typedef struct {   
    ULONG NextEntryOffset;   
    ULONG FileIndex;   
    LARGE_INTEGER CreationTime;   
    LARGE_INTEGER LastAccessTime;   
    LARGE_INTEGER LastWriteTime;   
    LARGE_INTEGER ChangeTime;   
    LARGE_INTEGER EndOfFile;   
    LARGE_INTEGER AllocationSize;   
    ULONG FileAttributes;   
    ULONG FileNameLength;   
    union {   
        struct {   
            WCHAR FileName[1];   
        } FileDirectoryInformationClass;   

        struct {      
            WCHAR FileName[1];   
        } FileFullDirectoryInformationClass;   

        struct {        
            WCHAR FileName[1];   
    } FileBothDirectoryInformationClass;   
    };   
} FILE_QUERY_DIRECTORY, *PFILE_QUERY_DIRECTORY;   


// ntdll!NtQueryDirectoryFile (NT specific!)   
//   
// The function searches a directory for a file whose name and attributes   
// match those specified in the function call.   
//   
// NTSYSAPI   
// NTSTATUS   
// NTAPI   
// NtQueryDirectoryFile(   
//    IN HANDLE FileHandle,                      // handle to the file   
//    IN HANDLE EventHandle OPTIONAL,   
//    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,   
//    IN PVOID ApcContext OPTIONAL,   
//    OUT PIO_STATUS_BLOCK IoStatusBlock,   
//    OUT PVOID Buffer,                          // pointer to the buffer to receive the result   
//    IN ULONG BufferLength,                     // length of Buffer   
//    IN FILE_INFORMATION_CLASS InformationClass,// information type   
//    IN BOOLEAN ReturnByOne,                    // each call returns info for only one file   
//    IN PUNICODE_STRING FileTemplate OPTIONAL,  // template for search   
//    IN BOOLEAN Reset                           // restart search   
// );   
typedef LONG (WINAPI *PROCNTQDF)( HANDLE,HANDLE,PVOID,PVOID,PIO_STATUS_BLOCK,PVOID,ULONG,   
                                  UINT,BOOL,PUNICODE_STRING,BOOL );   

static PROCNTQDF NtQueryDirectoryFile;   

void IGOIPCGetNamedPipes(eastl::list<uint32_t> &pipes)
{
    LONG ntStatus;   
    IO_STATUS_BLOCK IoStatus;   
    BOOL bReset = TRUE;   
    PFILE_QUERY_DIRECTORY DirInfo, TmpInfo;

    pipes.clear();

    if (!NtQueryDirectoryFile)
        NtQueryDirectoryFile = (PROCNTQDF)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQueryDirectoryFile");

    if (!NtQueryDirectoryFile)   
        return;   

    HANDLE hPipe = CreateFile(L"\\\\.\\Pipe\\",GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL,OPEN_EXISTING,0,NULL);   

    if (hPipe == INVALID_HANDLE_VALUE)   
        return;   

    BYTE *DirInfoData = new BYTE[sizeof(FILE_QUERY_DIRECTORY)*64];
    memset(DirInfoData, 0, sizeof(FILE_QUERY_DIRECTORY)*64);
    DirInfo = (PFILE_QUERY_DIRECTORY) DirInfoData;

    //printf("Pipe name (Number of instances, Maximum instances)\n\n");   
    while (1)   
    {   
        ntStatus = NtQueryDirectoryFile(hPipe, NULL, NULL, NULL, &IoStatus, DirInfo, sizeof(FILE_QUERY_DIRECTORY)*64,   
                                        FileDirectoryInformation, FALSE, NULL, bReset);   

        if (ntStatus == NO_ERROR) 
        {   
            TmpInfo = DirInfo;   
            while (1)   
            {
                // Store old values before we mangle the buffer
                const int endStringAt = TmpInfo->FileNameLength/sizeof(WCHAR);
                const WCHAR oldValue = TmpInfo->FileDirectoryInformationClass.FileName[endStringAt];

                // Place a null character at the end of the string so wprintf doesn't read past the end
                TmpInfo->FileDirectoryInformationClass.FileName[endStringAt] = NULL;   

                //wprintf(L"%s (%d, %d)\n",TmpInfo->FileDirectoryInformationClass.FileName,   
                //                        TmpInfo->EndOfFile.LowPart,   
                //                        TmpInfo->AllocationSize.LowPart ); 

                // if we start with IGO_pipe_, we add the number to the list
                const WCHAR igoPipeName[] = L"IGO_pipe_";
                uint32_t igoPipeNameLen = (uint32_t)wcslen(igoPipeName);
                if (memcmp(TmpInfo->FileDirectoryInformationClass.FileName, igoPipeName, igoPipeNameLen*sizeof(WCHAR)) == 0)
                {
                    uint32_t pid = _wtoi(TmpInfo->FileDirectoryInformationClass.FileName + igoPipeNameLen);
                    pipes.push_back(pid);
                }

                // Restore the buffer to its correct state
                TmpInfo->FileDirectoryInformationClass.FileName[endStringAt] = oldValue;

                if (TmpInfo->NextEntryOffset==0)   
                    break;

                TmpInfo = (PFILE_QUERY_DIRECTORY)(reinterpret_cast<size_t>(TmpInfo)+TmpInfo->NextEntryOffset);   
            }   
        }
        else
        {
            // error or no more files
            break;
        }

        bReset = FALSE;   
   }   

    delete[] DirInfoData;
    if (hPipe != INVALID_HANDLE_VALUE)
        CloseHandle(hPipe);
}

#elif defined(ORIGIN_MAC) // MAC OSX ====================================================================

#include <signal.h>
#include <mach/notify.h>
#include <mach/message.h>
#include <mach/kern_return.h>
#include <servers/bootstrap.h>

#include "IGOTrace.h"


ConnectionID FuzzyConnectionID(const ConnectionID& id)
{
    return ConnectionID(id.pid); // FIXME - IMPLEMENT!
}

ConnectionID UnfuzzyConnectionID(const ConnectionID& id)
{
    return ConnectionID(id.pid); // FIXME - IMPLEMENT!
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

namespace Origin
{
    
    namespace Mac
    {
        
        const MachPort MachPort::_nullPort((mach_port_t)MACH_PORT_NULL);
        
        // Types used to send/receive data - we don't support trailers right now (no need)
        // Need to make sure this is large enough to hold the largest IGOIPCMessage
        // If you add a new format, make sure to update the "generic" MPRecvMsg function to use
        // the largest of the supported messages as the base message.
        enum MPMessageType
        {
            MPMessageType_SIMPLE,
            MPMessageType_WITH_SEND_RIGHTS,
            MPMessageType_WITH_OOL,
            // If you add a new format, make sure to update the "generic" MPRecvMsg function to use
            // the largest of the supported messages as the base message.
        };
        
        struct MPSendMsg
        {
            mach_msg_header_t header;
            uint32_t dataSize;
            char data[Origin::Mac::MachPort::MAX_MSG_SIZE];    
        };
        
        struct MPRecvMsg : public MPSendMsg
        {
            mach_msg_trailer_t trailer;    
        };
        
        struct MPSendRightsMsg
        {
            mach_msg_header_t           header;
            mach_msg_body_t             body;
            mach_msg_port_descriptor_t  portRights;
            uint32_t dataSize;
            char data[Origin::Mac::MachPort::MAX_MSG_SIZE];
        };
        
        struct MPRecvRightsMsg : public MPSendRightsMsg
        {
            mach_msg_trailer_t trailer;
        };
        
        struct MPSendOOLMsg
        {
            mach_msg_header_t           header;
            mach_msg_body_t             body;
            mach_msg_ool_descriptor_t   ool;
            uint32_t dataSize;
            char data[Origin::Mac::MachPort::MAX_MSG_SIZE];    
        };
        
        struct MPRecvOOLMsg : public MPSendOOLMsg
        {
            mach_msg_trailer_t trailer;
        };
        
        // Define descriptions for potential errors when using mach ports.
        struct MPErrorDescription
        {
            mach_msg_return_t errorID;
            const char* description;
        };
        
        MPErrorDescription MPCommErrorDescriptions[] =
        {
            // Receive errors
            { MACH_RCV_INVALID_NAME, "The specified receive_name was invalid." },
            { MACH_RCV_IN_SET, "The specified port was a member of a port set." },
            { MACH_RCV_TIMED_OUT, "The timeout interval expired." },
            { MACH_RCV_INTERRUPTED, "A software interrupt occurred." },
            { MACH_RCV_PORT_DIED, "The caller lost the rights specified by receive_name." },
            { MACH_RCV_PORT_CHANGED, "receive_name specified a receive right which was moved into a port set during the call." },
            { MACH_RCV_TOO_LARGE, "When using MACH_RCV_LARGE, the message was larger than receive_limit. The message is left queued, and its actual size is returned in the message header/message body." },
            { MACH_RCV_SCATTER_SMALL, "When using MACH_RCV_LARGE with MACH_RCV_OVERWRITE, one or more scatter list descriptors specified an overwrite region smaller than the corresponding incoming region. The message is left queued, and the proper descriptors are returned in the message header/message body." },
            { MACH_RCV_INVALID_TRAILER, "The trailer type desired, or the number of trailer elements desired, is not supported by the kernel." },
            { MACH_RCV_HEADER_ERROR, "A resource shortage prevented the reception of the port rights in the message header." },
            { MACH_RCV_INVALID_NOTIFY, "When using MACH_RCV_NOTIFY, the notify argument did not denote a valid receive right." },
            { MACH_RCV_INVALID_DATA, "The specified message buffer was not writable." },
            { MACH_RCV_TOO_LARGE, "When not using MACH_RCV_LARGE, a message larger than receive_limit was de-queued and destroyed." },
            { MACH_RCV_SCATTER_SMALL, "When not using MACH_RCV_LARGE with MACH_RCV_OVERWRITE, one or more scatter list descriptors specified an overwrite region smaller than the corresponding incoming region. The message was de-queued and destroyed." },
            { MACH_RCV_INVALID_TYPE, "When using MACH_RCV_OVERWRITE, one or more scatter list descriptors did not have the type matching the corresponding incoming message descriptor or had an invalid copy (disposition) field." },
            { MACH_RCV_BODY_ERROR, "A resource shortage prevented the reception of a port right or out-of- line memory region in the message body." },    
            
            // Send errors
            { MACH_SEND_MSG_TOO_SMALL, "The specified send_size was smaller than the minimum size for a message." },
            { MACH_SEND_NO_BUFFER, "A resource shortage prevented the kernel from allocating a message buffer." },
            { MACH_SEND_INVALID_DATA, "The supplied message buffer was not readable." },
            { MACH_SEND_INVALID_HEADER, "The msgh_bits value was invalid." },
            { MACH_SEND_INVALID_DEST, "The msgh_remote_port value was invalid." },
            { MACH_SEND_INVALID_NOTIFY, "When using MACH_SEND_CANCEL, the notify argument did not denote a valid receive right." },
            { MACH_SEND_INVALID_REPLY, "The msgh_local_port value was invalid." },
            { MACH_SEND_INVALID_TRAILER, "The trailer to be sent does not correspond to the current kernel format, or the sending task does not have the privilege to supply the message attributes." },
            { MACH_SEND_INVALID_MEMORY, "The message body specified out-of-line data that was not readable." },
            { MACH_SEND_INVALID_RIGHT, "The message body specified a port right which the caller didn't possess." },
            { MACH_SEND_INVALID_TYPE, "A kernel processed descriptor was invalid." },
            { MACH_SEND_MSG_TOO_SMALL, "The last data item in the message ran over the end of the message." },
            { MACH_SEND_TIMED_OUT, "The timeout interval expired." },
            { MACH_SEND_INTERRUPTED, "A software interrupt occurred." },
            { MACH_SEND_TOO_LARGE, "The message is too large for port." }
            
        };
        
        MPErrorDescription MPApiErrorDescriptions[] =
        {
            { KERN_INVALID_ADDRESS, "Specified address is not currently valid." },
            { KERN_PROTECTION_FAILURE, "Specified memory is valid, but does not permit the required forms of access." },
            { KERN_NO_SPACE, "The address range specified is already in use, or no address range of the size specified could be found." },
            { KERN_INVALID_ARGUMENT, "The function requested was not applicable to this type of argument, or an argument is invalid" },
            { KERN_FAILURE, "The function could not be performed.  A catch-all." },
            { KERN_RESOURCE_SHORTAGE, "A system resource could not be allocated to fulfill this request.  This failure may not be permanent." },
            { KERN_NOT_RECEIVER, "The task in question does not hold receive rights for the port argument." },
            { KERN_NO_ACCESS, "Bogus access restriction." },
            { KERN_MEMORY_FAILURE, "During a page fault, the target address refers to a memory object that has been destroyed. This failure is permanent." },
            { KERN_MEMORY_ERROR, "During a page fault, the memory object indicated that the data could not be returned.  This failure may be temporary; future attempts to access this same data may succeed, as defined by the memory object." },
            { KERN_ALREADY_IN_SET, "The receive right is already a member of the portset." },
            { KERN_NOT_IN_SET, "The receive right is not a member of a port set." },
            { KERN_NAME_EXISTS, "The name already denotes a right in the task." },
            { KERN_ABORTED, "The operation was aborted.  Ipc code will catch this and reflect it as a message error." },
            { KERN_INVALID_NAME, "The name doesn't denote a right in the task." },
            { KERN_INVALID_TASK, "Target task isn't an active task." },
            { KERN_INVALID_RIGHT, "The name denotes a right, but not an appropriate right." },
            { KERN_INVALID_VALUE, "A blatant range error." },
            { KERN_UREFS_OVERFLOW, "Operation would overflow limit on user-references." },
            { KERN_INVALID_CAPABILITY, "The supplied (port) capability is improper." },
            { KERN_RIGHT_EXISTS, "The task already has send or receive rights for the port under another name."},
            { KERN_INVALID_HOST, "Target host isn't actually a host." },
            { KERN_MEMORY_PRESENT, "An attempt was made to supply 'precious' data for memory that is already present in a memory object." },
            { KERN_MEMORY_DATA_MOVED, "A page was requested of a memory manager via memory_object_data_request for an object using a MEMORY_OBJECT_COPY_CALL strategy, with the VM_PROT_WANTS_COPY flag being used to specify that the page desired is for a copy of the"
                " object, and the memory manager has detected the page was pushed into a copy of the object while the kernel was walking the shadow chain from the copy to the object. This error code is delivered via memory_object_data_error"
                " and is handled by the kernel (it forces the kernel to restart the fault). It will not be seen by users." },
            { KERN_MEMORY_RESTART_COPY, "A strategic copy was attempted of an object upon which a quicker copy is now possible. The caller should retry the copy using vm_object_copy_quickly. This error code is seen only by the kernel." },
            { KERN_INVALID_PROCESSOR_SET, "An argument applied to assert processor set privilege was not a processor set control port." },
            { KERN_POLICY_LIMIT, "The specified scheduling attributes exceed the thread's limits." },
            { KERN_INVALID_POLICY, "The specified scheduling policy is not currently enabled for the processor set." },
            { KERN_INVALID_OBJECT, "The external memory manager failed to initialize the memory object." },
            { KERN_ALREADY_WAITING, "A thread is attempting to wait for an event for which there is already a waiting thread." },
            { KERN_DEFAULT_SET, "An attempt was made to destroy the default processor set." },
            { KERN_EXCEPTION_PROTECTED, "An attempt was made to fetch an exception port that is protected, or to abort a thread while processing a protected exception." },
            { KERN_INVALID_LEDGER, "A ledger was required but not supplied." },
            { KERN_INVALID_MEMORY_CONTROL, "The port was not a memory cache control port." },
            { KERN_INVALID_SECURITY, "An argument supplied to assert security privilege was not a host security port." },
            { KERN_NOT_DEPRESSED, "thread_depress_abort was called on a thread which was not currently depressed." },
            { KERN_TERMINATED, "Object has been terminated and is no longer available" },
            { KERN_LOCK_SET_DESTROYED, "Lock set has been destroyed and is no longer available." },
            { KERN_LOCK_UNSTABLE, "The thread holding the lock terminated before releasing the lock" },
            { KERN_LOCK_OWNED, "The lock is already owned by another thread" },
            { KERN_LOCK_OWNED_SELF, "The lock is already owned by the calling thread" },
            { KERN_SEMAPHORE_DESTROYED, "Semaphore has been destroyed and is no longer available." },
            { KERN_RPC_SERVER_TERMINATED, "Return from RPC indicating the target server was terminated before it successfully replied" },
            { KERN_RPC_TERMINATE_ORPHAN, "Terminate an orphaned activation." },
            { KERN_RPC_CONTINUE_ORPHAN, "Allow an orphaned activation to continue executing." },
            { KERN_NOT_SUPPORTED, "Empty thread activation (No thread linked to it)" },
            { KERN_NODE_DOWN, "Remote node down or inaccessible." },
            { KERN_NOT_WAITING, "A signalled thread was not actually waiting." },
            { KERN_OPERATION_TIMED_OUT, "Some thread-oriented operation (semaphore_wait) timed out" },
            { KERN_CODESIGN_ERROR, "During a page fault, indicates that the page was rejected as a result of a signature check." },
            { KERN_RETURN_MAX, "Maximum return value allowable" }
        };
        
        // Helper to log errors and return whether a send/receive command was successful/the connection is broken.
        MachPort::CommResult MPHandleCommResult(mach_msg_return_t result, natural_t timeout)
        {
            if (result == MACH_MSG_SUCCESS)
                return MachPort::CommResult_SUCCEEDED;
            
            if (timeout == 0 && result == MACH_RCV_TIMED_OUT)
                return MachPort::CommResult_FAILED; // No message for that, just polling for data
            
            MachPort::CommResult retVal = MachPort::CommResult_FAILED;
            
            size_t idx = 0;
            size_t count = sizeof(MPCommErrorDescriptions) / sizeof(MPErrorDescription);
            for (; idx < count; ++idx)
            {
                if (MPCommErrorDescriptions[idx].errorID == result)
                {
                    IGO_TRACEF("Mach port snd/rcv error: %s", MPCommErrorDescriptions[idx].description);
                    if (result == MACH_RCV_PORT_DIED || result == MACH_SEND_INVALID_DEST)
                        retVal = MachPort::CommResult_CONNECTION_DEAD;
                    
                    break;
                }
            }
            
            if (idx == count)
            {
                IGO_TRACEF("Mach port snd/rcv error: no description available (%d)", result);
            }
            
            return retVal;
        }
        
        // Helper to log errors from the mach API
        bool MPHandleResult(kern_return_t result)
        {
            if (result == KERN_SUCCESS)
                return true;
            
            size_t idx = 0;
            size_t count = sizeof(MPApiErrorDescriptions) / sizeof(MPErrorDescription);
            for (; idx < count; ++idx)
            {
                if (MPApiErrorDescriptions[idx].errorID == result)
                {
                    IGO_TRACEF("Mach port api error: %s", MPApiErrorDescriptions[idx].description);
                    break;
                }
            }
            
            if (idx == count)
            {
                IGO_TRACEF("Mach port api error: no description available (%d)", result);
            }
            
            return false;
        }
        
        // Helper used to receive a simple message
        MachPort::CommResult MPRecvMsg(mach_port_t port, void* buffer, size_t bufferSize, int timeoutInMS, size_t* dataSize, mach_port_t* sourcePort, mach_port_t *srPort, void** oolData, size_t* oolDataSize)
        {
            // Make sure the buffer is big enough for any of the messages we support
            char msg[sizeof(MPRecvRightsMsg) + sizeof(MPRecvOOLMsg)];
            mach_msg_header_t* header = reinterpret_cast<mach_msg_header_t*>(msg);
            
            header->msgh_size        = sizeof(msg);
            header->msgh_local_port  = port;
            
            natural_t timeout = MACH_MSG_TIMEOUT_NONE;
            mach_msg_option_t options = MACH_RCV_MSG;
            if (timeoutInMS >= 0)
            {
                options |= MACH_RCV_TIMEOUT;
                if (timeoutInMS > 0)
                    timeout = (natural_t)timeoutInMS;
            }
            
            mach_msg_return_t result = mach_msg(header, options, 0, header->msgh_size, port, timeout, MACH_PORT_NULL);
            MachPort::CommResult mpResult = MPHandleCommResult(result, timeout);
            
            if (mpResult == MachPort::CommResult_SUCCEEDED)
            {
                // Which message type did we get?
                switch (header->msgh_id)
                {
                    case MPMessageType_SIMPLE:
                    {
                        struct MPRecvMsg* simpleMsg = reinterpret_cast<struct MPRecvMsg*>(msg);
                        
                        size_t copySize = buffer ? simpleMsg->dataSize : 0;
                        if (copySize > bufferSize)
                        {
                            IGO_TRACEF("Skipping truncated data from send rights message (%ld/%ld)", bufferSize, simpleMsg->dataSize);
                            mpResult = MachPort::CommResult_FAILED;
                        }
                        
                        else
                        {
                            if (copySize > 0)
                                memcpy(buffer, simpleMsg->data, copySize);
                            
                            if (dataSize)
                                *dataSize = copySize;
                            
                            if (sourcePort)
                                *sourcePort = simpleMsg->header.msgh_local_port;
                            
                            if (srPort)
                                *srPort = MACH_PORT_NULL; 
                            
                            if (oolData)
                                *oolData = NULL;
                            
                            if (oolDataSize)
                                *oolDataSize = 0;
                        }
                    }
                        break;
                        
                        //////////////
                        
                    case MPMessageType_WITH_SEND_RIGHTS:
                    {
                        struct MPRecvRightsMsg* rightsMsg = reinterpret_cast<MPRecvRightsMsg*>(msg);
                        if (rightsMsg->body.msgh_descriptor_count == 1 && rightsMsg->portRights.disposition == MACH_MSG_TYPE_PORT_SEND 
                            && rightsMsg->portRights.name != MACH_PORT_NULL && rightsMsg->portRights.name != MACH_PORT_DEAD)
                        {
                            size_t copySize = buffer ? rightsMsg->dataSize : 0;
                            if (copySize > bufferSize)
                            {
                                IGO_TRACEF("Skipping truncated data from send rights message (%ld/%ld)", bufferSize, rightsMsg->dataSize);
                                
                                // Release the port rights - call dummy recv to prevent optimizer from remove code
                                MachPort cleanPort(rightsMsg->portRights.name, true);
                                cleanPort.Recv(NULL, 0, 0, NULL, NULL, NULL, NULL);
                                
                                mpResult = MachPort::CommResult_FAILED;
                            }
                            
                            else
                            {
                                if (copySize > 0)
                                    memcpy(buffer, rightsMsg->data, copySize);
                                
                                if (dataSize)
                                    *dataSize = copySize;
                                
                                if (sourcePort)
                                    *sourcePort = rightsMsg->header.msgh_local_port;
                                
                                if (srPort)
                                    *srPort = rightsMsg->portRights.name; 
                                
                                if (oolData)
                                    *oolData = NULL;
                                
                                if (oolDataSize)
                                    *oolDataSize = 0;
                            }
                        }
                    }
                        break;
                        
                        //////////////
                        
                    case MPMessageType_WITH_OOL:
                    {
                        struct MPRecvOOLMsg* oolMsg = reinterpret_cast<MPRecvOOLMsg*>(msg);
                        if (oolMsg->body.msgh_descriptor_count == 1 && oolMsg->ool.address != NULL
                            && oolMsg->ool.size != 0)
                        {
                            size_t copySize = buffer ? oolMsg->dataSize : 0;
                            if (copySize > bufferSize)
                            {
                                IGO_TRACEF("Skipping truncated data from send rights message (%ld/%ld)", bufferSize, oolMsg->dataSize);                            
                                mpResult = MachPort::CommResult_FAILED;
                            }
                            
                            else
                            {
                                if (copySize > 0)
                                    memcpy(buffer, oolMsg->data, copySize);
                                
                                if (dataSize)
                                    *dataSize = copySize;
                                
                                if (sourcePort)
                                    *sourcePort = oolMsg->header.msgh_local_port;
                                
                                if (srPort)
                                    *srPort = MACH_PORT_NULL; 
                                
                                if (oolData)
                                    *oolData = oolMsg->ool.address;
                                
                                if (oolDataSize)
                                    *oolDataSize = oolMsg->ool.size;
                            }
                        }
                    }
                        break;

                        //////////////

                    case MACH_NOTIFY_DEAD_NAME:
                    {
                        mach_dead_name_notification_t* dnNotify = reinterpret_cast<mach_dead_name_notification_t*>(msg);
                        
                        // Deallocate extra reference from the notification
                        mach_port_deallocate(mach_task_self(), dnNotify->not_port);
                        
                        if (sourcePort)
                            *sourcePort = dnNotify->not_header.msgh_local_port;
                        
                        if (srPort)
                            *srPort = dnNotify->not_port;
                        
                        IGO_TRACEF("Got dead name notification for %d", dnNotify->not_port);
                        
                        mpResult = MachPort::CommResult_NOTIFY;
                    }
                        break;
                        
                        //////////////

                    default:
                        IGO_TRACEF("Receive message with unknown type (%d)", header->msgh_id);
                        mpResult = MachPort::CommResult_FAILED;
                        break;
                };
            }
            
            return mpResult;    
        }
        
        // Create a new r/w mach port
        MachPort::MachPort(int queueSize)
        : _port(MACH_PORT_NULL), _isValid(false), _isNotification(false), _sendRightsOnly(false)
        {
            mach_port_t port = MACH_PORT_NULL;
            if (MPHandleResult(mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port)))
            {
                if (queueSize > 0)
                {
                    mach_port_limits_t portLimits;
                    portLimits.mpl_qlimit = 512;
                    MPHandleResult(mach_port_set_attributes(mach_task_self(), port, MACH_PORT_LIMITS_INFO, reinterpret_cast<mach_port_info_t>(&portLimits), MACH_PORT_LIMITS_INFO_COUNT));
                }
                
                if (MPHandleResult(mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND)))
                {
                    _port = port;
                    _isValid = true;
                }
                
                else
                {
                    IGO_TRACE("Unable to insert send right to a newly allocated mach port");
                    
                    mach_port_deallocate(mach_task_self(), port);
                    mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE, -1);
                }
                
            }
            
            else
            {
                IGO_TRACE("Unable to allocate a mach port");
            }
        }
        
        // Create a wrapper around an existing port
        // sendRightsOnly = false -> don't take over cleanup responsabilities - used for compares
        // sendRightsOnly = true -> create of a write only port -> takes over the cleanup responsabilities.
        MachPort::MachPort(mach_port_t port, bool sendRightsOnly, bool isNotification)
        : _port(port), _isValid(sendRightsOnly), _isNotification(isNotification), _sendRightsOnly(sendRightsOnly)
        {
        }
        
        // Copy a mach port with the auto_ptr semantics (ie reference port is not responsible
        // anymore for cleanup the port)
        MachPort::MachPort(const MachPort& other)
        {
            _port = other._port;
            _isValid = other._isValid;
            _isNotification = other._isNotification;
            _sendRightsOnly = other._sendRightsOnly;
            
            other._isValid = false;
            other._sendRightsOnly = false;
        }
        
        // Look up a mach port service
        MachPort::MachPort(const char* serviceName)
        : _port(MACH_PORT_NULL), _isValid(false), _isNotification(false), _sendRightsOnly(false)
        {
            if (!serviceName || strlen(serviceName) >= sizeof(name_t))
            {
                IGO_TRACE("Invalid mach port service name to lookup...");
                return;
            }
            
            mach_port_t port = MACH_PORT_NULL;
            
            mach_port_t bootstrap;
            if (MPHandleResult(task_get_bootstrap_port(mach_task_self(), &bootstrap)))
            {
                name_t name;
                strncpy(name, serviceName, sizeof(name));
                name[sizeof(name) - 1] = '\0';
                
                kern_return_t result = bootstrap_look_up(bootstrap, name, &port);
                mach_port_deallocate(mach_task_self(), bootstrap);
                
                if (MPHandleResult(result))
                {
                    _port = port;
                    _isValid = true;
                    _sendRightsOnly = true;
                }
                
                else
                {
                    IGO_TRACEF("Unable to lookup a mach port service %s", serviceName);
                }
            }
            
            else
            {
                IGO_TRACE("Failed to get the bootstrap port");
            }   
        }
        
        // Deallocate/clear up a r/w mach port
        MachPort::~MachPort()
        {
            if (IsValid())
            {
#ifdef SHOW_MACHPORT_SETUP
                char portInfo[256];
                this->ToString(portInfo, sizeof(portInfo));
                IGO_TRACEF("About to destroy port: %s", portInfo);
#endif
                
                if (!_sendRightsOnly)
                {
                    // Make sure there is no portset using that guy!
                    if (!MPHandleResult(mach_port_move_member(mach_task_self(), _port, MACH_PORT_NULL)))
                    {
                        IGO_TRACE("Error while restoring the mach port receive right from a port set");
                    }
                    
                    // When removing receive right, we'll lose the send right too and the port becomes dead
                    if (!MPHandleResult(mach_port_mod_refs(mach_task_self(), _port, MACH_PORT_RIGHT_RECEIVE, -1)))
                    {
                        IGO_TRACE("Error while removing a mach port receive right");
                    }                
                }
                
                else
                if (!MPHandleResult(mach_port_mod_refs(mach_task_self(), _port, MACH_PORT_RIGHT_SEND, -1)))
                {
                    IGO_TRACE("Error while removing a mach port send right");
                }
                
                if (!_sendRightsOnly)
                {
                    if (!MPHandleResult(mach_port_deallocate(mach_task_self(), _port)))
                    {
                        IGO_TRACE("Error while deallocating a mach port");
                    }
                }
            }    
        }
        
        // Copy a mach port with the auto_ptr semantics (ie reference port is not responsible
        // anymore for cleanup the port)
        MachPort& MachPort::operator=(const MachPort& other)
        {
            if (this == &other)
                return *this;
            
            _port = other._port;
            _isValid = other._isValid;
            _isNotification = other._isNotification;
            _sendRightsOnly = other._sendRightsOnly;
            
            other._isValid = false;
            other._sendRightsOnly = false;
            
            return *this;
        }
        
        // Return a string to help with debugging
        void MachPort::ToString(char* buffer, size_t bufferSize)
        {
            if (!IsValid())
                snprintf(buffer, bufferSize, "invalid");
            
            else
            {
                mach_port_urefs_t sendRefs;
                mach_port_urefs_t recvRefs;
                mach_port_urefs_t sendOnceRefs;
                mach_port_urefs_t portSetRefs;
                mach_port_urefs_t deadRefs;
                
                MPHandleResult(mach_port_get_refs(mach_task_self(), _port, MACH_PORT_RIGHT_SEND, &sendRefs));
                MPHandleResult(mach_port_get_refs(mach_task_self(), _port, MACH_PORT_RIGHT_RECEIVE, &recvRefs));
                MPHandleResult(mach_port_get_refs(mach_task_self(), _port, MACH_PORT_RIGHT_SEND_ONCE, &sendOnceRefs));
                MPHandleResult(mach_port_get_refs(mach_task_self(), _port, MACH_PORT_RIGHT_PORT_SET, &portSetRefs));
                MPHandleResult(mach_port_get_refs(mach_task_self(), _port, MACH_PORT_RIGHT_DEAD_NAME, &deadRefs));
                
                
                if (_sendRightsOnly)
                    snprintf(buffer, bufferSize, "id=%d, send only - refs send:%d, recv:%d, sendOnce:%d, portset:%d, dead:%d", _port, sendRefs, recvRefs, sendOnceRefs, portSetRefs, deadRefs);
                
                else
                    snprintf(buffer, bufferSize, "id=%d, send/receive - refs send:%d, recv:%d, sendOnce:%d, portset:%d, dead:%d", _port, sendRefs, recvRefs, sendOnceRefs, portSetRefs, deadRefs);
            }
        }
        
        // Register a mach port as a service
        bool MachPort::RegisterService(const char* serviceName)
        {
            if (!IsValid())
                return false;
            
            
            bool retVal = false;
            
            mach_port_t bootstrap;
            if (MPHandleResult(task_get_bootstrap_port(mach_task_self(), &bootstrap)))
            {
                name_t name;
                strncpy(name, serviceName, sizeof(name));
                name[sizeof(name) - 1] = '\0';
                
                kern_return_t result = bootstrap_register(bootstrap, name, _port);
                mach_port_deallocate(mach_task_self(), bootstrap);
                
                if (MPHandleResult(result))
                {
                    retVal = true;
                }
                
                else
                {
                    IGO_TRACEF("Unable to register a mach port service %s", serviceName);
                }
            }
            
            else
            {
                IGO_TRACE("Failed to get the bootstrap port");
            }           
            
            return retVal;
        }
        
        // Setup port for watch for dead name notification on a different port.
        bool MachPort::Watch(const MachPort& port)
        {
            if (!IsValid())
                return false;
            
            mach_port_t prevPort = MACH_PORT_NULL;
            if (MPHandleResult(mach_port_request_notification(mach_task_self(), port._port, MACH_NOTIFY_DEAD_NAME, 0, this->_port, MACH_MSG_TYPE_MAKE_SEND_ONCE, &prevPort)))
            {
                if (prevPort != MACH_PORT_NULL)
                {
                    IGO_TRACEF("Found previous notification port (%d)!!!", prevPort);
                    mach_port_deallocate(mach_task_self(), prevPort);
                }

                return true;
            }
            
            else
            {
                IGO_TRACEF("Unable to watch mach port %d", port._port);
                return false;
            }
        }
        
        // Add send rights to the port -> return separate instance to manage it
        MachPort MachPort::AddSendRights()
        {
            if (!IsValid())
                return MachPort::Null();
            
            if (MPHandleResult(mach_port_insert_right(mach_task_self(), _port, _port, MACH_MSG_TYPE_MAKE_SEND)))
            {
                return MachPort(_port, true);
            }
            
            else
            {
                IGO_TRACE("Unable to add send rights to the port");
                return MachPort::Null();
            }
        }
        
        // Give send rights to another port
        MachPort::CommResult MachPort::ForwardSendRights(const MachPort& other, void* buffer, size_t bufferSize, int timeoutInMS)
        {
            if (!IsValid())
                return CommResult_FAILED;
            
            MPSendRightsMsg msg;
            msg.header.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0) | MACH_MSGH_BITS_COMPLEX;
            msg.header.msgh_size        = sizeof(msg);
            msg.header.msgh_local_port  = MACH_PORT_NULL;
            msg.header.msgh_remote_port = other._port;
            msg.header.msgh_id          = MPMessageType_WITH_SEND_RIGHTS;    
            
            msg.body.msgh_descriptor_count = 1;
            msg.portRights.type         = MACH_MSG_PORT_DESCRIPTOR;
            msg.portRights.disposition  = MACH_MSG_TYPE_COPY_SEND;
            msg.portRights.name         = _port;
            
            natural_t timeout = MACH_MSG_TIMEOUT_NONE;
            mach_msg_option_t options = MACH_SEND_MSG;
            if (timeoutInMS >= 0)
            {
                options |= MACH_SEND_TIMEOUT;
                if (timeoutInMS > 0)
                    timeout = (natural_t)timeoutInMS;
            }
            
            msg.dataSize = buffer ? bufferSize : 0;
            if (msg.dataSize > sizeof(msg.data))
            {
                IGO_TRACEF("Trying to forward truncated send rights message (%ld/%ld)", bufferSize, sizeof(msg.data));
                return MachPort::CommResult_FAILED;
                
            }
            if (msg.dataSize > 0)
                memcpy(msg.data, buffer, msg.dataSize);
            
            mach_msg_return_t result = mach_msg(&msg.header, options, msg.header.msgh_size, 0, MACH_PORT_NULL, timeout, MACH_PORT_NULL);
            return MPHandleCommResult(result, timeout);
        }
        
        // Send a message
        // Timeout values are in milliseconds. Use a negative timeout to block on the call. 
        // Use 0 for immediate polling.
        MachPort::CommResult MachPort::Send(const void* buffer, size_t bufferSize, int timeoutInMS, void* oolData, size_t oolDataSize)
        {
            if (!IsValid())
                return CommResult_FAILED;
            
            MPSendMsg msg;
            MPSendOOLMsg oolMsg;
            
            // Try handling messages with/without out-of-line data with the same function - but careful of the message layout!
            char* msgData = NULL;
            uint32_t* msgDataSize = NULL;
            mach_msg_header_t* header = NULL;
            
            if (oolData)
            {
                header = &oolMsg.header;
                msgData = oolMsg.data;
                msgDataSize = &oolMsg.dataSize;
                
                oolMsg.header.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0) | MACH_MSGH_BITS_COMPLEX;;
                oolMsg.header.msgh_size        = sizeof(oolMsg);
                oolMsg.header.msgh_local_port  = MACH_PORT_NULL;
                oolMsg.header.msgh_remote_port = _port;
                oolMsg.header.msgh_id          = MPMessageType_WITH_OOL;  
                
                oolMsg.body.msgh_descriptor_count = 1;
                oolMsg.ool.address      = oolData;
                oolMsg.ool.size         = oolDataSize;
                oolMsg.ool.deallocate   = FALSE;
                oolMsg.ool.copy         = MACH_MSG_VIRTUAL_COPY;
                oolMsg.ool.type         = MACH_MSG_OOL_DESCRIPTOR;
            }
            
            else
            {
                header = &msg.header;
                msgData = msg.data;
                msgDataSize = &msg.dataSize;
                
                msg.header.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
                msg.header.msgh_size        = sizeof(msg);
                msg.header.msgh_local_port  = MACH_PORT_NULL;
                msg.header.msgh_remote_port = _port;
                msg.header.msgh_id          = MPMessageType_SIMPLE;
            }
            
            natural_t timeout = MACH_MSG_TIMEOUT_NONE;
            mach_msg_option_t options = MACH_SEND_MSG;
            if (timeoutInMS >= 0)
            {
                options |= MACH_SEND_TIMEOUT;
                if (timeoutInMS > 0)
                    timeout = (natural_t)timeoutInMS;
            }
            
            size_t maxDataSize = sizeof(msg.data); // should be the same for all messages
            
            size_t dataSize = buffer ? bufferSize : 0;
            *msgDataSize = dataSize;
            
            if (dataSize > maxDataSize)
            {
                IGO_TRACEF("Forward truncated send rights message (%ld/%ld)", bufferSize, maxDataSize);
                return MachPort::CommResult_FAILED;
            }
            
            if (dataSize > 0)
                memcpy(msgData, buffer, dataSize);
            
            mach_msg_return_t result = mach_msg(header, options, header->msgh_size, 0, MACH_PORT_NULL, timeout, MACH_PORT_NULL);
            return MPHandleCommResult(result, timeout);
        }
        
        // Receive a message
        // Timeout values are in milliseconds. Use a negative timeout to block on the call. 
        // Use 0 for immediate polling.
        MachPort::CommResult MachPort::Recv(void* buffer, size_t bufferSize, int timeoutInMS, size_t* dataSize, MachPort* srPort, void** oolData, size_t* oolDataSize)
        {
            if (!IsValid() || _sendRightsOnly)
                return CommResult_FAILED;
            
            mach_port_t tmpSRPort = MACH_PORT_NULL;
            CommResult result = MPRecvMsg(_port, buffer, bufferSize, timeoutInMS, dataSize, NULL, &tmpSRPort, oolData, oolDataSize);
            if (result == CommResult_SUCCEEDED)
            {
                if (srPort)
                {
                    if (tmpSRPort != MACH_PORT_NULL)
                        *srPort = MachPort::MachPort(tmpSRPort, true);
                    
                    else
                        *srPort = MachPort::Null();
                }
            }
            
            return result;
        }
        
        // Create a new port set
        MachPortSet::MachPortSet()
        : _portset(MACH_PORT_NULL)
        {
            mach_port_t port = MACH_PORT_NULL;
            if (MPHandleResult(mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &port)))
            {
                _portset = port;
            }
            
            else
            {
                IGO_TRACE("Unable to allocate a new mach port set");
            }
        }
        
        // Deallocate a port set
        MachPortSet::~MachPortSet()
        {
            if (_portset != MACH_PORT_NULL)
            {
                if (!MPHandleResult(mach_port_deallocate(mach_task_self(), _portset)))
                {
                    IGO_TRACE("Error while deallocating a mach port set");
                }
            }
        }
        
        // Return a string to help with debugging
        void MachPortSet::ToString(char* buffer, size_t bufferSize)
        {
            if (!IsValid())
                snprintf(buffer, bufferSize, "Invalid portset");
            
            else
                snprintf(buffer, bufferSize, "id=%d", _portset);
        }
        
        // Move a port receive right to the set
        bool MachPortSet::Add(MachPort* port)
        {
            if (!port || !port->IsValid() || !IsValid())
                return false;
            
            if (!MPHandleResult(mach_port_move_member(mach_task_self(), port->_port, _portset)))
            {
                IGO_TRACE("Error while moving a mach port receive right to a port set");
                return false;
            }
            
            return true;
        }
        
        // Restore a port receive right from the set
        bool MachPortSet::Remove(MachPort* port)
        {
            if (!port || !port->IsValid() || !IsValid())
                return false;
            
            if (!MPHandleResult(mach_port_move_member(mach_task_self(), port->_port, MACH_PORT_NULL)))
            {
                IGO_TRACE("Error while restoring the mach port receive right from a port set");
                return false;
            }
                       
            return true;
        }
        
        // Listen for new data on any of the ports that attached themselves to the set (could contain send rights from another port).
        // Timeout values are in milliseconds. Use a negative timeout to block on the call. 
        // Use 0 for immediate polling.
        // Returns a valid MachPort ID if we got some data.
        MachPort MachPortSet::Listen(void* buffer, size_t bufferSize, int timeoutInMS, size_t* dataSize, MachPort* srPort)
        {
            if (!IsValid())
                return MachPort::Null();
            
            mach_port_t tmpSRPort = MACH_PORT_NULL;
            mach_port_t sourcePort = MACH_PORT_NULL;
            MachPort::CommResult result = MPRecvMsg(_portset, buffer, bufferSize, timeoutInMS, dataSize, &sourcePort, &tmpSRPort, NULL, NULL);
            if (result == MachPort::CommResult_SUCCEEDED)
            {
                if (srPort)
                {
                    if (tmpSRPort != MACH_PORT_NULL)
                        *srPort = MachPort::MachPort(tmpSRPort, true);
                    
                    else
                        *srPort = MachPort::Null();
                }
            
                return MachPort::MachPort(sourcePort, false);
            }            
            
            else
            if (result == MachPort::CommResult_NOTIFY)
            {
                return MachPort::MachPort(sourcePort, false, true);
            }
            
            return MachPort::Null();
        }
        
    } // namespace Mac
} // namespace Origin

#endif // ORIGIN_MAC
