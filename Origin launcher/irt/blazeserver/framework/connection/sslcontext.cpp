/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class SslContext

    This class wraps the OpenSSL implementation.  It is primarily used as a place to handle
    initialization of the OpenSSL context.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/sslcontext.h"
#include "framework/util/shared/blazestring.h"
#include "framework/util/shared/bytearrayoutputstream.h"
#include "framework/util/shared/base64.h"

#include "framework/util/directory.h"
#include "framework/vault/vaultmanager.h"
#undef X509_NAME
#include "openssl/err.h"
#include "eathread/eathread_mutex.h"

#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileStream.h>

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/
SslContextManager::SslContextManager()
{
    mSSLContextState = SSLContextState::LOAD_NOT_STARTED;
}

SslContextManager::~SslContextManager()
{

}


// static
SslContextPtr SslContextManager::get(const char8_t* name /* = nullptr */)
{
    if ((name == nullptr) || (name[0] == '\0'))
        name = SSLConfig::DEFAULT_SSL_CONTEXT_NAME;

    ContextsByName::iterator itr = mContexts.find(name);
    if (itr == mContexts.end())
        return SslContextPtr();

    return itr->second;
}

// static
bool SslContextManager::pendingContextExists(const char8_t* name /* = nullptr */)
{
    if ((name == nullptr) || (name[0] == '\0'))
        name = SSLConfig::DEFAULT_SSL_CONTEXT_NAME;

    ContextsByName::iterator itr = mPendingContexts.find(name);
    if (itr == mPendingContexts.end())
        return false;

    return true;
}

// static
bool SslContextManager::initializeContextsInternal(const SSLConfig& config, bool initializePending)
{
    bool foundDefault = false;
    SSLConfig::ContextsMap::const_iterator itr = config.getContexts().begin();
    SSLConfig::ContextsMap::const_iterator end = config.getContexts().end();
    for(; itr != end; ++itr)
    {
        const char8_t* name = itr->first;
        if (!initializePending && mContexts.find(name) != mContexts.end())
        {
            BLAZE_WARN_LOG(Log::CONNECTION, "[SslContext].initializeContextsInternal: attempt to re-initialize existing context '" << name << "'.");
            continue;
        }

        SslContext* context = BLAZE_NEW SslContext(name);
        if (!context->initialize(*itr->second))
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initializeContextsInternal: unable to initialize '" << name << "' context.");
            delete context;
            if (initializePending)
                mPendingContexts.clear();
            return false;
        }
        if (initializePending)
            mPendingContexts[name] = context;
        else
            mContexts[name] = context;

        if (blaze_stricmp(name, SSLConfig::DEFAULT_SSL_CONTEXT_NAME) == 0)
            foundDefault = true;
    }
    if (!foundDefault)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initializeContextsInternal: no default context defined.");
        if (initializePending)
            mPendingContexts.clear();
        return false;
    }
    return true;
}

// static
bool SslContextManager::createContext(const char8_t* caCertDir, const char8_t* cipherList, const char8_t* contextName/* = nullptr*/,
                               SSLContextConfig::SSLProtocolVersion minProtocolVersion/* = SSLContextConfig::TLSV_1_1*/)
{
    if (contextName == nullptr)
        contextName = SSLConfig::DEFAULT_SSL_CONTEXT_NAME;

    if (mContexts.find(contextName) != mContexts.end())
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[SslContext].createContext: failure to create context '" << contextName << "'; already exists.");
        return false;
    }

    SslContext* context = BLAZE_NEW SslContext(contextName);
    if (!context->initialize(caCertDir, cipherList, minProtocolVersion))
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[SslContext].createContext: unable to initialize '" << contextName << "' context.");
        delete context;
        return false;
    }
    mContexts[contextName] = context;
    return true;
}

