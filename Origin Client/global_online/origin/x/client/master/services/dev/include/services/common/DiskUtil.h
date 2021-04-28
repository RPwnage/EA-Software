#ifndef DISKUTIL_H
#define DISKUTIL_H

#include <QString>
#include <QFile>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Util
    {
        namespace DiskUtil
        {
            /// \brief Deletes a file if it exists.
            /// \param sFileName A path to the file to delete.
            /// \note  If a regular deletion fails two other attempts are made, first by ensuring READ_ONLY flags are not set, then by ensuring the containing
            ///        folder's ACLs are set properly using the OriginClientService.
            ///         
            ///        TODO: Add the ability to set file ACLs to the OriginClientService instead of just folders.
            ///
            /// \return True if deletion was successful.
            ORIGIN_PLUGIN_API bool DeleteFileWithRetry( QString sFileName );
        }
    }
}

#endif // DISKUTIL_H
