#include "CoreMessages.h"
#include <OriginSDKMain.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

extern OriginSDKMain* g_sdkController;

void* operator new (size_t size, const char * arena)
{
    Q_UNUSED(arena);

    void *p = malloc(size);
    return p;
}

void operator delete(void *p, const char * arena)
{
    Q_UNUSED(arena);

    free(p);
}

// Crap we need for EASTL
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return malloc(size);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    return malloc(size);
}

namespace fb
{

// Globals
MessageManager* g_coreMessageManager = new MessageManager();

void fbprintf(int type, char* fmt, ...)
{
    char logbuffer[1024];
    memset(logbuffer, 0, sizeof(logbuffer));

    va_list args;
    va_start(args,fmt);
    vsprintf_s<sizeof(logbuffer)>(logbuffer, fmt, args);
    va_end(args);

    QString msgCat;
    QString formatStr = "FB %1: %2";
    switch (type)
    {
    case 0:
        msgCat = "INFO";
        break;
    case 1:
        msgCat = "WARNING";
        formatStr = QString("<p style=\"color:#FF6600\"><b>%1</b></p>").arg(formatStr);
        break;
    case 2:
        msgCat = "ERROR";
        formatStr = QString("<p style=\"color:#CC0000\"><b>%1</b></p>").arg(formatStr);
        break;
    default:
        msgCat = "UNKNOWN";
        break;
    }

    QString logMessage(QString(formatStr).arg(msgCat).arg(logbuffer));
    if (g_sdkController)
    {
        g_sdkController->Log(logMessage);
    }
}

Message::Message() :
    category(MessageCategory_Unknown),
    type(MessageType_Unknown)
{

}

Message::~Message()
{

}

MessageListener::MessageListener()
{
}

MessageListener::~MessageListener()
{

}
    
void MessageListener::onMessage(const Message& message)
{
    // Nothing to do here
}

MessageManager::MessageManager()
{
}

MessageManager::~MessageManager()
{
}

void MessageManager::registerMessageListener(MessageCategory cat, MessageListener* listener)
{
    _listeners[cat].push_back(listener);
}
void MessageManager::unregisterMessageListener(MessageCategory cat, MessageListener* listener)
{
    _listeners[cat].removeAll(listener);
}

void MessageManager::queueMessage(Message* msg)
{
    if (g_sdkController)
    {
        g_sdkController->QueueMessage(msg->category, msg->type, msg->Serialize());
    }
}

void MessageManager::executeMessage(const Message& msg)
{
    if (g_sdkController)
    {
        g_sdkController->Log(QString("Exec message cat: %1 type: %2 msg: <div style=\"color:#009900\">%3</div>").arg((int)msg.category).arg((int)msg.type).arg(msg.Serialize()));
    }

    if (_listeners.contains(msg.category))
    {
        for (QList<MessageListener*>::iterator iter = _listeners[msg.category].begin(); iter != _listeners[msg.category].end(); ++iter)
        {
            (*iter)->onMessage(msg);
        }
    }
}

void MessageManager::onQueuedMessage(int category, int type, QString msg)
{
    if (g_sdkController)
    {
        g_sdkController->Log(QString("Received cat: %1 type: %2 msg: <div style=\"color:#009900\">%3</div>").arg(category).arg(type).arg(msg));
    }

    MessageCategory mCat = static_cast<MessageCategory>(category);
    MessageType mType = static_cast<MessageType>(type);

    Message *msgObj = NULL;

    switch (mCat)
    {
    case MessageCategory_OriginSDKRequest:
        {
            switch (mType)
            {
            case MessageType_OriginSDKRequestIsProgressiveInstallAvailableRequest:
                {
                    msgObj = new OriginSDKRequestIsProgressiveInstallAvailableRequestMessage();
                }
                break;
            case MessageType_OriginSDKRequestAreChunksInstalledRequest:
                {
                    msgObj = new OriginSDKRequestAreChunksInstalledRequestMessage();
                }
                break;
            case MessageType_OriginSDKRequestQueryChunkStatusRequest:
                {
                    msgObj = new OriginSDKRequestQueryChunkStatusRequestMessage();
                }
                break;
            }
        }
        break;
    case MessageCategory_OriginStreamInstall:
        {
            switch (mType)
            {
            case MessageType_OriginStreamInstallOriginAvailable:
                {
                    msgObj = new OriginStreamInstallOriginAvailableMessage();
                }
                break;
            case MessageType_OriginStreamInstallOriginUnavailable:
                {
                    msgObj = new OriginStreamInstallOriginUnavailableMessage();
                }
                break;
            case MessageType_OriginStreamInstallOriginSDKError:
                {
                    msgObj = new OriginStreamInstallOriginSDKErrorMessage();
                }
                break;
            case MessageType_OriginStreamInstallIsProgressiveInstallAvailableResponse:
                {
                    msgObj = new OriginStreamInstallIsProgressiveInstallAvailableResponseMessage();
                }
                break;
            case MessageType_OriginStreamInstallAreChunksInstalledResponse:
                {
                    msgObj = new OriginStreamInstallAreChunksInstalledResponseMessage();
                }
                break;
            case MessageType_OriginStreamInstallQueryChunkStatusResponse:
                {
                    msgObj = new OriginStreamInstallQueryChunkStatusResponseMessage();
                }
                break;
            case MessageType_OriginStreamInstallChunkUpdateEvent:
                {
                    msgObj = new OriginStreamInstallChunkUpdateEventMessage();
                }
                break;
            }
        }
        break;
    case MessageCategory_StreamInstall:
        {
            switch (mType)
            {
            case MessageType_StreamInstallGameInstalled:
                {
                    msgObj = new StreamInstallGameInstalledMessage();
                }
                break;
            case MessageType_StreamInstallInstalling:
                {
                    msgObj = new StreamInstallInstallingMessage();
                }
                break;
            case MessageType_StreamInstallChunkInstalled:
                {
                    msgObj = new StreamInstallChunkInstalledMessage();
                }
                break;
            case MessageType_StreamInstallInstallDone:
                {
                    msgObj = new StreamInstallInstallDoneMessage();
                }
                break;
            case MessageType_StreamInstallInstallProgress:
                {
                    msgObj = new StreamInstallInstallProgressMessage();
                }
                break;
            case MessageType_StreamInstallInstallFailed:
                {
                    msgObj = new StreamInstallInstallFailedMessage();
                }
                break;
            }
        }
        break;
    };
    
    if (!msgObj)
        return;

    msgObj->Deserialize(msg);

    if (_listeners.contains(mCat))
    {
        for (QList<MessageListener*>::iterator iter = _listeners[mCat].begin(); iter != _listeners[mCat].end(); ++iter)
        {
            (*iter)->onMessage(*msgObj);
        }
    }
}

}// fb