// static
void SslContextManager::initializeSsl()
{
    BLAZE_PUSH_ALLOC_OVERRIDE(MEM_GROUP_FRAMEWORK_NOLEAK, "OSSL Init"); //This has some memory we can't free
    SSL_library_init();
    SSL_load_error_strings();
    BLAZE_POP_ALLOC_OVERRIDE();
}

void SslContextManager::cleanupSsl()
{
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    ERR_free_strings();
}

// static
void SslContextManager::replaceOldContexts()
{
    if (mPendingContexts.empty())
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].replaceOldContexts: Old contexts not replaced - there are no pending contexts!");
    }
    else
    {
        mContexts.clear();
        for ( ContextsByName::const_iterator itr = mPendingContexts.cbegin(); itr != mPendingContexts.cend(); ++itr )
            mContexts[itr->first] = itr->second;
        mPendingContexts.clear();
    }
}

SslContext::SslContext(const char8_t* name)
    : mName(name),
      mContext(nullptr),
      mPathType(SSLContextConfig::VAULT_HOSTNAME),
      mExternalCert(true),
      mCertPath(nullptr),
      mKeyPath(nullptr),
      mCaCertDir(nullptr),
      mCaFile(nullptr),
      mRefCount(0)
{
    mCommonName[0] = '\0';

}

SslContext::~SslContext()
{
    if (mContext != nullptr)
        SSL_CTX_free(mContext);

    BLAZE_FREE(mCertPath);
    BLAZE_FREE(mKeyPath);
    BLAZE_FREE(mCaCertDir);
    BLAZE_FREE(mCaFile);

    gSslContextManager->cleanupSsl();

}

bool SslContext::initialize(const SSLContextConfig &config)
{
    bool rc = true;
    // Load all configured contexts
    bool verifyPeer = config.getVerifyPeer();
    mPathType = config.getPathType();
    mExternalCert = config.getExternalCert();
    const char8_t* key = config.getKey();
    const char8_t* cert = config.getCert();
    const char8_t* caCertDir = config.getCaCertDirectory();
    const char8_t* cipherList = config.getCipherList();
    mCaFile = blaze_strdup(config.getCaFile());
    SSLContextConfig::SSLProtocolVersion minProtocolVersion = config.getMinSSLProtocolVersion();

    BLAZE_INFO_LOG(Log::CONNECTION, "[SslContext].initialize: initializing context (" << mName << "), ssl version (" << SSLeay_version(SSLEAY_VERSION) << "), ssl (" << SSLeay_version(SSLEAY_BUILT_ON) << ")");

    for (const auto& trustedHash : config.getTrustedHashes())
    {
        mTrustedHashes.insert(trustedHash.c_str());
    }

    rc = initialize(verifyPeer, key, cert, caCertDir, cipherList, minProtocolVersion);

    return rc;
}

bool SslContext::initialize(const char8_t* caCertDir, const char8_t* cipherList, SSLContextConfig::SSLProtocolVersion minProtocolVersion)
{
    return initialize(true, nullptr, nullptr, caCertDir, cipherList, minProtocolVersion);
}

SSL* SslContext::createSsl(SOCKET sock)
{
    SSL* ssl = SSL_new(mContext);
    if (ssl != nullptr)
    {
        SSL_set_fd(ssl, (int32_t)sock);
        SSL_set_msg_callback(ssl, SslContext::logSslMessage);
        SSL_set_info_callback(ssl, SslContext::logSslInfo);
    }
    return ssl;
}

// the code is based on the way of mod_ssl of Apache.
int SslContext::Ssl_smart_shutdown(SSL *ssl) const
{
    int rc = 0;
    /*
     * Repeat the calls, because SSL_shutdown internally dispatches through a
     * little state machine. Usually only one or two interation should be
     * needed, so we restrict the total number of restrictions in order to
     * avoid process hangs in case the client played bad with the socket
     * connection and OpenSSL cannot recognize it.
     */
    for (int i = 0; i < 4 /* max 2x pending + 2x data = 4 */; i++) {
        rc = SSL_shutdown(ssl);
        if (rc)
            break;
    }
    return rc;
}

