#pragma once

#include <QVector>
#include <QMap>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <eastl/string.h>

#define FB_GLOBAL_ARENA "unused"
#define FB_INFO(fmt) fbprintf(0, fmt)
#define FB_INFO_FORMAT(fmt, ...) fbprintf(0, fmt, __VA_ARGS__)
#define FB_WARNING(fmt) fbprintf(1, fmt)
#define FB_WARNING_FORMAT(fmt, ...) fbprintf(1, fmt, __VA_ARGS__)
#define FB_ERROR(fmt) fbprintf(2, fmt)
#define FB_ERROR_FORMAT(fmt, ...) fbprintf(2, fmt, __VA_ARGS__)

class OriginSDKMain;

// Necessary because all the FB new/delete calls use an arena
void* operator new (size_t size, const char * arena);
void operator delete(void *p, const char * arena);

namespace fb
{

typedef unsigned int uint;

void fbprintf(int type, char* fmt, ...);

template <class T>
class ArrayEditor
{
public:
    ArrayEditor(QVector<T> &arr) : _arr(arr) { }
    unsigned int size() const { return _arr.size(); }
    void push_back(T val) { _arr.push_back(val); }

private:
    QVector<T> &_arr;
};

template <class T>
class ArrayReader
{
public:
    ArrayReader(QVector<T> arr) : _arr(arr) { }
    unsigned int size() const { return _arr.size(); }
    T operator[](unsigned int index) const { return _arr[index]; }

private:
    QVector<T> _arr;
};

template <class T>
T* allocArray(const char* arena, unsigned int size) 
{ 
    Q_UNUSED(arena); 
    return new T[size]; 
}

template <class T>
void freeArray(const char* arena, T* arr, unsigned int size) 
{ 
    Q_UNUSED(arena); 
    Q_UNUSED(size); 
    delete [] arr; 
}

enum MessageCategory
{
    MessageCategory_Unknown = 0,
    MessageCategory_OriginSDKRequest,
    MessageCategory_OriginStreamInstall,
    MessageCategory_StreamInstall
};

enum MessageType
{
    MessageType_Unknown = 0,
    MessageType_OriginSDKRequestIsProgressiveInstallAvailableRequest,
    MessageType_OriginSDKRequestAreChunksInstalledRequest,
    MessageType_OriginSDKRequestQueryChunkStatusRequest,
    MessageType_OriginStreamInstallOriginAvailable,
    MessageType_OriginStreamInstallOriginUnavailable,
    MessageType_OriginStreamInstallOriginSDKError,
    MessageType_OriginStreamInstallIsProgressiveInstallAvailableResponse,
    MessageType_OriginStreamInstallAreChunksInstalledResponse,
    MessageType_OriginStreamInstallQueryChunkStatusResponse,
    MessageType_OriginStreamInstallChunkUpdateEvent,
    MessageType_StreamInstallGameInstalled,
    MessageType_StreamInstallInstalling,
    MessageType_StreamInstallChunkInstalled,
    MessageType_StreamInstallInstallDone,
    MessageType_StreamInstallInstallProgress,
    MessageType_StreamInstallInstallFailed
};

class Message
{
public:
    Message();
    virtual ~Message();

    template <class T> T& as() const
    {
        return *((T*)this);
    }

    virtual QString Serialize() const = 0;
    virtual void Deserialize(QString data) = 0;

    MessageCategory category;
    MessageType type;
};

class MessageManager;

class MessageListener
{
    friend class MessageManager;
public:
    virtual ~MessageListener();

protected:
    MessageListener();
    virtual void onMessage(const Message& message);
};

class MessageManager
{
    friend class OriginSDKMain;
public:
    MessageManager();
    ~MessageManager();

    void registerMessageListener(MessageCategory cat, MessageListener* listener);
    void unregisterMessageListener(MessageCategory cat, MessageListener* listener);

    void queueMessage(Message* msg);
    void executeMessage(const Message& msg);

private:
    void onQueuedMessage(int category, int type, QString msg);

private:
    QMap<MessageCategory, QList<MessageListener*> > _listeners;
};

// Origin-specific Messages
class OriginSDKRequestIsProgressiveInstallAvailableRequestMessage : public Message
{
public:
    OriginSDKRequestIsProgressiveInstallAvailableRequestMessage() 
    {
        category = MessageCategory_OriginSDKRequest;
        type = MessageType_OriginSDKRequestIsProgressiveInstallAvailableRequest;
    }

    virtual QString Serialize() const { return QString("OriginSDKRequestIsProgressiveInstallAvailableRequestMessage"); }
    virtual void Deserialize(QString data) { }
};

class OriginSDKRequestAreChunksInstalledRequestMessage : public Message
{
public:
    OriginSDKRequestAreChunksInstalledRequestMessage() 
    {
        category = MessageCategory_OriginSDKRequest;
        type = MessageType_OriginSDKRequestAreChunksInstalledRequest;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        QJsonArray chunks;
        for (QVector<uint>::const_iterator iter = _chunkIds.cbegin(); iter != _chunkIds.cend(); ++iter)
        {
            chunks.append(QJsonValue((int)(*iter)));
        }
        thisObj["chunkIds"] = chunks;
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        _chunkIds.clear();
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        QJsonArray chunks = doc.object()["chunkIds"].toArray();
        for (int i = 0; i < chunks.size(); ++i)
        {
            _chunkIds.append(chunks.at(i).toInt());
        }
    }

