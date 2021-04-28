#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "WidgetDirectoryFileProvider.h"

namespace WebWidget
{
    WidgetDirectoryFileProvider::WidgetDirectoryFileProvider(const QString &localPath) : 
        mLocalPath(localPath)
    {
#ifdef WEBWIDGET_EMULATE_CASESENSITIVITY
        addValidPaths(localPath);
#endif
    }

#ifdef WEBWIDGET_EMULATE_CASESENSITIVITY
    void WidgetDirectoryFileProvider::addValidPaths(const QString &dir)
    {
        // Get the list of all files
        const QDir::Filters filters = QDir::NoDotAndDotDot | QDir::Readable | QDir::Files | QDir::Dirs;
        const QFileInfoList entries = QDir(dir).entryInfoList(filters);

        for(QList<QFileInfo>::const_iterator it = entries.constBegin();
            it != entries.constEnd();
            it++)
        {
            if (it->isDir())
            {
                // Search recursively
                addValidPaths(it->absoluteFilePath());
            }
            else if (it->isFile())
            {
                // Include as a valid path
                QString relativePath(QDir(mLocalPath).relativeFilePath(it->absoluteFilePath()));
                mValidPaths.append(relativePath);
            }
        }
    }
#endif

    QFileInfo WidgetDirectoryFileProvider::backingFileInfo(const QString &canonicalPackagePath) const
    {
#ifdef WEBWIDGET_EMULATE_CASESENSITIVITY
        if (!mValidPaths.contains(canonicalPackagePath))
        {
            return QFileInfo();
        }
#endif

        return QFileInfo(QDir(mLocalPath), canonicalPackagePath);
    }
}
