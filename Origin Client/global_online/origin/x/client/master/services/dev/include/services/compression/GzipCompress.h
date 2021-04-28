#ifndef _GZIPCOMPRESS_H
#define _GZIPCOMPRESS_H

#include <zlib.h>
#include <QByteArray>
#include <QFile>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
	    ORIGIN_PLUGIN_API QByteArray gzipData(const char *data, int size);
	    ORIGIN_PLUGIN_API QByteArray gzipData(const QByteArray &);
        ORIGIN_PLUGIN_API bool gzipFile(QFile &, QFile*);
    }
}

#endif