    ArrayEditor<uint> editChunkIds() { return ArrayEditor<uint>(_chunkIds); }
    ArrayReader<uint> readChunkIds() const { return ArrayReader<uint>(_chunkIds); }
private:
    QVector<uint> _chunkIds;
};

class OriginSDKRequestQueryChunkStatusRequestMessage : public Message
{
public:
    OriginSDKRequestQueryChunkStatusRequestMessage() 
    {
        category = MessageCategory_OriginSDKRequest;
        type = MessageType_OriginSDKRequestQueryChunkStatusRequest;
    }

    virtual QString Serialize() const { return QString("OriginSDKRequestQueryChunkStatusRequestMessage"); }
    virtual void Deserialize(QString data) { }
};

class OriginStreamInstallOriginSDKErrorMessage : public Message
{
public:
    OriginStreamInstallOriginSDKErrorMessage() : _errCode(0)
    {
        category = MessageCategory_OriginStreamInstall;
        type = MessageType_OriginStreamInstallOriginSDKError;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        thisObj["errCode"] = QJsonValue((int)_errCode);
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _errCode = (uint)doc.object()["errCode"].toInt();
    }

    void setError(uint errCode) { _errCode = errCode; }
    uint getError() const { return _errCode; }

private:
    uint _errCode;
};

class OriginStreamInstallOriginAvailableMessage : public Message
{
public:
    OriginStreamInstallOriginAvailableMessage()
    {
        category = MessageCategory_OriginStreamInstall;
        type = MessageType_OriginStreamInstallOriginAvailable;
    }

    virtual QString Serialize() const { return QString("OriginStreamInstallOriginAvailableMessage"); }
    virtual void Deserialize(QString data) { }
};

class OriginStreamInstallOriginUnavailableMessage : public Message
{
public:
    OriginStreamInstallOriginUnavailableMessage()
    {
        category = MessageCategory_OriginStreamInstall;
        type = MessageType_OriginStreamInstallOriginUnavailable;
    }

    virtual QString Serialize() const { return QString("OriginStreamInstallOriginUnavailableMessage"); }
    virtual void Deserialize(QString data) { }
};

class OriginStreamInstallIsProgressiveInstallAvailableResponseMessage : public Message
{
public:
    OriginStreamInstallIsProgressiveInstallAvailableResponseMessage() : _available(false)
    {
        category = MessageCategory_OriginStreamInstall;
        type = MessageType_OriginStreamInstallIsProgressiveInstallAvailableResponse;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        thisObj["available"] = QJsonValue(_available);
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _available = doc.object()["available"].toBool();
    }

    void setavailable(bool val) { _available = val; }
    bool getavailable() const { return _available; }

private:
    bool _available;
};

class OriginStreamInstallAreChunksInstalledResponseMessage : public Message
{
public:
    OriginStreamInstallAreChunksInstalledResponseMessage() : _installed(false) 
    {
        category = MessageCategory_OriginStreamInstall;
        type = MessageType_OriginStreamInstallAreChunksInstalledResponse;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        thisObj["installed"] = QJsonValue(_installed);
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _installed = doc.object()["installed"].toBool();
    }

    void setinstalled(bool val) { _installed = val; }
    bool getinstalled() const { return _installed; }
private:
    bool _installed;
};

class OriginStreamInstallQueryChunkStatusResponseMessage : public Message
{
public:
    OriginStreamInstallQueryChunkStatusResponseMessage()
    {
        category = MessageCategory_OriginStreamInstall;
        type = MessageType_OriginStreamInstallQueryChunkStatusResponse;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        QJsonArray chunks;
        for (QVector<uint>::const_iterator iter = _chunkIds.cbegin(); iter != _chunkIds.cend(); ++iter)
        {
            chunks.append(QJsonValue((int)(*iter)));
        }
        thisObj["chunkIds"] = chunks;
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        _chunkIds.clear();
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        QJsonArray chunks = doc.object()["chunkIds"].toArray();
        for (int i = 0; i < chunks.size(); ++i)
        {
            _chunkIds.append(chunks.at(i).toInt());
        }
    }

