#include "engine/cloudsaves/FileFingerprint.h"

#include <QCryptographicHash>
#include <QFile>
#include <QHash>

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    // This is a comprimise between memory use and syscall overhead
    static const unsigned int HashBlockSize = 65536;

    FileFingerprint FileFingerprint::fromFile(QFile &file)
    {
        // Calculate the size as we MD5 hash to they can't be out of sync
        // This is more important on POSIX where files can easily change under us
        qint64 totalSize = 0;
        QCryptographicHash md5Hasher(QCryptographicHash::Md5);

        while(true) 
        {
            QByteArray block = file.read(HashBlockSize);
            
            if (block.isEmpty())
            {
                // We're done
                break;
            }

            md5Hasher.addData(block);
            totalSize += block.size();
        }

        return FileFingerprint(totalSize, md5Hasher.result());
    }


    uint qHash(const CloudSaves::FileFingerprint &key)
    {
        // We could get away with just using the MD5 but this feels less dirty and
        // should be cheap
        return qHash(key.md5()) ^ 
               (key.size() & 0xFFFFF) ^ 
               ((key.size() >> 32) & 0xFFFF);
    }
}
}
}
