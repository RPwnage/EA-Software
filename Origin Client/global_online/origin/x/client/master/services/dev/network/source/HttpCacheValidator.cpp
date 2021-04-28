#include <QNetworkReply>
#include "services/network/HttpCacheValidator.h"
#include "services/log/LogService.h"

namespace Origin
{

namespace Services
{

HttpCacheValidator::HttpCacheValidator()
{
}

HttpCacheValidator::HttpCacheValidator(const QNetworkReply *reply)
{
	mEtag = reply->rawHeader("ETag");
	mLastModified = reply->rawHeader("Last-Modified");
	mUrl = reply->url();
}

bool HttpCacheValidator::addRequestHeaders(QNetworkRequest &req)
{
	if (req.url() != mUrl)
	{
		return false;
	}

	// RFC 2616 encourages us to send all the validators we have for maximum
	// compatibilty
	if (!mEtag.isEmpty())
	{
		req.setRawHeader("If-None-Match", mEtag);
	}

	if (!mLastModified.isEmpty())
	{
		req.setRawHeader("If-Modified-Since", mLastModified);
	}

	return true;
}

// Determines if we have anything to validate on
bool HttpCacheValidator::isEmpty()
{
	return mEtag.isEmpty() && mLastModified.isEmpty(); 
}

// Determines if the reply matches our validator
// This only uses ETag as Last-Modified isn't a strong validator
HttpCacheValidator::MatchType HttpCacheValidator::match(const QNetworkReply *otherReply)
{         
    if (otherReply->url() != mUrl)
	{
		// Not even close
		return NoMatch;
	}

    if (!mEtag.isEmpty() &&
		otherReply->hasRawHeader("ETag"))
	{
		// Yay, we can do an ETag match
		QByteArray otherEtag = otherReply->rawHeader("ETag");

		if (mEtag == otherEtag)
		{
			// The W/ prefix indicates this is a weak validator
    		return mEtag.startsWith("W/") ? WeakMatch : StrongMatch; 
		}
		else
		{
			return NoMatch;
		}
	}

	if (!mLastModified.isEmpty() &&
		otherReply->hasRawHeader("Last-Modified"))
	{
		QByteArray otherLastModified = otherReply->rawHeader("Last-Modified");
		if (mLastModified == otherLastModified)
		{
			// Last-Modified is a weak validator
			return WeakMatch;
		}
	}

	// Doesn't match at all
	return NoMatch;
}

QDataStream& operator<<(QDataStream &out, const Origin::Services::HttpCacheValidator &validator)
{
    out << validator.mUrl;
    out << validator.mEtag;
    out << validator.mLastModified;

    return out;
}

QDataStream& operator>>(QDataStream &in, Origin::Services::HttpCacheValidator &validator)
{
    in >> validator.mUrl;
    in >> validator.mEtag;
    in >> validator.mLastModified;

    return in;
}

}

}
