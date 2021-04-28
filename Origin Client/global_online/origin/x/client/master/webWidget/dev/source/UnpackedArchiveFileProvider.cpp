#include <QFile>
#include "UnpackedArchiveFileProvider.h"

namespace WebWidget
{
    UnpackedArchiveFileProvider::UnpackedArchiveFileProvider(const UnpackedArchive &unpackedUpdate) :
        mUnpackedArchive(unpackedUpdate)
    {
    }

    QFileInfo UnpackedArchiveFileProvider::backingFileInfo(const QString &canonicalPackagePath) const
    {
        return QFileInfo(mUnpackedArchive.unpackedFilePath(canonicalPackagePath));
    }
}