void SslContext::destroySsl(SSL& ssl) const
{
    // according to the code of stunnel and mod_ssl, the following is probabaly safer way.
    SSL_set_shutdown(&ssl, SSL_SENT_SHUTDOWN|SSL_RECEIVED_SHUTDOWN);
    Ssl_smart_shutdown(&ssl);
    SSL_free(&ssl);
}

bool SslContext::verifyPeer(const SSL* ssl, const InetAddress& peerAddress) const
{
    char8_t peerBuf[256];
    peerAddress.toString(peerBuf, sizeof(peerBuf));

    X509* cert = SSL_get_peer_certificate(ssl);
    if (cert == nullptr)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[SslContext].verifyPeer: Peer " << peerBuf << " did not send certificate. Peer is not mTLS trusted.");
        return false;
    }

    // NOTE One might think the X509_pubkey_digest function would be a great way to get the digest,
    // but unfortunately that method only computes the digest on the actual bits of the public key,
    // but not the full subjectPublicKeyInfo (aka SPKI) that includes algorithm and parameters.
    // So, we need to do a bit more work to get the DER representation of the full SPKI and then
    // compute the digest ourselves.
    //
    // Envoy supports two types of certificate verification, the full certificate hash or the spki hash,
    // the spki is noted as preferred from Envoy's point of view because a reissued cert with the same key
    // will not require a whitelist update if we only hash on the spki.

    // Convert the SPKI to binary DER format
    EVP_PKEY* pubKey = X509_get_pubkey(cert);
    uint8_t* pubKeyDer = BLAZE_NEW_ARRAY(uint8_t, i2d_PUBKEY(pubKey, nullptr));
    uint8_t* bufPtr = pubKeyDer;
    int32_t pubKeyLen = i2d_PUBKEY(pubKey, &bufPtr);

    // Compute the SHA256 digest of the SPKI
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint32_t hashLength;

    EVP_MD_CTX* ctx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, pubKeyDer, pubKeyLen);
    EVP_DigestFinal_ex(ctx, hash, &hashLength);
    EVP_MD_CTX_destroy(ctx);

    delete [] pubKeyDer;

    // Base 64 encode it - buffer required for base64 encoding is 4*(ceil(x/3)) plus a null terminator,
    // but to stick to purely integer math this can be expressed as 4*((x+2)/3)+1
    char8_t buf[4*((SHA256_DIGEST_LENGTH+2)/3)+1];
    ByteArrayOutputStream baos(reinterpret_cast<uint8_t*>(buf), sizeof(buf));
    uint32_t numWritten = Base64::encode(hash, hashLength, &baos);
    buf[numWritten] = 0;

    X509_free(cert);

    if (mTrustedHashes.find(buf) == mTrustedHashes.end())
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[SslContext].verifyPeer: Peer " << peerBuf << " presented a certificate with hash(" << buf << ") to sslcontext (" << mName << ") that does not match any of the configured trusted hashes. Peer is not trusted.");
        return false;
    }

    BLAZE_INFO_LOG(Log::CONNECTION, "[SslContext].verifyPeer: Peer " << peerBuf << " is trusted based on certificate hash(" << buf << ") on ssl context (" << mName << ").");

    return true; 
}

/*** Private Methods *****************************************************************************/