    ArrayEditor<uint> editChunkIds() { return ArrayEditor<uint>(_chunkIds); }
    ArrayReader<uint> readChunkIds() const { return ArrayReader<uint>(_chunkIds); }

private:
    QVector<uint> _chunkIds;
};

class OriginStreamInstallChunkUpdateEventMessage : public Message
{
public:
    OriginStreamInstallChunkUpdateEventMessage() : _chunkId(0), _chunkState(0), _chunkSize(0), _progress(0) 
    {
        category = MessageCategory_OriginStreamInstall;
        type = MessageType_OriginStreamInstallChunkUpdateEvent;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        thisObj["chunkId"] = QJsonValue((int)_chunkId);
        thisObj["chunkState"] = QJsonValue((int)_chunkState);
        thisObj["chunkSize"] = QJsonValue(QString("%1").arg(_chunkSize));
        thisObj["progress"] = QJsonValue(_progress);
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _chunkId = (uint)doc.object()["chunkId"].toInt();
        _chunkState = (uint)doc.object()["chunkState"].toInt();
        _chunkSize = (uint)doc.object()["chunkSize"].toString().toLongLong();
        _progress = (float)doc.object()["progress"].toDouble();
    }

    uint getChunkId() const { return _chunkId; }
    uint getChunkState() const { return _chunkState; }
    unsigned long long int getChunkSize() const { return _chunkSize; }
    float getProgress() const { return _progress; }

    void setChunkId(uint chunkId) { _chunkId = chunkId; }
    void setChunkState(uint chunkState) { _chunkState = chunkState; }
    void setChunkSize(unsigned long long int chunkSize) { _chunkSize = chunkSize; }
    void setProgress(float progress) { _progress = progress; }

private:
    uint _chunkId;
    uint _chunkState;
    unsigned long long int _chunkSize;
    float _progress;
};

class StreamInstallGameInstalledMessage : public Message
{
public:
    StreamInstallGameInstalledMessage()
    {
        category = MessageCategory_StreamInstall;
        type = MessageType_StreamInstallGameInstalled;
    }

    virtual QString Serialize() const { return QString("StreamInstallGameInstalledMessage"); }
    virtual void Deserialize(QString data) { }
};

class StreamInstallInstallDoneMessage : public Message
{
public:
    StreamInstallInstallDoneMessage()
    {
        category = MessageCategory_StreamInstall;
        type = MessageType_StreamInstallInstallDone;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        thisObj["group"] = QJsonValue(QString(_group.c_str()));
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _group = doc.object()["group"].toString().toLocal8Bit().constData();
    }

    eastl::string getInstallGroup() const { return _group; }

    void setInstallGroup(eastl::string group) { _group = group; }
private:
    eastl::string _group;  
};

class StreamInstallInstallingMessage : public Message
{
public:
    StreamInstallInstallingMessage()
    {
        category = MessageCategory_StreamInstall;
        type = MessageType_StreamInstallInstalling;
    }

    virtual QString Serialize() const { return QString("StreamInstallInstallingMessage"); }
    virtual void Deserialize(QString data) { }
};

class StreamInstallChunkInstalledMessage : public Message
{
public:
    StreamInstallChunkInstalledMessage() : _chunkId(0)
    {
        category = MessageCategory_StreamInstall;
        type = MessageType_StreamInstallChunkInstalled;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        thisObj["chunkId"] = QJsonValue((int)_chunkId);
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _chunkId = (uint)doc.object()["chunkId"].toInt();
    }

    uint getChunkId() const { return _chunkId; }

    void setChunkId(uint chunkId) { _chunkId = chunkId; }

private:
    uint _chunkId;
};

class StreamInstallInstallProgressMessage : public Message
{
public:
    StreamInstallInstallProgressMessage() : _progress(0)
    {
        category = MessageCategory_StreamInstall;
        type = MessageType_StreamInstallInstallProgress;
    }

    virtual QString Serialize() const
    { 
        QJsonObject thisObj;
        thisObj["group"] = QJsonValue(QString(_group.c_str()));
        thisObj["progress"] = QJsonValue(_progress);
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _group = doc.object()["group"].toString().toLocal8Bit().constData();
        _progress = (float)doc.object()["progress"].toDouble();
    }

    float getProgress() const { return _progress; }
    eastl::string getInstallGroup() const { return _group; }

    void setProgress(float progress) { _progress = progress; }
    void setInstallGroup(eastl::string group) { _group = group; }
private:
    float _progress;
    eastl::string _group;  
};

class StreamInstallInstallFailedMessage : public Message
{
public:
    StreamInstallInstallFailedMessage() : _errCode(0)
    {
        category = MessageCategory_StreamInstall;
        type = MessageType_StreamInstallInstallFailed;
    }

    virtual QString Serialize() const 
    { 
        QJsonObject thisObj;
        thisObj["errCode"] = QJsonValue((int)_errCode);
        QJsonDocument doc(thisObj);
        return QString(doc.toJson(QJsonDocument::Indented));
    }
    virtual void Deserialize(QString data) 
    {
        QJsonDocument doc = QJsonDocument::fromJson(data.toLocal8Bit());
        _errCode = (uint)doc.object()["errCode"].toInt();
    }

    uint getErrCode() const { return _errCode; }

    void setErrCode(uint errCode) { _errCode = errCode; }
private:
    uint _errCode;
};

extern MessageManager* g_coreMessageManager;

}// fb