#ifndef _LOGHELPERS_H
#define _LOGHELPERS_H

#include <QString>

#include "services/plugin/PluginAPI.h"

class QUrl;

namespace Origin
{
namespace Downloader
{
namespace LogHelpers
{

///
/// \brief Returns a version of a download URL with sensitive information removed.
/// \param url Download URL to clean.
///
ORIGIN_PLUGIN_API QString logSafeDownloadUrl(const QUrl &url);

}
}
}

#endif
