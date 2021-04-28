#include <QDataStream>

#include "UpdateIdentifier.h"

namespace WebWidget
{
    bool UpdateIdentifier::operator==(const UpdateIdentifier &other) const
    {
        return updateUrl() == other.updateUrl() &&
            finalUrl() == other.finalUrl() &&
            etag() == other.etag();
    }

    bool UpdateIdentifier::operator!=(const UpdateIdentifier &other) const
    {
        return !(*this == other);
    }
}

QDataStream& operator<<(QDataStream &out, const WebWidget::UpdateIdentifier &identifier)
{
    return out << identifier.updateUrl() << identifier.finalUrl() << identifier.etag();
}

QDataStream& operator>>(QDataStream &in, WebWidget::UpdateIdentifier &identifier)
{
    QUrl updateUrl;
    QUrl finalUrl;
    QByteArray etag;

    in >> updateUrl >> finalUrl >> etag;
    identifier = WebWidget::UpdateIdentifier(updateUrl, finalUrl, etag);

    return in;
}
