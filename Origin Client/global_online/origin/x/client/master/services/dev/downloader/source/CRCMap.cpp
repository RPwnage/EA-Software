#include "services/downloader/Common.h"
#include "services/downloader/CRCMap.h"
#include "services/platform/EnvUtils.h"
#include "services/downloader/FilePath.h"
#include "services/downloader/Parameterizer.h"
#include "services/downloader/StringHelpers.h"
#include "services/downloader/Timer.h"
#include "services/log/LogService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef ORIGIN_MAC
#include <unistd.h>
#endif

namespace Origin
{
namespace Downloader
{
    /// In milliseconds how often to send a CRC progress signal out to the world.
    /// First and Last signals are always sent.
    qint64 gs_crcProgressIntervalMS = 1000;

    /// Factory object which manages CRC maps
    CRCMapFactory* g_crcMapFactory = NULL;

quint32 gs_crc32Table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

quint32 gs_invcrc32Table[256] =
{
    0x00000000, 0xDB710641, 0x6D930AC3, 0xB6E20C82, 0xDB261586, 0x005713C7, 0xB6B51F45, 0x6DC41904,
    0x6D3D2D4D, 0xB64C2B0C, 0x00AE278E, 0xDBDF21CF, 0xB61B38CB, 0x6D6A3E8A, 0xDB883208, 0x00F93449,
    0xDA7A5A9A, 0x010B5CDB, 0xB7E95059, 0x6C985618, 0x015C4F1C, 0xDA2D495D, 0x6CCF45DF, 0xB7BE439E,
    0xB74777D7, 0x6C367196, 0xDAD47D14, 0x01A57B55, 0x6C616251, 0xB7106410, 0x01F26892, 0xDA836ED3,
    0x6F85B375, 0xB4F4B534, 0x0216B9B6, 0xD967BFF7, 0xB4A3A6F3, 0x6FD2A0B2, 0xD930AC30, 0x0241AA71,
    0x02B89E38, 0xD9C99879, 0x6F2B94FB, 0xB45A92BA, 0xD99E8BBE, 0x02EF8DFF, 0xB40D817D, 0x6F7C873C,
    0xB5FFE9EF, 0x6E8EEFAE, 0xD86CE32C, 0x031DE56D, 0x6ED9FC69, 0xB5A8FA28, 0x034AF6AA, 0xD83BF0EB,
    0xD8C2C4A2, 0x03B3C2E3, 0xB551CE61, 0x6E20C820, 0x03E4D124, 0xD895D765, 0x6E77DBE7, 0xB506DDA6,
    0xDF0B66EA, 0x047A60AB, 0xB2986C29, 0x69E96A68, 0x042D736C, 0xDF5C752D, 0x69BE79AF, 0xB2CF7FEE,
    0xB2364BA7, 0x69474DE6, 0xDFA54164, 0x04D44725, 0x69105E21, 0xB2615860, 0x048354E2, 0xDFF252A3,
    0x05713C70, 0xDE003A31, 0x68E236B3, 0xB39330F2, 0xDE5729F6, 0x05262FB7, 0xB3C42335, 0x68B52574,
    0x684C113D, 0xB33D177C, 0x05DF1BFE, 0xDEAE1DBF, 0xB36A04BB, 0x681B02FA, 0xDEF90E78, 0x05880839,
    0xB08ED59F, 0x6BFFD3DE, 0xDD1DDF5C, 0x066CD91D, 0x6BA8C019, 0xB0D9C658, 0x063BCADA, 0xDD4ACC9B,
    0xDDB3F8D2, 0x06C2FE93, 0xB020F211, 0x6B51F450, 0x0695ED54, 0xDDE4EB15, 0x6B06E797, 0xB077E1D6,
    0x6AF48F05, 0xB1858944, 0x076785C6, 0xDC168387, 0xB1D29A83, 0x6AA39CC2, 0xDC419040, 0x07309601,
    0x07C9A248, 0xDCB8A409, 0x6A5AA88B, 0xB12BAECA, 0xDCEFB7CE, 0x079EB18F, 0xB17CBD0D, 0x6A0DBB4C,
    0x6567CB95, 0xBE16CDD4, 0x08F4C156, 0xD385C717, 0xBE41DE13, 0x6530D852, 0xD3D2D4D0, 0x08A3D291,
    0x085AE6D8, 0xD32BE099, 0x65C9EC1B, 0xBEB8EA5A, 0xD37CF35E, 0x080DF51F, 0xBEEFF99D, 0x659EFFDC,
    0xBF1D910F, 0x646C974E, 0xD28E9BCC, 0x09FF9D8D, 0x643B8489, 0xBF4A82C8, 0x09A88E4A, 0xD2D9880B,
    0xD220BC42, 0x0951BA03, 0xBFB3B681, 0x64C2B0C0, 0x0906A9C4, 0xD277AF85, 0x6495A307, 0xBFE4A546,
    0x0AE278E0, 0xD1937EA1, 0x67717223, 0xBC007462, 0xD1C46D66, 0x0AB56B27, 0xBC5767A5, 0x672661E4,
    0x67DF55AD, 0xBCAE53EC, 0x0A4C5F6E, 0xD13D592F, 0xBCF9402B, 0x6788466A, 0xD16A4AE8, 0x0A1B4CA9,
    0xD098227A, 0x0BE9243B, 0xBD0B28B9, 0x667A2EF8, 0x0BBE37FC, 0xD0CF31BD, 0x662D3D3F, 0xBD5C3B7E,
    0xBDA50F37, 0x66D40976, 0xD03605F4, 0x0B4703B5, 0x66831AB1, 0xBDF21CF0, 0x0B101072, 0xD0611633,
    0xBA6CAD7F, 0x611DAB3E, 0xD7FFA7BC, 0x0C8EA1FD, 0x614AB8F9, 0xBA3BBEB8, 0x0CD9B23A, 0xD7A8B47B,
    0xD7518032, 0x0C208673, 0xBAC28AF1, 0x61B38CB0, 0x0C7795B4, 0xD70693F5, 0x61E49F77, 0xBA959936,
    0x6016F7E5, 0xBB67F1A4, 0x0D85FD26, 0xD6F4FB67, 0xBB30E263, 0x6041E422, 0xD6A3E8A0, 0x0DD2EEE1,
    0x0D2BDAA8, 0xD65ADCE9, 0x60B8D06B, 0xBBC9D62A, 0xD60DCF2E, 0x0D7CC96F, 0xBB9EC5ED, 0x60EFC3AC,
    0xD5E91E0A, 0x0E98184B, 0xB87A14C9, 0x630B1288, 0x0ECF0B8C, 0xD5BE0DCD, 0x635C014F, 0xB82D070E,
    0xB8D43347, 0x63A53506, 0xD5473984, 0x0E363FC5, 0x63F226C1, 0xB8832080, 0x0E612C02, 0xD5102A43,
    0x0F934490, 0xD4E242D1, 0x62004E53, 0xB9714812, 0xD4B55116, 0x0FC45757, 0xB9265BD5, 0x62575D94,
    0x62AE69DD, 0xB9DF6F9C, 0x0F3D631E, 0xD44C655F, 0xB9887C5B, 0x62F97A1A, 0xD41B7698, 0x0F6A70D9
};

void CRCWorkerThread::setJobDetails(CRCMap *map, CRCMapVerifyProgressProxy *verifyProxy, bool quickVerify, const QString& rootPath, const QString& id /*= ""*/)
{
	_map = map;
    _verifyProxy = verifyProxy;
	_quickVerify = quickVerify;
    _rootPath = rootPath;
    _id = id;
}

void CRCWorkerThread::run()
{
	VerifyThreadProc(_map, _verifyProxy, _quickVerify, _rootPath, _id);
}

void CRCWorkerThread::VerifyThreadProc(CRCMap *map, CRCMapVerifyProgressProxy *verifyProxy, bool quick, const QString& rootPath, const QString& id)
{
    if (_shouldCancel)
        return;

    qint64 bytesProcessed = 0;

    // Get some values we need from the map
    qint64 totalBytes = _map->GetSizeForId(id);
    QList<QString> crcMapKeys = _map->GetAllKeys();

    verifyProxy->_transferStats.setLogId(id);
    verifyProxy->_transferStats.onStart(totalBytes, 0);

	// Walk all the entires in the map and verify them
	for (QList<QString>::Iterator it = crcMapKeys.begin(); it != crcMapKeys.end(); it++)
	{
        QString mapKey = *it;

        // The map may have changed since we got the keys list
        if (!_map->Contains(mapKey))
            continue;

        const CRCMapData crcData = _map->GetCRC(mapKey);

        if (crcData.id.isEmpty() || crcData.id.compare(id, Qt::CaseInsensitive) == 0)
        {
		    QString filePath = rootPath + mapKey;
            QFileInfo finfo (filePath);
            
		    if (_shouldCancel)
			    break;

		    // If it doesn't exist, it isn't valid
		    if (!EnvUtils::GetFileExistsNative(filePath))
		    {
                // Move the progress bar
			    bytesProcessed += crcData.size;

                verifyProxy->_transferStats.onDataReceived((quint32)crcData.size);
			    verifyProxy->verifyThreadUpdateProgress(bytesProcessed, totalBytes, (qint64)verifyProxy->_transferStats.bps(), verifyProxy->_transferStats.estimatedTimeToCompletion(true));

#ifdef DOWNLOADER_CRC_DEBUG
                ORIGIN_LOG_EVENT << "CRC Map: File did not exist - " << filePath; 
#endif

                // Remove the file from the map since it doesn't exist
                _map->Remove(mapKey);
			    continue;
		    }

		    if (_shouldCancel)
			    break;

		    qint64 fileSize = CRCMap::GetFileLength(filePath);

		    // Bail out here for quick validation.  Quick validation implies that the file exists, and that the size is correct.
		    if (quick && !finfo.isSymLink())
		    {
			    // Move the progress bar
			    bytesProcessed += crcData.size;

			    verifyProxy->_transferStats.onDataReceived((quint32)crcData.size);
			    verifyProxy->verifyThreadUpdateProgress(bytesProcessed, totalBytes, (qint64)verifyProxy->_transferStats.bps(), verifyProxy->_transferStats.estimatedTimeToCompletion(true));

                // Quick (non-repair) validation assumes that if the file was present, the computed CRC matches the stored CRC, so just bail here
			    continue;
		    }

		    // Compute the CRC (this method reports progress)
		    quint32 calculatedCRC = CRCMap::_CalculateCRCForFile(verifyProxy, filePath, -1, true, true, bytesProcessed, totalBytes);

#ifdef DOWNLOADER_CRC_DEBUG
            ORIGIN_LOG_EVENT << "CRC Map: File - " << filePath << " Size - " << fileSize << " CRC - " << calculatedCRC; 
#endif

            // Update the progress counter
            bytesProcessed += fileSize;

            // Update the map with the calculated CRC and observed file size (and overwrite the previous stored values, because we've actually computed them here)
            _map->AddModify(mapKey, calculatedCRC, fileSize, crcData.id);

		    if (_shouldCancel)
			    break;
        }
	}
    
	if (!_shouldCancel)
	{
		// Give the protocol one final progress update so that the GUI will show 100%.
		verifyProxy->verifyThreadUpdateProgress(bytesProcessed, totalBytes, (qint64)verifyProxy->_transferStats.bps(), verifyProxy->_transferStats.estimatedTimeToCompletion(true));
	}
    
    // Clear the transfer stats.
    verifyProxy->_transferStats.onFinished();

    // If this wasn't a quick verify, we've actually gone and computed a bunch of CRCs... so we should save that work here
    if (!quick)
    {
        _map->Save();
    }

	// Inform our caller that we've completed
	verifyProxy->verifyThreadComplete();
}

CRCMapVerifyProgressProxy::CRCMapVerifyProgressProxy() : 
    QObject(NULL),
	_transferStats("CRC Map", "not set"),
    _timeSinceLastProgress(0)
{
}

CRCMapVerifyProgressProxy::~CRCMapVerifyProgressProxy()
{
    // Mark ourselves canceled, just in case
	_verifyThread._shouldCancel = true;
}

void CRCMapVerifyProgressProxy::verifyThreadUpdateProgress(qint64 bytesProcessed, qint64 totalBytes, qint64 bytesPerSecond, qint64 estimatedSecondsRemaining)
{
    // Throttle the emission of progress signals here rather than clog up the system with them
    if(bytesProcessed == totalBytes || (GetTick() - _timeSinceLastProgress) >= gs_crcProgressIntervalMS)
    {
        _timeSinceLastProgress = GetTick();
	    // Let the protocol know that we've made progress.
        emit verifyProgress(bytesProcessed, totalBytes, bytesPerSecond, estimatedSecondsRemaining);
    }
}

void CRCMapVerifyProgressProxy::verifyThreadComplete()
{
	emit verifyCompleted();
}

void CRCMapVerifyProgressProxy::Start()
{
    _verifyThread.start();
}

void CRCMapVerifyProgressProxy::Cancel()
{
	_verifyThread._shouldCancel = true;
}

void CRCMapVerifyProgressProxy::Cleanup()
{
    // We are being destroyed, so don't let our signals get emitted anymore
    QObject::disconnect(this, 0, 0, 0);

    // Mark the operation as canceled to ensure it ends
    _verifyThread._shouldCancel = true;

    // Wait on our thread to finish
    if (_verifyThread.isRunning())
        _verifyThread.wait();

    // Finally, delete ourselves later on the event loop
    this->deleteLater();
}

CRCMap::CRCMap() :
	QObject(NULL),
    _isLoaded(false),
    _hasChanges(false),
    _mapLock(QMutex::Recursive) // We need this because some functions here call others
{
	// To be used only for static calls
}

CRCMap::CRCMap(const QString& crcMapPath) :
    QObject(NULL),
    _crcMapPath(crcMapPath),
    _isLoaded(false),
    _hasChanges(false),
    _mapLock(QMutex::Recursive) // We need this because some functions here call others
{

}

CRCMap::~CRCMap()
{
	// Don't let our object get deleted out from under us while cleaning up
	{
		QMutexLocker lock(&_mapLock);
	}
}

const QString& CRCMap::getCRCMapPath()
{
    return _crcMapPath;
}

bool CRCMap::SafeWriteString(QFile* file, const QString& string)
{
	// wchar_t is 4 bytes on Mac, but we want to use UTF16 (2-byte) strings
	qint64 len = string.length() * sizeof(char16_t);

	if (file->write(reinterpret_cast<const char*>(string.utf16()), len) == len)
		return true;
	return false;
}

// This version specifically does not change the calculated size or CRC (safer than trying to infer from default parameters)
void CRCMap::AddModify(const QString& file, quint32 crc, qint64 size, const QString& id)
{
    QMutexLocker lock(&_mapLock);

    // If we've added stuff to this map, we can consider this loaded
    _isLoaded = true;
    _hasChanges = true;

    QString mapKey = file;
#ifdef ORIGIN_PC
    mapKey = file.toLower();
#endif
	CRCMapData& crcData = _crcMap[mapKey];
	crcData.crc = crc;
	crcData.size = size;
    crcData.id = id;
}

void CRCMap::SetContentId(const QString& file, const QString& id)
{
    QMutexLocker lock(&_mapLock);

    // If we've added stuff to this map, we can consider this loaded
    _isLoaded = true;
    _hasChanges = true;

    QString mapKey = file;
#ifdef ORIGIN_PC
    mapKey = file.toLower();
#endif

    CRCMapData& crcData = _crcMap[mapKey];
    crcData.id = id;
}

bool CRCMap::SetJobId(const QString& file, const QString& jobId)
{
    QMutexLocker lock(&_mapLock);

    _hasChanges = true;

    QString mapKey = file;
#ifdef ORIGIN_PC
    mapKey = file.toLower();
#endif

    if (_crcMap.contains(mapKey))
    {
        _crcMap[mapKey].jobId = jobId;
        return true;
    }
    return false;
}

bool CRCMap::Contains(const QString& file)
{
    QMutexLocker lock(&_mapLock);

    QString mapKey = file;
#ifdef ORIGIN_PC
    mapKey = file.toLower();
#endif
	return _crcMap.contains(mapKey);
}

void CRCMap::Remove(const QString& file)
{
	QMutexLocker lock(&_mapLock);

    _hasChanges = true;

    QString mapKey = file;
#ifdef ORIGIN_PC
    mapKey = file.toLower();
#endif
	if (_crcMap.contains(mapKey))
		_crcMap.remove(mapKey);
}

CRCMapData CRCMap::GetCRC(const QString& file)
{
    QMutexLocker lock(&_mapLock);

    QString mapKey = file;
#ifdef ORIGIN_PC
    mapKey = file.toLower();
#endif
	return _crcMap[mapKey];
}

void CRCMap::Clear()
{
	QMutexLocker lock(&_mapLock);
    
    _isLoaded = false;
    _hasChanges = true;
	_crcMap.clear();
}

bool CRCMap::ClearDiskFile()
{
    QMutexLocker lock(&_mapLock);

    return QFile::remove(_crcMapPath);
}

qint64 CRCMap::GetSizeForId(const QString& id)
{
    QMutexLocker lock(&_mapLock);

    qint64 totalBytes = 0;

    // Calculate the total size of our data
    for (QMap<QString, CRCMapData>::Iterator it = _crcMap.begin(); it != _crcMap.end(); it++)
    {
        if (id.isEmpty() || (*it).id.compare(id, Qt::CaseInsensitive) == 0)
        {
            totalBytes += it.value().size;
        }
    }

    return totalBytes;
}

QList<QString> CRCMap::GetAllKeys()
{
    QMutexLocker lock(&_mapLock);

    return _crcMap.keys();
}

CRCMapVerifyProgressProxy* CRCMap::VerifyBegin(bool quick, const QString& rootPath, const QString& id /*= ""*/)
{
    CRCMapVerifyProgressProxy *verifyProxy = new CRCMapVerifyProgressProxy();

	// Set up worker thread (caller must start it)
	verifyProxy->_verifyThread.setJobDetails(this, verifyProxy, quick, rootPath, id);

    return verifyProxy;
}

void CRCMap::VerifyEnd(CRCMapVerifyProgressProxy* verifyProxy)
{
    verifyProxy->Cleanup();
}

bool CRCMap::HasChanges() const
{
    return _hasChanges;
}

bool CRCMap::Save()
{
    QMutexLocker lock(&_mapLock);

	QFile file(_crcMapPath);
	if (!file.open(QIODevice::WriteOnly))
		return false;

	// Write Unicode Signature
	unsigned char sig[2];
	sig[0] = (unsigned char) 0xFF;
	sig[1] = (unsigned char) 0xFE;	
	file.write((const char*)sig, 2);

	// Walk all the entires in the map and write them
	for (QMap<QString, CRCMapData>::Iterator it = _crcMap.begin(); it != _crcMap.end(); it++)
	{
		CRCMapData& crcData = it.value();

		Parameterizer param;

        // Store the CRC map entries (never store the calculated CRC)
		param.SetString("file", it.key());
		param.Set64BitInt("crc", crcData.crc);
		param.Set64BitInt("size", crcData.size);
        param.SetString("id", crcData.id);
        param.SetString("jobid", crcData.jobId);

		QString sParams;
		if (!param.GetPackedParameterString(sParams))
			continue;

		// Append a newline
		sParams.append("\n");

		// Write it to the file
		SafeWriteString(&file, sParams);
	}

#ifdef ORIGIN_MAC
    
    // Explicitly set relaxed permissions to allow updates by other users.
    file.setPermissions( file.permissions() | QFile::WriteOwner|QFile::WriteGroup|QFile::WriteUser|QFile::WriteOther );
    
#endif
    
	file.close();

    // Clear the has changes flag
    _hasChanges = false;

	return true;
}

bool CRCMap::Load()
{
    QMutexLocker lock(&_mapLock);

    if (_isLoaded)
        return true;

	Clear();

	QString sFullCRCMap;
	if (!StringHelpers::LoadQStringFromFile(_crcMapPath, sFullCRCMap))
	{
		return false;
	}

	int nIndex = 0;
	while (nIndex >= 0 && nIndex < sFullCRCMap.length())
	{
		int nStartLine = nIndex;
		if (nStartLine < 0)
			break;

		int nEndLine = sFullCRCMap.indexOf("\n", nStartLine+1);
		if (nEndLine < 0)		// if there are no more new lines, then we're at the end of the file
			nEndLine = sFullCRCMap.length();	

		// absorb all newline characters
		while (nEndLine < sFullCRCMap.length() && (sFullCRCMap.at(nEndLine) == '\r' || sFullCRCMap.at(nEndLine) == '\n'))
			nEndLine++;	

		QString sLine = sFullCRCMap.mid(nStartLine, nEndLine-nStartLine);
		
		// Parse an entry
		Parameterizer param;
		if (!param.Parse(sLine, true))
			return false;

		QString fileName = param.GetString("file");
		quint32 crc = (quint32) param.Get64BitInt("crc");
		qint64 size = param.Get64BitInt("size");
        QString id = param.GetString("id");
        QString jobId = param.GetString("jobid");

		// Add the entry
		this->AddModify(fileName, crc, size, id);

        if (!jobId.isEmpty())
        {
            this->SetJobId(fileName, jobId);
        }

		nIndex = nEndLine;	// advance the index
	}

    _isLoaded = true;
	return true;
}

bool CRCMap::IsLoaded()
{
    return _isLoaded;
}

qint64 CRCMap::GetFileLength(const QString& file)
{
    qint64 fileSize = 0;

    // Qt handles shortcut (.lnk) in an utterly stupid way on Windows, it treats them as symlinks
    // Furthermore none of the QFile objects can operate on the .lnk file itself, they all go to the file it points to, which is incorrect for our purposes
    // Therefore the only safe way to get the file size is using the native APIs (this just uses Qt on Mac)
    EnvUtils::GetFileDetailsNative(file, fileSize);

    return fileSize;
}

quint32 CRCMap::GetCRCInitialValue()
{
	return 0xffffffff;
}

quint32 CRCMap::CalculateCRC(quint32 previous, const void* data, unsigned int dataLen)
{
	size_t nLength = dataLen;
	uint32_t nInitialValue = previous;

	const uint8_t* pData8 = (const uint8_t*)data;

	while(nLength >= 8)
	{
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);
		nLength -= 8;
	}

