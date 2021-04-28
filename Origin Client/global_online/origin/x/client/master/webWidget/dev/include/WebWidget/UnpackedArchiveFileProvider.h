#ifndef _WEBWIDGET_UNPACKEDUPDATEFILEPROVIDER_H
#define _WEBWIDGET_UNPACKEDUPDATEFILEPROVIDER_H

#include "WebWidgetPluginAPI.h"
#include "WidgetFileProvider.h"
#include "UnpackedArchive.h"

namespace WebWidget
{
    ///
    /// WidgetFileProvider that serves files from an UnpackedArchive
    ///
    class WEBWIDGET_PLUGIN_API UnpackedArchiveFileProvider : public WidgetFileProvider
    {
    public:
        ///
        /// Creates a new UnpackedArchiveFileProvider for the given UnpackedArchive
        ///
        /// \param unpackedArchive UnpackedArchive to serve files from
        ///
        UnpackedArchiveFileProvider(const UnpackedArchive &unpackedArchive);
        
        QFileInfo backingFileInfo(const QString &canonicalPackagePath) const;

        ///
        /// Returns the UnpackedArchive this file provider was constructed with
        ///
        UnpackedArchive unpackedArchive() const
        {
            return mUnpackedArchive;
        }

    private:
        UnpackedArchive mUnpackedArchive;
    };
}

#endif