bool SslContext::initialize(bool peerVerify, const char8_t* key, const char8_t* cert,
        const char8_t* caCertDir, const char8_t* cipherList, SSLContextConfig::SSLProtocolVersion minProtocolVersion)
{
    gSslContextManager->initializeSsl();

    SSL_CTX* ctx = SSL_CTX_new(SSLv23_method()); // The actual minimum protocol version is controlled by the option below and not this call.
    SSL_CTX_set_options(ctx, getSSLProtocolOptions(minProtocolVersion));
    SSL_CTX_set_msg_callback(ctx, SslContext::logSslMessage);
    SSL_CTX_set_info_callback(ctx, SslContext::logSslInfo);

    // For consistency we will always set the session id context, it is only required in some circumstances,
    // such as when verify peer is enabled (otherwise OpenSSL will reject a client attempting to resume a previous session).
    // Note that session resume is handled differently in TLS 1.3, thus this should be revisited when
    // Blaze is updated to OpenSSL 1.1.1 and to support TLS 1.3.
    SSL_CTX_set_session_id_context(ctx, (const unsigned char*) mName.c_str(), mName.length());

    if ((cipherList != nullptr) && (cipherList[0] != '\0'))
    {
        int rc = 1;
        // Restrict the available ciphers
        if (SSLContextConfig::TLSV_1_3 == minProtocolVersion)
        {
            rc = SSL_CTX_set_ciphersuites(ctx, cipherList);
        }
        else 
        {
            rc = SSL_CTX_set_cipher_list(ctx, cipherList);
        }

        if (rc != 1)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initialize: invalid cipher list given.");
            SslContext::logErrorStack("SslContext", this, nullptr);
            SSL_CTX_free(ctx);
            return false;
        }
    }

    // Setup for non-blocking IO
    SSL_CTX_set_mode(ctx,
            SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

    // Enable cached sessions to speed up reconnection times
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_BOTH);
    SSL_CTX_set_timeout(ctx, 60);

    BIO* bio = nullptr;

    if (caCertDir != nullptr)
    {
        mCaCertDir = blaze_strdup(caCertDir);

        Directory::FilenameList fileList;
        Directory::getDirectoryFilenames(caCertDir, ".pem", fileList);
        Directory::FilenameList::iterator itr = fileList.begin();
        Directory::FilenameList::iterator end = fileList.end();
        for(; itr != end; ++itr)
        {
            const char8_t* caData = itr->c_str();
            if (caData == nullptr)
                continue;

            bio = BIO_new_file(caData, "r");
            if (bio == nullptr)
            {
               BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initialize: BIO new file failed for ca '" << caData << "'");
               SslContext::logErrorStack("SslContext", this, nullptr);
               SSL_CTX_free(ctx);
               return false;
            }

            X509* ca = PEM_read_bio_X509(bio, nullptr, 0, nullptr);
            if (ca == nullptr)
            {
                BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initialize: PEM_read_bio_X509 failed "
                        "for ca '" << caData << "'");
                SslContext::logErrorStack("SslContext", this, nullptr);
                BIO_free(bio);
                SSL_CTX_free(ctx);
                return false;
            }

            X509_NAME* name = X509_get_subject_name(ca);

            char8_t nameBuf[512];
            BLAZE_TRACE_LOG(Log::CONNECTION, "[SslContext].initialize: adding CA root " << X509_NAME_oneline(name, nameBuf, sizeof(nameBuf)) 
                <<" from " << caData);

            // Set the CA cert to be used for verification
            X509_STORE* certStore = SSL_CTX_get_cert_store(ctx);
            if (certStore == nullptr || X509_STORE_add_cert(certStore, ca) != 1)
            {
                BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initialize: X509_STORE_add_cert failed.");
                SslContext::logErrorStack("SslContext", this, nullptr);
                X509_free(ca);
                BIO_free(bio);
                SSL_CTX_free(ctx);
                return false;
            }

            X509_free(ca);
            BIO_free(bio);
        }
    }

    if (peerVerify)
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
    else
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

    if (mContext != nullptr)
        SSL_CTX_free(mContext);

    mContext = ctx;

    if (cert != nullptr)
    {
        mCertPath = blaze_strdup(cert);
    }

    if (key != nullptr)
    {
        mKeyPath = blaze_strdup(key);
    }

    return true;
}