	while(nLength--)
		nInitialValue = gs_crc32Table[(nInitialValue ^ *pData8++) & 0xff] ^ (nInitialValue >> 8);

	return nInitialValue;
}

quint32 CRCMap::CalculateFinalCRC(quint32 crc)
{
	return crc ^ GetCRCInitialValue();
}

quint32 CRCMap::CalculateCRCForFile(const QString& file, qint64 maxReadBytes, bool finalize)
{
	// Let someone call this more easily (fewer params), so tell it we don't care about progress, etc
	return _CalculateCRCForFile(NULL, file, maxReadBytes, finalize, false, 0, 0);
}

quint32 CRCMap::_CalculateCRCForFile(CRCMapVerifyProgressProxy *verifyProxy, const QString& file, qint64 maxReadBytes, bool finalize, bool wantProgress, qint64 bytesProcessed, qint64 bytesTotal)
{
	if (!EnvUtils::GetFileExistsNative(file))
	{
		return (quint32)-1;
	}

    // Make sure we have somewhere to send progress!
    if (!verifyProxy)
        wantProgress = false;

#ifdef ORIGIN_MAC // Need to grab the symbolic links as a relative path
    // Symlinks are processed a bit differently than regular files - the content of a CRC is
    // the path to what it points to
    //    
    QFileInfo finfo (file);
    if (finfo.isSymLink())
    {
        // We want to get the size of the relative link
        char linktarget[512];
        memset(linktarget,0,sizeof(linktarget));
        ssize_t targetSize = readlink(qPrintable(file), linktarget, sizeof(linktarget)-1);
        
        quint32 sym_crc = CalculateCRC(GetCRCInitialValue(), linktarget, targetSize);
        
        // If they wanted progress, we'll update it (with mildly fakey data)
		if (wantProgress)
		{
			verifyProxy->_transferStats.onDataReceived(targetSize);
			verifyProxy->verifyThreadUpdateProgress(bytesProcessed + targetSize, bytesTotal, (qint64)verifyProxy->_transferStats.bps(), verifyProxy->_transferStats.estimatedTimeToCompletion(true));
		}

        if (finalize)
        {
            sym_crc = CalculateFinalCRC(sym_crc);
        }
        
        return sym_crc;
    }
#endif        

	QFile resumeFile(file);
	qint64 fileSize = CRCMap::GetFileLength(file);
   
	if (maxReadBytes == -1)
	{
		maxReadBytes = fileSize;
	}

	if (maxReadBytes > fileSize)
	{
		return (quint32)-1;
	}

	if (fileSize == 0)
		return (quint32)-1;

	if (!resumeFile.open(QIODevice::ReadOnly))
	{
		return (quint32)-1;
	}

	quint32 crc = GetCRCInitialValue();

	// Read a megabyte at a time
	qint64 totalBytesRead = 0;
	qint64 bytesRemaining = qMin(fileSize, maxReadBytes);
	qint64 maxBytesPerRead = qMin(bytesRemaining, (qint64)(1024 * 1024));
	uint8_t* buffer = new uint8_t[maxBytesPerRead];

	while (totalBytesRead < maxReadBytes)
	{
		// If we've been asked to cancel
		if (verifyProxy && verifyProxy->_verifyThread._shouldCancel)
		{
			crc = 0;
			break;
		}

		memset(buffer, 0, maxBytesPerRead);

		qint64 bytesThisRead = qMin(bytesRemaining, maxBytesPerRead);
		qint64 bytesRead = resumeFile.read((char*)buffer, bytesThisRead);

		if (bytesRead != bytesThisRead)
		{
			resumeFile.close();
			delete[] buffer;
			return (quint32)-1;
		}

		// Process the bytes we've read so far
		crc = CalculateCRC(crc, buffer, bytesThisRead);

		totalBytesRead += bytesRead;
		bytesRemaining -= bytesRead;

		// If they wanted progress, we'll update it
		if (wantProgress)
		{
			verifyProxy->_transferStats.onDataReceived((quint32)bytesRead);
			verifyProxy->verifyThreadUpdateProgress(bytesProcessed + totalBytesRead, bytesTotal, (qint64)verifyProxy->_transferStats.bps(), verifyProxy->_transferStats.estimatedTimeToCompletion(true));
		}
	}

	resumeFile.close();
	delete[] buffer;
	if (finalize)
		return CalculateFinalCRC(crc);
	return crc;
}

bool CRCMap::LoadLegacy(const QString& filename, QHash<QString, quint32>& legacyCrcMap)
{
	legacyCrcMap.clear();

    QFile legacyCrcFile(filename);
    if (!legacyCrcFile.open(QIODevice::ReadOnly))
    {
        return false;
    }

    const QString fileCRCMapTag("FileCRCMap");
    char tempBuf[4096];
    memset(tempBuf, 0, 4096);

    qint64 bytesRead = legacyCrcFile.read(tempBuf, fileCRCMapTag.size());
    if (bytesRead != fileCRCMapTag.size() || fileCRCMapTag.compare(QString(tempBuf), Qt::CaseInsensitive) != 0)
    {
        legacyCrcFile.close();
        return false;
    }

    bool result = false;
	qint32 numEntries = 0;
    bytesRead = legacyCrcFile.read((char*)&numEntries, sizeof(numEntries));
    if (bytesRead == sizeof(numEntries))
    {
        for (size_t count = 0; count < (size_t)numEntries; ++count)
        {
            qint32 filenameLength = 0;
            bytesRead = legacyCrcFile.read((char*)&filenameLength, sizeof(filenameLength));

            if (bytesRead == sizeof(filenameLength) && filenameLength < 4096)
            {
                bytesRead = legacyCrcFile.read(tempBuf, filenameLength);
                if (bytesRead == filenameLength)
                {
                    QString filename = QString::fromWCharArray((wchar_t*) &tempBuf[0], filenameLength/sizeof(wchar_t));
                    quint32 crc = 0;
                    bytesRead = legacyCrcFile.read((char*)&crc, sizeof(crc));
                    legacyCrcMap.insert(filename, crc);
                }
            }
        }

        result = true;
    }
    
    legacyCrcFile.close();
    return result;
}

bool CRCMap::ConvertLegacy(const QString& legacyFilename, const QString& contentInstallPath,
    const QString& id, CRCMap& map)
{
    QHash<QString, quint32> legacyCrcMap;
    if (contentInstallPath.isEmpty() || legacyFilename.isEmpty() || !LoadLegacy(legacyFilename, legacyCrcMap))
    {
        return false;
    }

    QString installPath(contentInstallPath);
    installPath.replace("\\", "/");
    if (!installPath.endsWith("/"))
    {
        installPath.append("/");
    }

    for (QHash<QString, quint32>::const_iterator i = legacyCrcMap.constBegin(); i != legacyCrcMap.constEnd(); ++i)
    {
        QString filePath(i.key());
        quint32 crc = i.value();
        if (!filePath.isEmpty())
        {
            filePath.replace("\\", "/");
            if (filePath.startsWith("/"))
            {
                filePath = filePath.mid(1);
            }
            
            QFileInfo fileInfo(installPath + filePath);
            if (fileInfo.exists())
            {
                map.AddModify(filePath, crc, fileInfo.size(), id);
            }
           
        }
    }

    return true;
}

bool CRCMap::GenerateFolderCRCs(const QString& folderPath, const QString& id, CRCMap& map, const QString& baseFolderPath /* = "" */)
{
    if (folderPath.isEmpty())
    {
        return false;
    }

    QString folder(folderPath);
    folder.replace("\\", "/");
    if (!folder.endsWith("/"))
    {
        folder.append("/");
    }

    QDir dir(folder);
    if (dir.exists())
    {
        dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
		QFileInfoList finfoList = dir.entryInfoList();
		for (QFileInfoList::Iterator iter = finfoList.begin(); iter != finfoList.end(); iter++)
		{
			QFileInfo &entry = *iter;

            if (entry.isDir())
            {
                GenerateFolderCRCs(entry.absoluteFilePath(), id, map, baseFolderPath.isEmpty() ? folder : baseFolderPath);
            }
            else
            {
                QString s = entry.filePath();
                qint64 fileSize = entry.size();
                QString relativePath(entry.absoluteFilePath());
                relativePath.remove(baseFolderPath);
                quint32 crc = CalculateCRCForFile(entry.absolutePath(), fileSize);

                map.AddModify(relativePath, crc, fileSize, id);
            }
        }

        return true;
    }

    return false;
}

void CRCMap::CalculateCRCPatchBytes(quint32 crcStart, quint32 crcTarget, quint8 *patchBytes)
{
    quint32 crcState[5];
    quint32 temp;

    crcState[0] = crcStart;
    crcState[4] = crcTarget;
    crcState[3] = (crcState[4] << 8 ) ^ gs_invcrc32Table[(quint8)(crcState[4] >> 24)] & 0xFFFFFF00L;
    crcState[2] = (crcState[3] << 8 ) ^ gs_invcrc32Table[(quint8)(crcState[3] >> 24)] & 0xFFFF0000L;
    crcState[1] = (crcState[2] << 8 ) ^ gs_invcrc32Table[(quint8)(crcState[2] >> 24)] & 0xFF000000L;

    temp = crcState[0] ^ gs_invcrc32Table[(quint8)(crcState[1] >> 24)];
    patchBytes[0] = temp & 0xFF;
    crcState[1] |= temp >> 8;

    temp = crcState[1] ^ gs_invcrc32Table[(quint8)(crcState[2] >> 24)];
    patchBytes[1] = temp & 0xFF;
    crcState[2] |= temp >> 8;

    temp = crcState[2] ^ gs_invcrc32Table[(quint8)(crcState[3] >> 24)];
    patchBytes[2] = temp & 0xFF;
    crcState[3] |= temp >> 8;

    temp = crcState[3] ^ gs_invcrc32Table[(quint8)(crcState[4] >> 24)];
    patchBytes[3] = temp & 0xFF;
}

CRCMapFactory* CRCMapFactory::instance()
{
    if (!g_crcMapFactory)
    {
        g_crcMapFactory = new CRCMapFactory();
    }
    return g_crcMapFactory;
}

CRCMapFactory::CRCMapFactory()
{
    // Nothing to do right now
}

CRCMapFactory::~CRCMapFactory()
{
    // Nothing to do right now
}

CRCMap* CRCMapFactory::getCRCMap(const QString& crcMapPath)
{
    QMutexLocker lock(&m_mapLock);

    if (m_crcMaps.contains(crcMapPath))
    {
        CRCMapInstanceData &instance = m_crcMaps[crcMapPath];

        ++instance.m_refCount;

        ORIGIN_LOG_EVENT << "CRC Map created for : " << crcMapPath << " Ref Count: " << instance.m_refCount;

        return instance.m_map;
    }
    else
    {
        CRCMapInstanceData instance;

        instance.m_map = new CRCMap(crcMapPath);
        ++instance.m_refCount;
        m_crcMaps.insert(crcMapPath, instance);

        ORIGIN_LOG_EVENT << "CRC Map created for : " << crcMapPath << " Ref Count: " << instance.m_refCount;

        return instance.m_map;
    }
}

void CRCMapFactory::releaseCRCMap(CRCMap* crcMap)
{
    QMutexLocker lock(&m_mapLock);

    if (!crcMap)
        return;

    QString crcMapPath = crcMap->getCRCMapPath();

    if (m_crcMaps.contains(crcMapPath))
    {
        CRCMapInstanceData &instance = m_crcMaps[crcMapPath];

        --instance.m_refCount;

        ORIGIN_LOG_EVENT << "CRC Map released for : " << crcMapPath << " Ref Count: " << instance.m_refCount;
        
        // If there are no more references to this map, remove it
        if (instance.m_refCount == 0)
        {
            delete instance.m_map;
            instance.m_map = NULL;

            m_crcMaps.remove(crcMapPath);

            ORIGIN_LOG_EVENT << "CRC Map destroyed for : " << crcMapPath;
        }
    }
}

} // namespace Downloader
} // namespace Origin

