#ifndef CRC_MAP_H_INCLUDED
#define CRC_MAP_H_INCLUDED

#include <QMap>
#include <QFile>
#include <QHash>
#include <QString>
#include <QObject>
#include <QMutex>
#include <QThread>

#include "services/downloader/TransferStats.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

struct CRCMapData
{
    CRCMapData() : crc(0), size(0), id(""), jobId("") {}
    quint32 crc;
    qint64 size;
    QString id;
    QString jobId;
};

typedef QMap<QString, CRCMapData>        tCRCMap;

class CRCMap;
class CRCMapFactory;
class CRCMapVerifyProgressProxy;

class ORIGIN_PLUGIN_API CRCWorkerThread : public QThread
{
    friend class CRCMapVerifyProgressProxy;
    friend class CRCMap;

    public:
    
        CRCWorkerThread() : _shouldCancel(false)
        {
            setObjectName("CRCWorkerThread");
        }
    
        void setJobDetails(CRCMap *map, CRCMapVerifyProgressProxy *verifyProxy, bool quickVerify, const QString& rootPath, const QString& id = "");

        virtual void run();

        void VerifyThreadProc(CRCMap *map, CRCMapVerifyProgressProxy *verifyProxy, bool quick, const QString& rootPath, const QString& id);
    private:
        CRCMap *_map;
        CRCMapVerifyProgressProxy *_verifyProxy;
        bool _quickVerify;
        volatile bool _shouldCancel;
        QString _rootPath;
        QString _id;
};

class ORIGIN_PLUGIN_API CRCMapVerifyProgressProxy : public QObject
{
    friend class CRCMap;
    friend class CRCWorkerThread;

    Q_OBJECT

private:
    CRCMapVerifyProgressProxy();
    ~CRCMapVerifyProgressProxy();

    // Will be called by our helper thread
    void verifyThreadUpdateProgress(qint64 bytesProcessed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);
    void verifyThreadComplete();
signals:
    void verifyProgress(qint64 bytesProcessed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining);
    void verifyCompleted();

public:
    void Start();
    void Cancel();
    void Cleanup();
private:
    TransferStats _transferStats;
    qint64 _timeSinceLastProgress;
    CRCWorkerThread _verifyThread;
};

class ORIGIN_PLUGIN_API CRCMap : public QObject
{
    friend class CRCWorkerThread;
    friend class CRCMapFactory;

    Q_OBJECT
private:
    tCRCMap _crcMap;
    QString _crcMapPath;
    bool _isLoaded;
    bool _hasChanges;
    
    QMutex _mapLock;

    // Private do-nothing constructor
    CRCMap();

    static bool SafeWriteString(QFile* file, const QString& string);

    static quint32 _CalculateCRCForFile(CRCMapVerifyProgressProxy *verifyProxy, const QString& file, qint64 maxReadBytes, bool finalize, bool wantProgress, qint64 bytesProcessed, qint64 bytesTotal);

private:
    CRCMap(const QString& crcMapPath);
    ~CRCMap();

public:
    const QString& getCRCMapPath();

    void    AddModify(const QString& file, quint32 crc, qint64 size, const QString& id);
    void    SetContentId(const QString& file, const QString& id);
    bool    SetJobId(const QString& file, const QString& jobId);
    bool    Contains(const QString& file);
    void    Remove(const QString& file);
    CRCMapData GetCRC(const QString& file);

    void    Clear();
    bool    ClearDiskFile();

    // Gets the total data size for all entries of a specific content-id
    qint64 GetSizeForId(const QString& id);

    // Gets all known CRC map keys
    QList<QString> GetAllKeys();

    // Asynchronous methods
    CRCMapVerifyProgressProxy* VerifyBegin(bool quick, const QString& rootPath, const QString& id = "");
    void VerifyEnd(CRCMapVerifyProgressProxy* verifyProxy);

    bool HasChanges() const;
    bool Save();
    bool Load();
    bool IsLoaded();

    static qint64  GetFileLength(const QString& file);
    static quint32 GetCRCInitialValue();
    static quint32 CalculateCRC(quint32 previous, const void* data, unsigned int dataLen);
    static quint32 CalculateFinalCRC(quint32 crc);
    static quint32 CalculateCRCForFile(const QString& file, qint64 maxReadBytes, bool finalize = true);
    static bool LoadLegacy(const QString& file, QHash<QString, quint32>& legacyCrcMap);
    static bool ConvertLegacy(const QString& legacyCRCFilename, const QString& contentInstallPath,
        const QString& id, CRCMap& map);
    static bool GenerateFolderCRCs(const QString& folderPath, const QString& id, CRCMap& map, const QString& baseFolderPath = "");

    static void CalculateCRCPatchBytes(quint32 crcStart, quint32 crcTarget, quint8 *patchBytes);
};

struct CRCMapInstanceData
{
    CRCMapInstanceData() : m_map(NULL), m_refCount(0) { }
    CRCMap* m_map;
    quint32 m_refCount;
};

class ORIGIN_PLUGIN_API CRCMapFactory : public QObject
{
    Q_OBJECT

public:
    static CRCMapFactory* instance();

    CRCMapFactory();
    ~CRCMapFactory();

    CRCMap* getCRCMap(const QString& crcMapPath);
    void releaseCRCMap(CRCMap* crcMap);

private:
    QMutex m_mapLock;
    QMap<QString, CRCMapInstanceData> m_crcMaps;
};

} // namespace Downloader
} // namespace Origin

#endif

