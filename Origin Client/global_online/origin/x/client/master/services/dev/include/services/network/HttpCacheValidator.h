#ifndef HTTPCACHEVALIDATOR_H
#define HTTPCACHEVALIDATOR_H

#include <QNetworkRequest>
#include <QByteArray>
#include <QUrl>
#include <QDataStream>

#include "services/plugin/PluginAPI.h"

class QNetworkReply;

namespace Origin
{
namespace Services
{

/**
 * Remembers validators associated with a given network reply
 */
class ORIGIN_PLUGIN_API HttpCacheValidator
{
public:
	friend ORIGIN_PLUGIN_API QDataStream& operator<<(QDataStream &, const HttpCacheValidator &);
	friend ORIGIN_PLUGIN_API QDataStream& operator>>(QDataStream &, HttpCacheValidator &);

	enum MatchType
	{
		NoMatch,
		WeakMatch,
		StrongMatch
	};

	// Creates an empty validator
	HttpCacheValidator();

	// Creates a new validator from the results of a network reply
	explicit HttpCacheValidator(const QNetworkReply *);

	// Adds If-None-Match or If-Modified-Since to a request
	bool addRequestHeaders(QNetworkRequest &req);

	// Determines if we have anything to validate on
	bool isEmpty();

	// Determines if the reply matches our validator
	MatchType match(const QNetworkReply *);

protected:
	QByteArray mEtag;
	QByteArray mLastModified;
	QUrl mUrl;
};

ORIGIN_PLUGIN_API QDataStream& operator<<(QDataStream &out, const Origin::Services::HttpCacheValidator &validator);
ORIGIN_PLUGIN_API QDataStream& operator>>(QDataStream &in, Origin::Services::HttpCacheValidator &validator);

}
}


#endif