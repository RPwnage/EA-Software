#ifndef SIGNATUREVERIFIER_H
#define SIGNATUREVERIFIER_H

#include <QString>
#include <QVector>
#include <QMap>
#include "services/platform/PlatformService.h"

namespace Origin
{

namespace Services
{

namespace Publishing
{

class SignatureVerifier
{
public:
    static const QByteArray SIGNATURE_HEADER;

    static void init();
    static void release();
    static bool verify(const QByteArray &key, const QByteArray &body, const QByteArray &signature);
};

}

}

}

#endif // SIGNATUREVERIFIER_H
