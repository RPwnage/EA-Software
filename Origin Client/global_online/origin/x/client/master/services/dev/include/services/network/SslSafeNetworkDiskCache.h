#ifndef _SSLSAFENETWORKDISKCACHE_H
#define _SSLSAFENETWORKDISKCACHE_H

#include <QNetworkDiskCache>

#include "services/plugin/PluginAPI.h"

///
/// QNetworkDiskCache that only saves HTTPS requests if Cache-Control: public is specified
///
/// This should prevent personal information sent over HTTPS being committed to disk without the server explicitly
/// allowing it
///
class ORIGIN_PLUGIN_API SslSafeNetworkDiskCache : public QNetworkDiskCache
{
    Q_OBJECT
public:
	SslSafeNetworkDiskCache(QObject *parent = NULL);

	QIODevice *prepare(const QNetworkCacheMetaData &);
};

#endif