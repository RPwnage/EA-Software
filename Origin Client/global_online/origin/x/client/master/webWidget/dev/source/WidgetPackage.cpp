#include <QDebug>
#include <QStringList>

#include "WidgetPackage.h"
#include "WidgetFileProvider.h"

namespace
{
    // Implements http://www.w3.org/TR/widgets/#rule-for-identifying-the-media-type-of-a-file
    QByteArray mediaTypeForPath(const QString &path)
    {
        QString filename = path.split('/').last();

        if (!filename.contains('.'))
        {
            // No extension
            return QByteArray();
        }

        // If the only . appears at the beginning then abort
        if (filename.startsWith('.') && (filename.count('.') == 1))
        {
            return QByteArray();
        }

        QString extension = filename.split('.').last();

        // Make sure its non-empty and alphanumeric
        if (!QRegExp("[0-9a-zA-Z]+").exactMatch(extension))
        {
            return QByteArray();
        }

        // Lowercase the extension
        extension = extension.toLower();

        if (extension == "html")
            return "text/html";
        else if (extension == "htm")
            return "text/html";
        else if (extension == "css")
            return "text/css";
        else if (extension == "js")
            return "text/javascript";
        else if (extension == "xml")
            return "text/xml";
        else if (extension =="txt")
            return "text/plain";
        else if (extension == "wav")
            return "audio/x-wav";
        else if (extension == "xhtml")
            return "application/xhtml+xml";
        else if (extension == "xht")
            return "application/xhtml+xml";
        else if (extension == "gif")
            return "image/gif";
        else if (extension == "png")
            return "image/png";
        else if (extension == "ico")
            return "image/vnd.microsoft.icon";
        else if (extension == "svg")
            return "image/svg+xml";
        else if (extension == "jpg")
            return "image/jpeg";
        else if (extension == "mp3")
            return "audio/mp3";

        // Valid-looking extension but not recognized
        return QByteArray();
    }

    // Defined in http://www.w3.org/TR/widgets/#character-definitions
    // We don't filter out forward solidus as we treat that as a path separator
    // It's up to the widget extractor to reject those files
    bool containsZipForbiddenCharacters(const QString &path)
    {
        static const QString forbidden("\x7f<>:\"\\|?*\x5e\x60{}!");

        for(int i = 0; i < path.length(); i++)
        {
            if (forbidden.contains(path[i]))
            {
                return true;
            }
        }

        return false;
    }
}

namespace WebWidget
{
    WidgetPackage::WidgetPackage(const WidgetFileProvider *fileProvider) :
        mFileProvider(fileProvider)
    {
    }

    bool WidgetPackage::isNull() const
    {
        return mFileProvider == NULL;
    }
        
    WidgetPackageFile WidgetPackage::packageFile(const QString &packagePath, const QByteArray &providedMediaType, const QByteArray &providedEncoding) const
    {
        if (!mFileProvider)
        {
            // No file provider; return an non-existent file
            return WidgetPackageFile(packagePath);
        }
        
        // Find our canonical path
        QString canonicalPackagePath(WidgetPackage::canonicalizePath(packagePath));

        if (canonicalPackagePath.isEmpty())
        {
            // Empty or invalid path
            return WidgetPackageFile(packagePath);
        }

        const QFileInfo fileInfo(mFileProvider->backingFileInfo(canonicalPackagePath));

        if (fileInfo.exists())
        {
            QByteArray mediaType(providedMediaType);

            if (mediaType.isNull())
            {
                // Try to guess based on hardcoded file extension rules
                mediaType = mediaTypeForPath(canonicalPackagePath);
            }

            return WidgetPackageFile(packagePath, fileInfo, mediaType, providedEncoding);
        }
        else
        {
            // File not found
            return WidgetPackageFile(packagePath);
        }
    }

    QString WidgetPackage::canonicalizePath(const QString &path)
    {
        // Don't allow forbidden paths
        // This is important as some of them may have OS-specific meaning allowing the widget to escape its package.
        // \ on Windows is a particularly grave example
        if (containsZipForbiddenCharacters(path))
        {
            return QString();
        }

        // Rebuild the path after dealing with . and ..
        QStringList incomingPathParts(path.split("/"));
        QStringList outgoingPathParts;

        for(QList<QString>::const_iterator it = incomingPathParts.constBegin();
            it != incomingPathParts.end();
            it++)
        {
            if (*it == "")
            {
                // Multiple / is the same as a single /
            }
            else if (*it == ".")
            {
                // This referring to the current directory; discard it
            }
            else if (*it == "..")
            {
                // .. from the root is still the root
                if (!outgoingPathParts.empty())
                {
                    // Remove the last path item
                    outgoingPathParts.takeLast();
                }
            }
            else
            {
                // Add the path part on
                outgoingPathParts.append(*it);
            }
        }

        return outgoingPathParts.join("/");
    }
}
