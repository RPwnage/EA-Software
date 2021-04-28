#include <QUrl>
#include <QUrlQuery>

#include "LogHelpers.h"

namespace Origin
{
namespace Downloader
{
namespace LogHelpers
{

QString logSafeDownloadUrl(const QUrl &url) 
{
    QUrl logUrl(url);
    
#ifndef _DEBUG
    QUrlQuery logUrlQuery(logUrl);
    
    //  Truncate Akamai's "sauth" parameter
    {
	    const QString sauth = logUrlQuery.queryItemValue("sauth");
	    if (!sauth.isEmpty())
	    {
		    // Chop out the middle part of sauth for security reasons
		    logUrlQuery.removeQueryItem("sauth");
		    logUrlQuery.addQueryItem("sauth", sauth.left(5) + "..." + sauth.right(5));
	    }
    }

    //  Truncate Level3's "token" parameter
    {
        const QString l3token = logUrlQuery.queryItemValue("token");
        if (!l3token.isEmpty())
        {
            // Chop out the middle part of sauth for security reasons
            logUrlQuery.removeQueryItem("token");
            logUrlQuery.addQueryItem("token", l3token.left(5) + "..." + l3token.right(5));
        }
    }
    
    logUrl.setQuery(logUrlQuery);
#endif
    
	return logUrl.toString();
}

}
}
}