bool SslContextManager::loadServerCerts(bool pending)
{
    ContextsByName& contexts = (pending) ? mPendingContexts : mContexts;

    for (auto& entry : contexts)
    {
        if (!entry.second->loadServerCert())
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContextManager].loadServerCerts: Aborting due to failure for context " << entry.first);
            return false;
        }
    }

    return true;
}

bool SslContext::loadServerCert()
{
    if (mCertPath == nullptr || mKeyPath == nullptr)
        return true;

    eastl::string cert;
    eastl::string key;

    if (!loadServerData(mCertPath, "sslCert", "cert", cert))
    {
        return false;
    }

    if (!loadServerData(mKeyPath, "sslKey", "key", key))
    {
        return false;
    }

    return setServerCert(cert, key);
}

void SslContext::refreshServerCert()
{
    if (mCertPath == nullptr || mKeyPath == nullptr)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Not refreshing cert / key for Ssl Context " << mName << " that has no cert / key paths defined");
        return;
    }

    if (mPathType != SSLContextConfig::VAULT_SHARED && mPathType != SSLContextConfig::VAULT_HOSTNAME)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Skipping refresh of Ssl Context " << mName << " whose cert is not in Vault");
        return;
    }

    BLAZE_INFO_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Ssl Context " << mName << " is configured to use Vault, checking for new cert / key");

    eastl::string cert;
    eastl::string key;

    if (!loadServerData(mCertPath, "sslCert", "cert", cert))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Aborting refresh of Ssl Context " << mName << " due to failure to fetch cert from Vault");
        return;
    }

    if (!loadServerData(mKeyPath, "sslKey", "key", key))
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Aborting refresh of Ssl Context " << mName << " due to failure to fetch key from Vault");
        return;
    }

    if (mCertData.compare(cert) == 0 && mKeyData.compare(key) == 0)
    {
        BLAZE_TRACE_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Cert and key for Ssl Context " << mName << " have not changed, nothing to refresh");
        return;
    }

    eastl::string prevCert = mCertData;
    eastl::string prevKey = mKeyData;

    if (setServerCert(cert, key))
    {
        BLAZE_INFO_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Successfully installed new cert and key for Ssl Context " << mName);
        return;
    }

    BLAZE_WARN_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Failed to install refreshed cert for Ssl Context " << mName << ", attempting to restore previous values");

    if (setServerCert(prevCert, prevKey))
    {
        BLAZE_WARN_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Successfully restored previous cert for Ssl Context " << mName);
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].refreshServerCert: Failed to restore previous cert for Ssl Context " << mName);
    }
}

bool SslContext::setServerCert(eastl::string& cert, eastl::string& key)
{
    mCertData = cert;
    mKeyData = key;

    BIO* bio = BIO_new_mem_buf(mCertData.c_str(), -1);
    if (bio != nullptr)
    {
        X509* x509Cert = PEM_read_bio_X509(bio, nullptr, 0, nullptr);
        if (x509Cert == nullptr)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].setServerCert: PEM_read_bio_X509 failed.");
            SslContext::logErrorStack("SslContext", this, nullptr);
            BIO_free(bio);
            return false;
        }
        if (SSL_CTX_use_certificate(mContext, x509Cert) != 1)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].setServerCert: SSL_CTX_use_certificate failed.");
            SslContext::logErrorStack("SslContext", this, nullptr);
            X509_free(x509Cert);
            BIO_free(bio);
            return false;
        }

        X509_NAME* name = X509_get_subject_name(x509Cert);
        X509_NAME_get_text_by_NID(name, NID_commonName, mCommonName, sizeof(mCommonName));
        X509_free(x509Cert);

        BIO_free(bio);
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].setServerCert: BIO new mem buf failed for cert");
        SslContext::logErrorStack("SslContext", this, nullptr);
        return false;
    }

    bio = BIO_new_mem_buf(mKeyData.c_str(), -1);
    if (bio != nullptr)
    {
        EVP_PKEY* evpkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, (void *)"");
        if (evpkey == nullptr)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].setServerCert: PEM_read_bio_PrivateKey failed.");
            SslContext::logErrorStack("SslContext", this, nullptr);
            BIO_free(bio);
            return false;
        }
        if (SSL_CTX_use_PrivateKey(mContext, evpkey) != 1)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].setServerCert: SSL_CTX_use_PrivateKey failed.");
            SslContext::logErrorStack("SslContext", this, nullptr);
            EVP_PKEY_free(evpkey);
            BIO_free(bio);
            return false;
        }

        EVP_PKEY_free(evpkey);
        BIO_free(bio);
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].setServerCert: BIO new mem buf failed for key");
        SslContext::logErrorStack("SslContext", this, nullptr);
        return false;
    }

    BLAZE_INFO_LOG(Log::CONNECTION, "[SslContext].setServerCert: Installed cert for Ssl Context " << mName << " with CN " << mCommonName);

    return true;
}

