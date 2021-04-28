//    Copyright 2011-2013, Electronic Arts
//    All rights reserved.

#include "services/common/NetUtil.h"
#include "services/debug/DebugService.h"
#include <QString>
#include <QUrl>
#include <QHostInfo>

namespace Origin
{
    namespace Util
    {
        namespace NetUtil
        {
            QString ipFromUrl(const QString& url)
            {
                QString ipAddress;

                // Get the host from the download url
                QString hostName(QUrl(url).host());

                // Perform the lookup
                QHostInfo hostInfo(QHostInfo::fromName(hostName));

                if (hostInfo.error() == QHostInfo::NoError && !hostInfo.addresses().isEmpty()) 
                {
                    QHostAddress address = hostInfo.addresses().first();

                    // Convert to string representation of IP address
                    ipAddress = address.toString();
                }

                else if (QUrl(url).scheme().compare("file", Qt::CaseInsensitive) == 0)
                {
                    ipAddress = "FileOverride";
                }

                else if (hostInfo.error() == QHostInfo::HostNotFound)
                {
                    ipAddress = "HostNotFound";
                }

                else if (hostInfo.error() == QHostInfo::UnknownError)
                {
                    ipAddress = "UnknownError";
                }

                return ipAddress;
            }
        }
    }
}