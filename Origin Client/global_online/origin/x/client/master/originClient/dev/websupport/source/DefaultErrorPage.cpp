#include <QFile>
#include <QObject>

#include "services/debug/DebugService.h"
#include "DefaultErrorPage.h"
#include "Qt/originwebview.h"

namespace 
{
    QString tr(const char *source)
    {
        return QObject::tr(source);
    }
}

namespace Origin
{
namespace Client
{
    
DefaultErrorPage::DefaultErrorPage(Alignment align) :
    mAlignment(align)
{
}

QString DefaultErrorPage::offlineErrorHtml() const
{
    return buildErrorPage(
            tr("ebisu_client_not_connected_uppercase"),
            tr("ebisu_client_reconnect_message_text"));
}

QString DefaultErrorPage::recoveringErrorHtml() const
{
    return buildErrorPage(
            tr("ebisu_client_not_connected_uppercase"),
            tr("ebisu_client_refreshing_content_text"));
}

QString DefaultErrorPage::pageErrorHtml(PageErrorDetector::ErrorType, QNetworkReply *) const
{
    return buildErrorPage(
            tr("ebisu_client_service_unavailable_title"),
            tr("ebisu_client_service_unavailable_text"));
}
    
QString DefaultErrorPage::buildErrorPage(const QString &title, const QString &body) const
{
    // Build our filename
    QString fileSuffix = (mAlignment == CenterAlign) ? "" : "_dialog";
    QFile resourceFile(QString(":html/errorPage%1.html").arg(fileSuffix));

    if (!resourceFile.open(QIODevice::ReadOnly))
    {
        ORIGIN_ASSERT(0);
        return QString();
    }

    // Replace our stylesheet
    QString stylesheet = UIToolkit::OriginWebView::getErrorStyleSheet(mAlignment == CenterAlign);
    return QString::fromUtf8(resourceFile.readAll())
        .arg(stylesheet) // %1
        .arg(title) // %2
        .arg(body) // %3
        .arg(tr("application_name")); // %4
}

}
}