bool SslContext::loadServerData(const char8_t* path, const char8_t* vaultKey, const char8_t* altVaultKey, eastl::string& data)
{
    if (mPathType == SSLContextConfig::VAULT_SHARED || mPathType == SSLContextConfig::VAULT_HOSTNAME)
    {
        const char8_t* hostname = (mExternalCert) ?
            NameResolver::getExternalAddress().getHostname() : NameResolver::getInternalAddress().getHostname();

        eastl::string vaultPath = path;
        if (mPathType == SSLContextConfig::VAULT_HOSTNAME)
            vaultPath.append_sprintf("/%s", hostname);

        SecretVault::SecretVaultDataMap dataMap;
        if (gVaultManager->readSecret(vaultPath, dataMap) != ERR_OK)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].loadServerData: Failed to fetch ssl server data from Vault path " << vaultPath);
            SslContext::logErrorStack("SslContext", this, nullptr);
            return false;
        }

        // In order to simplify the management of certificates and keys in ESS for SSL Admins,
        // we will standardize on using "sslCert" and "sslKey" as the keys to data in Vault.
        // However, in order to allow a graceful transition period where older branches of Blaze
        // and newer branches point to the same paths in Vault, we will try both the old
        // "cert" and "key" keys as well as the new ones.
        SecretVault::SecretVaultDataMap::const_iterator iter = dataMap.find(vaultKey);
        if (iter != dataMap.end())
        {
            data = iter->second;
        }
        else
        {
            iter = dataMap.find(altVaultKey);
            if (iter != dataMap.end())
            {
                data = iter->second;
            }
            else
            {
                BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].loadServerCert: Failed to locate either "
                    << vaultKey << " or " << altVaultKey << " in Vault data");
                SslContext::logErrorStack("SslContext", this, nullptr);
                return false;
            }
        }
    }
    else
    {
        EA::IO::FileStream fs(path);
        if (!fs.Open())
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].loadServerCert: Failed to open file " << path);
            return false;
        }

        size_t fileSize = EA::IO::File::GetSize(path);
        if (fileSize == 0)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].loadServerCert: File " << path << " is empty");
            return false;
        }

        data.resize(fileSize);
        size_t readSize = fs.Read((void*)data.data(), fileSize);
        fs.Flush();
        fs.Close();
        if (readSize != fileSize)
        {
            BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].loadServerCert: Expected to read " << fileSize << " bytes from file "
                          << path << " but actually read " << readSize << " bytes");
            return false;
        }
    }

    return true;
}

uint32_t SslContext::getSSLProtocolOptions(SSLContextConfig::SSLProtocolVersion minProtocolVersion) const
{
    uint32_t contextOpts = SSL_OP_NO_SSLv2;
    switch (minProtocolVersion)
    {
    case SSLContextConfig::TLSV_1_3:
        contextOpts |= SSL_OP_NO_TLSv1_2;
    case SSLContextConfig::TLSV_1_2:
        contextOpts |= SSL_OP_NO_TLSv1_1;
    case SSLContextConfig::TLSV_1_1:
        contextOpts |= SSL_OP_NO_TLSv1;
    case SSLContextConfig::TLSV_1:
        contextOpts |= SSL_OP_NO_SSLv3;
    case SSLContextConfig::SSLV_2_3:
        break;
    }

    return contextOpts;
}

