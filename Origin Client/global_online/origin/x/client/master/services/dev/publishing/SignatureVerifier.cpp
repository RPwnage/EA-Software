#include "services/publishing/SignatureVerifier.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/config/OriginConfigService.h"
#include "TelemetryAPIDLL.h"

#include <QFile>

#include <openssl/rsa.h>
#include <openssl/sha.h>
//#include <openssl/objects.h>
#include <openssl/pem.h>
#include <openssl/err.h>

namespace Origin
{

namespace Services
{

namespace Publishing
{

const QByteArray SignatureVerifier::SIGNATURE_HEADER = QString("X-Origin-Signature").toUtf8();

const static QByteArray TELEM_TAG = "EntitlementSigning";

static EVP_PKEY* sPubKey = NULL;

void SignatureVerifier::release()
{
    if(sPubKey)
    {
        EVP_PKEY_free(sPubKey);
    }
}

void SignatureVerifier::init()
{
    if(sPubKey == NULL)
    {
        QFile pubKeyFile(":/certificates/ClientEntitlements.cer");
        QByteArray pubKeyContents;

        if(pubKeyFile.exists() && pubKeyFile.open(QIODevice::ReadOnly))
            pubKeyContents = pubKeyFile.readAll();

        if(pubKeyContents.isEmpty())
        {
            ORIGIN_LOG_ERROR << "Failed to read certificate";
            GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "Failed to read certificate");
        }
        else
        {
            char errorBuf[120];
            BIO* bio = BIO_new(BIO_s_mem());

            if(bio)
            {
                BIO_puts(bio, pubKeyContents.data());

                X509* x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);

                if(x509 != NULL)
                {
                    sPubKey = X509_get_pubkey(x509);
                    X509_free(x509);
                }

                BIO_free(bio);
            }

            if(sPubKey == NULL)
            {
                ORIGIN_LOG_ERROR << "Failed to read signature key: " << ERR_error_string(ERR_get_error(), errorBuf);
                GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "Failed to parse certificate");
            }
        }
    }
}

#if 0
bool SignatureVerifier::verify(const QByteArray &key, const QByteArray &body, const QByteArray &signature)
{
    if (Origin::Services::OriginConfigService::instance()->ecommerceConfig().entitlementSignatureVerificationDisabled)
    {
        return true;
    }

    if (signature.isEmpty())
    {
        ORIGIN_LOG_ERROR << "Signature verification failed: Entitlement signature empty";
        GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "Entitlement signature empty");
        return false;
    }

    if (NULL == sPubKey)
    {
        ORIGIN_LOG_ERROR << "Signature verification failed: No key found.";
        GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "No key found");
        return false;
    }

    bool signaturePassed = false;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(sPubKey);

    if (ctx != NULL)
    {
        if (EVP_PKEY_verify_init(ctx) > 0 &&
            EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PSS_PADDING) > 0 &&
            EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256()) > 0 &&
            EVP_PKEY_verify(ctx, (unsigned char *) key.data(), key.length()) &&
            EVP_DigestVerifyUpdate(ctx, (unsigned char *) body.data(), body.length()) &&
            EVP_DigestVerifyFinal(ctx, (unsigned char*) signature.data(), signature.length()))
        {
            ORIGIN_LOG_DEBUG_IF(Origin::Services::DebugLogToggles::entitlementLoggingEnabled) << "Signature verification successful.";
            signaturePassed = true;
        }

        EVP_PKEY_CTX_destroy(ctx);
    }

    if (!signaturePassed)
    {
        char errorBuf[120];
        ORIGIN_LOG_ERROR << "Signature verification failed: " << ERR_error_string(ERR_get_error(), errorBuf);
        GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "Signature failure");
    }

    return signaturePassed;
}
#else
bool SignatureVerifier::verify(const QByteArray &key, const QByteArray &body, const QByteArray &signature)
{
    if (Origin::Services::OriginConfigService::instance()->ecommerceConfig().entitlementSignatureVerificationDisabled)
    {
        return true;
    }

    if (signature.isEmpty())
    {
        ORIGIN_LOG_ERROR << "Signature verification failed: Entitlement signature empty";
        GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "Entitlement signature empty");
        return false;
    }

    if (NULL == sPubKey)
    {
        ORIGIN_LOG_ERROR << "Signature verification failed: No key found.";
        GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "No key found");
        return false;
    }

    bool signaturePassed = false;
    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();

    if (mdctx != NULL)
    {
        if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, sPubKey) &&
            EVP_DigestVerifyUpdate(mdctx, (unsigned char *) key.data(), key.length()) &&
            EVP_DigestVerifyUpdate(mdctx, (unsigned char *) body.data(), body.length()) &&
            EVP_DigestVerifyFinal(mdctx, (unsigned char*) signature.data(), signature.length()))
        {
            ORIGIN_LOG_DEBUG_IF(Origin::Services::DebugLogToggles::entitlementLoggingEnabled) << "Signature verification successful.";
            signaturePassed = true;
        }

        EVP_MD_CTX_destroy(mdctx);
    }

    if (!signaturePassed)
    {
        char errorBuf[120];
        ORIGIN_LOG_ERROR << "Signature verification failed: " << ERR_error_string(ERR_get_error(), errorBuf);
        GetTelemetryInterface()->Metric_ERROR_NOTIFY(TELEM_TAG.data(), "Signature failure");
    }

    return signaturePassed;
}
#endif
} // namespace Publishing

} // namespace Services

} // namespace Origin
