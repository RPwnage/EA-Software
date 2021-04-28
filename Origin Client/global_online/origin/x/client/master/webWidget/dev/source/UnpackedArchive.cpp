#include <QCryptographicHash>
#include <QDebug>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "libarchive/archive.h"
#include "libarchive/archive_entry.h"

#include "UnpackedArchive.h"
#include "WidgetPackage.h"

namespace
{
    ///
    /// Returns the 8bit path name for a Unicode path name
    ///
    /// This only really needs special handling on Windows
    ///
    QByteArray narrowPathName(const QString &widePathName)
    {
#ifdef _WIN32
        wchar_t shortPath[MAX_PATH];

        if (!GetShortPathName(QDir::toNativeSeparators(widePathName).utf16(), shortPath, MAX_PATH))
        {
            return QByteArray();
        }

        return QString::fromUtf16(shortPath).toLatin1();
#else
        // Every modern *nix installation uses UTF-8 filenames
        return widePathName.toUtf8();
#endif
    }
}

namespace WebWidget
{
    QString UnpackedArchive::unpackedFilePath(const QString &canonicalPackagePath) const
    {
        // Use the SHA-1 sum of the filename
        // This should allow us to be cheaply case sensitive on Windows
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(canonicalPackagePath.toUtf8());
        
        return mRootDir.absoluteFilePath(hash.result().toHex());
    }
        
    UnpackedArchive UnpackedArchive::unpackFromCompressedPackage(const QString &rootPath, const QString &packagePath)
    {
        struct archive *a;
        int result;

        if (!QDir("/").mkpath(rootPath))
        {
            return UnpackedArchive();
        }

        a = archive_read_new();
        // Support all compression formats but only on ZIP files
        archive_read_support_compression_all(a);
        archive_read_support_format_zip(a);

        result = archive_read_open_filename(a, narrowPathName(packagePath).constData(), 10240);

        UnpackedArchive update(rootPath);

        // unpackPackageArchive is split out so we can add an archive_read_free() to all of its exit paths
        if ((result != ARCHIVE_OK) || !update.unpackPackageArchive(a))
        {
            archive_read_free(a);

            update.remove();
            return UnpackedArchive();
        }

        if (archive_read_free(a) != ARCHIVE_OK)
        {
            update.remove();
            return UnpackedArchive();
        }

        return update;
    }
    
    bool UnpackedArchive::unpackPackageArchive(archive *a)
    {
        struct archive_entry *entry;
        int result;

        while(archive_read_next_header(a, &entry) == ARCHIVE_OK)
        {
            const QString zipPathName(QString::fromUtf8(archive_entry_pathname(entry)));
            __LA_MODE_T mode = archive_entry_filetype(entry);

            if ((mode & AE_IFDIR) || zipPathName.endsWith("/"))
            {
                // Ignore directories
                archive_read_data_skip(a);
                continue;
            }
            else if (!(mode & AE_IFREG))
            {
                qWarning() << "Unexpected file type" << mode;
                return false;
            }

            QString canonicalPathName(WidgetPackage::canonicalizePath(zipPathName));

            if (canonicalPathName.isNull())
            {
                qWarning() << "Unable to canonicalize ZIP path name" << zipPathName;
                return false;
            }

            QFile unpackedFile(unpackedFilePath(canonicalPathName));

            if (!unpackedFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered))
            {
                return false;
            }
        
            // Read the package data from the archive in to the file
            while(true)
            {
                const void *buffer;
                size_t size;
                long long offset;

                result = archive_read_data_block(a, &buffer, &size, &offset);

                if (result == ARCHIVE_EOF)
                {
                    // File is finished
                    break;
                }
                else if (result != ARCHIVE_OK)
                {
                    // Error of some sort
                    return false;
                }

                // writev would be nice here
                unpackedFile.seek(offset);
                unpackedFile.write(static_cast<const char *>(buffer), size);
            }
        }

        return true;
    }
    
    bool UnpackedArchive::remove()
    {
        QFileInfoList unpackedFiles(mRootDir.entryInfoList(QDir::Files));

        // Note that we're intentionally not recursive as unpacked updates aren't recursive
        for(QList<QFileInfo>::const_iterator it = unpackedFiles.constBegin();
            it != unpackedFiles.constEnd();
            it++)
        {
            if (it->isSymLink())
            {
                // This is very dodgy; abort
                return false;
            }

            if (!QFile(it->absoluteFilePath()).remove())
            {
                // Can't remove
                return false;
            }
        }

        return QDir("/").rmdir(mRootDir.absolutePath());
    }
}