// static
// This method is intended for callers such as OutboundHttpConnections who want to populate
// the SSL_CTX owned by libcurl.  While libcurl will accept local file paths and do all the
// work for us, when we have the data in memory as fetched from Vault, it is up to us
// to populate the data into libcurl's SSL context
bool SslContext::initSslCtx(SSL_CTX* sslCtx, bool vault, const char8_t* certStr, const char8_t* keyStr, const char8_t* passphrase)
{
    BIO* bio = vault ? BIO_new_mem_buf(certStr, -1) : BIO_new_file(certStr, "r");
    if (bio == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initSslCtx: BIO new file failed for cert");
        SslContext::logErrorStack("SslContext", nullptr, nullptr);
        return false;
    }

    X509* cert = PEM_read_bio_X509(bio, nullptr, 0, nullptr);
    if (cert == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initSslCtx: PEM_read_bio_X509 failed.");
        SslContext::logErrorStack("SslContext", nullptr, nullptr);
        BIO_free(bio);
        return false;
    }

    if (SSL_CTX_use_certificate(sslCtx, cert) != 1)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initSslCtx: SSL_CTX_use_certificate failed.");
        SslContext::logErrorStack("SslContext", nullptr, nullptr);
        X509_free(cert);
        BIO_free(bio);
        return false;
    }

    // Release our reference count to the cert, the SSL CTX now owns it
    X509_free(cert);
    BIO_free(bio);

    bio = vault ? BIO_new_mem_buf(keyStr, -1) : BIO_new_file(keyStr, "r");
    if (bio == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initSslCtx: BIO new file failed for key");
        SslContext::logErrorStack("SslContext", nullptr, nullptr);
        return false;
    }

    // Note that the password can be supplied directly as the fourth param while setting the callback
    // (the third param) to nullptr, but if both are nullptr the typical behaviour is to prompt on the terminal
    // which we definitely do not want to happen, so make sure we pass in empty string if passphrase is null
    EVP_PKEY* evpkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, (void *) (passphrase == nullptr ? "" : passphrase));
    if (evpkey == nullptr)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initSslCtx: PEM_read_bio_PrivateKey failed.");
        SslContext::logErrorStack("SslContext", nullptr, nullptr);
        BIO_free(bio);
        return false;
    }

    if (SSL_CTX_use_PrivateKey(sslCtx, evpkey) != 1)
    {
        BLAZE_ERR_LOG(Log::CONNECTION, "[SslContext].initSslCtx: SSL_CTX_use_PrivateKey failed.");
        SslContext::logErrorStack("SslContext", nullptr, nullptr);
        EVP_PKEY_free(evpkey);
        BIO_free(bio);
        return false;
    }

    // Release our reference count to the private key, the SSL CTX now owns it
    EVP_PKEY_free(evpkey);
    BIO_free(bio);

    return true;
}

// static
void SslContext::logErrorStack(const char8_t* className, void* instance, const char8_t *label)
{
    uint32_t err = 0;
    char8_t errbuf[256] = "";

    err = ERR_get_error();
    if (err == 0)
        return;

    logErrorStack(className, instance, label);
    ERR_error_string_n(err, errbuf, sizeof(errbuf));
    BLAZE_WARN_LOG(Log::CONNECTION, "[" << className << ":" << instance << "].logError: " << (label ? label : "") << (label ? " " : "") << " " << errbuf << " (" << err << ")");
}

void SslContext::logSslMessage(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg)
{
    char8_t* buffer = BLAZE_NEW_ARRAY(char8_t, len+1);
    memcpy(buffer, buf, len);

    BLAZE_SPAM_LOG(Log::CONNECTION, "logSslMessage: SSL message " << (write_p ? "sent" : "received") <<
        " - Protocol version(" << version << ") - ContentType(" << content_type << "). Message: " << buffer << ".");

    delete [] buffer;
}

