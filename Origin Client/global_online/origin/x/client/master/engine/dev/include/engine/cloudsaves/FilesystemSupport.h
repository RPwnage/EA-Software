#ifndef _CLOUDSAVES_FILESYSTEMSUPPORT_H
#define _CLOUDSAVES_FILESYSTEMSUPPORT_H

#include <QString>
#include <QDir>
#include "LocalInstance.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Engine
{ 
namespace CloudSaves
{
    namespace FilesystemSupport
    {
        enum CloudSavesDirectory
        {
            RoamingRoot,
            LocalRoot
        };

        ORIGIN_PLUGIN_API bool setFileMetadata(const LocalInstance &instance);
        ORIGIN_PLUGIN_API QDir cloudSavesDirectory(CloudSavesDirectory dir, bool subscriptionBackup = false);
    }
}
}
}

#endif
