#ifndef _WEBWIDGET_UNPACKEDUPDATE_H
#define _WEBWIDGET_UNPACKEDUPDATE_H

#include "WebWidgetPluginAPI.h"
#include "UnpackedArchive.h"
#include "UpdateIdentifier.h"

namespace WebWidget
{
    ///
    /// Represents a downloaded and unpacked update to a widget
    ///
    /// Unpacked updates are created and managed by UpdateCache.
    ///
    class WEBWIDGET_PLUGIN_API UnpackedUpdate : public UnpackedArchive
    {
    public:
        ///
        /// Creates a null UnpackedUpdate
        ///
        UnpackedUpdate() {}

        ///
        /// Creates a new UnpackedUpdate
        ///
        /// \param  identifier Identifier for the unpacked update
        /// \param  rootDir    Filesystem root of the unpacked widget update
        ///
        UnpackedUpdate(const UpdateIdentifier &identifier, const QDir &rootDir) :
            UnpackedArchive(rootDir),
            mIdentifier(identifier) {}

        ///
        /// Returns the identifier for the unpacked widget update
        ///
        UpdateIdentifier identifier() const { return mIdentifier; }

        ///
        /// Creates an UnpackedUpdate from a zipped widget package on the local filesystem
        ///
        /// \param  identifier   Identifier for the unpacked update
        /// \param  rootPath     Filesystem root to unpack the widget package to. This directory will be created
        ///                      if it does not exist.
        /// \param  packagePath  Filesystem path to the zipped widget package
        /// \return Non-null UnpackedUpdate on success
        ///
        static UnpackedUpdate unpackFromCompressedPackage(const UpdateIdentifier &identifier, const QString &rootPath, const QString &packagePath);

    private:
        ///
        /// Creates an UnpackedUpdate from an UnpackedArchive and UpdateIdentifier
        ///
        /// This is a helper for UnpackedUpdate::unpackFromCompressedPackage()
        ///
        UnpackedUpdate(const UnpackedArchive &, const UpdateIdentifier &);

        UpdateIdentifier mIdentifier;
    };
}

#endif