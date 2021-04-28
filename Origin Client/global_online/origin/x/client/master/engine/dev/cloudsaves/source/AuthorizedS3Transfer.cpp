#include <QtConcurrentRun>
#include <QDebug>

#include "services/debug/DebugService.h"
#include "services/compression/GzipCompress.h"
#include "engine/cloudsaves/AuthorizedS3Transfer.h"

namespace
{
    bool gzipFile(const QString &path, QFile *zipFile)
    {
        QFile sourceFile(path);

        if (!sourceFile.open(QIODevice::ReadOnly))
        {
            ORIGIN_ASSERT(0);
            // Broken
            return false;
        }

        return Origin::Services::gzipFile(sourceFile, zipFile);
    }
}

namespace Origin
{
namespace Engine
{
namespace CloudSaves
{
    void AuthorizedS3Upload::prepare()
    {
        if (!m_prepared)
        {
            m_prepared = true;
            m_zipSucceeded = QtConcurrent::run(gzipFile, sourcePath(), zipFile());
        }
    }
    
    bool AuthorizedS3Upload::performGzip() const
    {
        if (m_prepared)
        {
            // Wait for the result from our concurrent op
            return m_zipSucceeded.result();
        }
        else
        {
            return gzipFile(sourcePath(), zipFile());
        }
    }
}
}
}