void SslContext::logSslInfo(const SSL *s, int where, int ret)
{
    const char *str;
    int w = where & ~SSL_ST_MASK;
    if (w & SSL_ST_CONNECT)
        str = "SSL_connect";
    else if (w & SSL_ST_ACCEPT)
        str = "SSL_accept";
    else
        str = "undefined";

    if (where & SSL_CB_LOOP)
    {
        BLAZE_SPAM_LOG(Log::CONNECTION, "logSslInfo: " << str << " " << SSL_state_string_long(s) << ".");
    }
    else if (where & SSL_CB_ALERT)
    {
        str = (where & SSL_CB_READ) ? "read" : "write";
        BLAZE_SPAM_LOG(Log::CONNECTION, "logSslInfo: " << str << ":" << SSL_alert_type_string_long(ret) << ":" << SSL_alert_desc_string_long(ret) << ".");
    }
    else if (where & SSL_CB_EXIT)
    {
        if (ret == 0)
        {
            BLAZE_SPAM_LOG(Log::CONNECTION, "logSslInfo: failed in " << SSL_state_string_long(s));
        }
        else if (ret < 0)
        {
            BLAZE_SPAM_LOG(Log::CONNECTION, "logSslInfo: error in " << SSL_state_string_long(s));

        }
    }
}

// static
bool SslContextManager::initializeContextsUseContextState(const SSLConfig& sslConfig)
{
    if (mSSLContextState == SSLContextState::INITIAL_LOAD_SUCCEEDED)
        return true;
    else if (mSSLContextState == SSLContextState::INITIAL_LOAD_FAILED)
        return false;

    if (initializeContexts(sslConfig))
    {
        mSSLContextState = SSLContextState::INITIAL_LOAD_SUCCEEDED;
        return true;
    }
    else
    {
        mSSLContextState = SSLContextState::INITIAL_LOAD_FAILED;
        return false;
    }
}

// static
bool SslContextManager::startReloadingContexts(const SSLConfig& sslConfig)
{
    if (mSSLContextState == SSLContextState::RELOAD_SUCCEEDED)
        return true;
    else if (mSSLContextState == SSLContextState::RELOAD_FAILED)
        return false;

    if (initializePendingContexts(sslConfig))
    {
        mSSLContextState = SSLContextState::RELOAD_SUCCEEDED;
        return true;
    }
    else
    {
        mSSLContextState = SSLContextState::RELOAD_FAILED;
        return false;
    }
}


// static
void SslContextManager::finishReloadingContexts(bool replaceContexts)
{
    if (mSSLContextState == SSLContextState::RELOAD_SUCCEEDED)
    {
        if (replaceContexts)
            replaceOldContexts();
        else
            mPendingContexts.clear();
    }
    mSSLContextState = SSLContextState::LOAD_NOT_STARTED;
}

// static
bool SslContextManager::verifyContextExists(const char8_t* contextName)
{
    if (mSSLContextState == SSLContextState::RELOAD_SUCCEEDED)
        return pendingContextExists(contextName);
    else
        return get(contextName) != nullptr;
}

void SslContextManager::refreshCerts()
{
    BLAZE_TRACE_LOG(Log::CONNECTION, "[SslContext].refreshCerts: Starting refresh of Ssl Context certificates from Vault");

    // Take copy of the map of contexts on the off chance it mutates (e.g. due to reconfigure) while we block talking to Vault
    ContextsByName contexts(mContexts);

    for (auto& entry : contexts)
    {
        entry.second->refreshServerCert();
    }

    BLAZE_TRACE_LOG(Log::CONNECTION, "[SslContext].refreshCerts: Done refresh of Ssl Context certificates");
}

} // Blaze

