#include <QIODevice>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "WidgetPackageSignature.h"
#include <QCryptographicHash>

namespace
{
    using namespace WebWidget;

    const QByteArray SignatureMagic("originsigv1");
    const QString DefaultSignaturePackagePath("origin/signature");

    struct ParsedSignatureLine
    {
        QByteArray sha256Hash;
        QByteArray packagePath;

        bool isNull() const
        {
            return sha256Hash.isNull() || packagePath.isNull();
        }
    };

    QByteArray assembleLines(const QList<QByteArray> lines)
    {
        QByteArray ret;

        for(QList<QByteArray>::ConstIterator it = lines.constBegin();
                it != lines.constEnd();
                it++)
        {
            ret += *it + "\n";
        }

        return ret;
    }

    ParsedSignatureLine parseSignatureLine(const QByteArray &line)
    {
        QList<QByteArray> lineParts = line.split('\t');
        ParsedSignatureLine ret;

        if (lineParts.count() != 2)
        {
            return ret;
        }

        ret.sha256Hash = QByteArray::fromHex(lineParts[0]);
        ret.packagePath = lineParts[1];

        return ret;
    }

    bool verifySignature(const QCryptographicHash &hash, const QByteArray &signature, RSA *key)
    {
        const QByteArray hashResult(hash.result());

        if (hashResult.isNull())
        {
            return false;
        }

        return (RSA_verify(NID_sha256,
                reinterpret_cast<const unsigned char *>(hashResult.constData()), hashResult.size(),
                reinterpret_cast<const unsigned char*>(signature.constData()), signature.size(),
                key) == 1);
    }
}

namespace WebWidget
{
    WidgetPackageSignature::WidgetPackageSignature(QIODevice *signatureData, const RsaPublicKey &publicKey) :
        mPublicKey(publicKey),
        mValid(false)
    {
        QByteArray rawSignature = signatureData->readAll();

        // Only accept Unix newlines
        QList<QByteArray> signatureLines = rawSignature.split('\n');

        // We need at least 3 lines
        if (signatureLines.count() < 3)
        {
            return;
        }

        // Last line must be blank
        if (signatureLines.takeLast().length() != 0)
        {
            return;
        }

        // Second to last line is our self signature
        QByteArray selfSignature = QByteArray::fromBase64(signatureLines.takeLast());

        if (selfSignature.isNull())
        {
            return;
        }

        // Hash ourselves to check against the self signature
        QCryptographicHash selfHash(QCryptographicHash::Sha256);
        selfHash.addData(assembleLines(signatureLines));

        // Check to see if the self signature is valid
        if (!verifySignature(selfHash, selfSignature, publicKey.openSslKey()))
        {
            return;
        }

        // Make sure the top line is correct
        if (signatureLines.takeFirst() != SignatureMagic)
        {
            return;
        }

        // Parse all the remaining lines as package file signatures
        QHash<QString, QByteArray> packageFileSignatures;

        QByteArray lastPackagePath;
        for(QList<QByteArray>::ConstIterator it = signatureLines.constBegin();
                it != signatureLines.constEnd();
                it++)
        {
            ParsedSignatureLine signatureLine = parseSignatureLine(*it);

            if (signatureLine.isNull())
            {
                // Bad signature
                return;
            }

            if (!lastPackagePath.isNull() && (signatureLine.packagePath <= lastPackagePath))
            {
                // Duplicate or unordered package path
                return;
            }

            packageFileSignatures.insert(QString::fromUtf8(signatureLine.packagePath), signatureLine.sha256Hash);
            lastPackagePath = signatureLine.packagePath;
        }

        // We made it somehow
        mPackageFileSignatures = packageFileSignatures;
        mValid = true;
    }
    
    WidgetPackageSignature::WidgetPackageSignature() 
        : mValid(false)
    {
    }

    bool WidgetPackageSignature::isValid() const
    {
        return mValid;
    }
        
    QStringList WidgetPackageSignature::packagePaths() const
    {
        return mPackageFileSignatures.keys();
    }

    WidgetPackageSignatureEntry WidgetPackageSignature::entryForPackagePath(const QString &packagePath) const
    {
        QHash<QString, QByteArray>::ConstIterator it = mPackageFileSignatures.find(packagePath);

        if (it == mPackageFileSignatures.constEnd())
        {
            // Not found
            return WidgetPackageSignatureEntry();
        }

        return WidgetPackageSignatureEntry(*it);
    }
        
    QString WidgetPackageSignature::defaultPackagePath()
    {
        return DefaultSignaturePackagePath;
    }

    WidgetPackageSignatureEntry::WidgetPackageSignatureEntry(const QByteArray &sha256Hash) :
        mSha256Hash(sha256Hash),
        mNull(false)
    {
    }

    WidgetPackageSignatureEntry::WidgetPackageSignatureEntry() :
        mNull(true)
    {
    }

    bool WidgetPackageSignatureEntry::isNull() const
    {
        return mNull;
    }

    bool WidgetPackageSignatureEntry::verify(QIODevice *packageFile) const
    {
        if (isNull())
        {
            return false;
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);

        while(!packageFile->atEnd())
        {
            hash.addData(packageFile->read(65536));
        }

        return hash.result() == mSha256Hash;
    }
}

