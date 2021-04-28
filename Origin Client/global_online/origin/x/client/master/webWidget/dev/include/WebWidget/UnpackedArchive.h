#ifndef _WEBWIDGET_UNPACKEDARCHIVE_H
#define _WEBWIDGET_UNPACKEDARCHIVE_H

#include <QString>
#include <QDir>

#include "WebWidgetPluginAPI.h"

struct archive;

namespace WebWidget
{
    ///
    /// Represents an widget archive unpacked on the local filesystem
    ///
    /// Unlike widget directories which mirror the contents of the ZIP archive on the local filesystem, unpacked
    /// archives use an internal format which is not designed to be modified once the archive has been unpacked.
    ///
    /// UnpackedArchiveFileProvider can be use to expose the contents of an unpacked archive to the rest of the
    /// WebWidget framework.
    ///
    class WEBWIDGET_PLUGIN_API UnpackedArchive
    {
    public:
        ///
        /// Creates a null UnpackedArchive
        ///
        UnpackedArchive() : mNull(true) {}

        ///
        /// Creates a new UnpackedArchive
        ///
        /// \param  rootDir  Filesystem root of the unpacked widget archive. The contents of this directory should
        ///                  have been previously created by unpackFromCompressedPackage
        ///
        explicit UnpackedArchive(const QDir &rootDir) :
            mRootDir(rootDir),
            mNull(false) {}

        ///
        /// Returns if this is a null unpacked archive
        ///
        bool isNull() const { return mNull; }
        
        ///
        /// Returns the filesystem root of the unpacked archive
        ///
        QDir rootDirectory() const { return mRootDir; }

        ///
        /// Returns the expected unpacked file path for the canonical package path
        ///
        /// Unlike WidgetFileProvider::localFilesystemPath this will return a path even if the target file doesn't
        /// exist.
        ///
        /// \param  canonicalPackagePath  Canonical widget package path
        /// \return Absolute path for the unpacked file
        ///
        /// \sa UnpackedArchiveFileProvider
        ///
        QString unpackedFilePath(const QString &canonicalPackagePath) const;

        ///
        /// Removes the unpacked update from the filesystem including its root directory
        ///
        /// This is conservative in that it only attempts to remove files and directories that look like part of an
        /// unpacked archive. However, this happens in one pass so files may already be deleted when the removal is
        /// aborted. As a result the UnpackedArchive instance is invalid once this function is called regardless of 
        /// the return value.
        ///
        /// \return True if the root directory is removed from the filesystem, false if otherwise. 
        ///
        bool remove();

        ///
        /// Creates an UnpackedArchive from a zipped widget package on the local filesystem
        ///
        /// \param  rootPath     Filesystem root to unpack the widget package to. This directory will be created
        ///                      if it does not exist.
        /// \param  packagePath  Filesystem path to the zipped widget package
        /// \return Non-null UnpackedArchive on success
        ///
        static UnpackedArchive unpackFromCompressedPackage(const QString &rootPath, const QString &packagePath);

    private:
        // Helper for unpackFromCompressedPackage
        bool unpackPackageArchive(archive *a);

        QDir mRootDir;
        bool mNull;
    };
}

#endif
