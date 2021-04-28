#include <QUuid>

#include "InstalledWidget.h"
#include "UnpackedArchive.h"
#include "UnpackedArchiveFileProvider.h"

namespace WebWidget
{
    InstalledWidget::InstalledWidget(WidgetFileProvider *fileProvider, const SecurityOrigin &securityOrigin) :
       mSecurityOrigin(securityOrigin),
       mNull(false),
       mPackage(fileProvider)
    {
    }
    
    InstalledWidget::InstalledWidget(const UnpackedArchive &unpackedArchive, const QString &authority) :
        mNull(false),
        mPackage(new UnpackedArchiveFileProvider(unpackedArchive))
    {
        QString originHost(authority);

        if (originHost.isNull())
        {
            originHost = InstalledWidget::generateAuthority();
        }

        mSecurityOrigin = SecurityOrigin("widget", originHost, SecurityOrigin::UnknownPort);
    }

    QString InstalledWidget::generateAuthority()
    {
        return QUuid::createUuid().toString()
            .replace('{', "")
            .replace('}', "");
    }

    bool InstalledWidget::isNull() const
    {
        return package().isNull();
    }
}