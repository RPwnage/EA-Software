/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef _BLAZE_SSLCONTEXT_H
#define _BLAZE_SSLCONTEXT_H

/*** Include files *******************************************************************************/

#include "framework/connection/platformsocket.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "openssl/ssl.h"
#include "EASTL/intrusive_ptr.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class SSLConfig;
class SSLContextConfig;

class SslContext;
typedef eastl::intrusive_ptr<SslContext> SslContextPtr;

class SslContextManager
{
    NON_COPYABLE(SslContextManager);

public:
    SslContextManager();
    ~SslContextManager();

    SslContextPtr get(const char8_t* name = nullptr);
    bool initializeContexts(const SSLConfig& config) { return initializeContextsInternal(config, false); }
    bool createContext(const char8_t* caCertDir, const char8_t* cipherList, const char8_t* contextName = nullptr,
        SSLContextConfig::SSLProtocolVersion minProtocolVersion = SSLContextConfig::TLSV_1_1);
    void initializeSsl();
    void cleanupSsl();

protected:

    friend class Controller;
    friend class Endpoint;

    bool verifyContextExists(const char8_t* contextName);
    bool initializeContextsUseContextState(const SSLConfig& sslConfig);
    bool loadServerCerts(bool pending);
    bool startReloadingContexts(const SSLConfig& sslConfig);
    void finishReloadingContexts(bool replaceContexts);

    void refreshCerts();

private:
    enum SSLContextState
    {
        LOAD_NOT_STARTED,
        INITIAL_LOAD_FAILED,
        INITIAL_LOAD_SUCCEEDED,
        RELOAD_FAILED,
        RELOAD_SUCCEEDED
    };

    bool initializeContextsInternal(const SSLConfig& config, bool initializePending);
    bool initializePendingContexts(const SSLConfig& config) { return initializeContextsInternal(config, true); }
    bool pendingContextExists(const char8_t* name = nullptr);
    void replaceOldContexts();

    typedef eastl::map<eastl::string, SslContextPtr, CaseInsensitiveStringLessThan > ContextsByName;

    ContextsByName mContexts;
    ContextsByName mPendingContexts;
    SSLContextState mSSLContextState;
};

extern EA_THREAD_LOCAL SslContextManager *gSslContextManager;

class SslContext
{
    NON_COPYABLE(SslContext);

public:
    SslContext(const char8_t* name);
    ~SslContext();

    const char8_t* getName() const { return mName.c_str(); }
    const char8_t* getCommonName() const { return mCommonName; }
    SSL* createSsl(SOCKET sock);
    void destroySsl(SSL& ssl) const;

    const char8_t* getCertPath() const { return mCertPath; }
    const char8_t* getKeyPath() const { return mKeyPath; }
    const char8_t* getCertData() const { return mCertData.c_str(); }
    const char8_t* getKeyData() const { return mKeyData.c_str(); }
    const char8_t* getCaCertDir() const { return mCaCertDir; }
    const char8_t* getCaFile() const { return mCaFile; }

    static bool initSslCtx(SSL_CTX* sslCtx, bool vault, const char8_t* cert, const char8_t* key, const char8_t* passphrase);

    static void logErrorStack(const char8_t* className, void* instance, const char8_t *label);
    static void logSslMessage(int write_p, int version, int content_type, const void *buf, size_t len, SSL *ssl, void *arg);
    static void logSslInfo(const SSL *s, int where, int ret);

    bool verifyPeer(const SSL* ssl, const InetAddress& peerAddress) const;

protected:

    friend class Controller;
    friend class Endpoint;
    friend class SslContextManager;
private:

    eastl::string mName;
    SSL_CTX* mContext;
    char8_t mCommonName[256];

    bool initialize(const SSLContextConfig& config);
    bool initialize(const char8_t* caCertDir, const char8_t* cipherList, SSLContextConfig::SSLProtocolVersion minProtocolVersion);
    bool initialize(bool peerVerify, const char8_t* key, const char8_t* cert, const char8_t* caCertDir, const char8_t* cipherList,
        SSLContextConfig::SSLProtocolVersion minProtocolVersion);
    bool loadServerCert();
    bool loadServerData(const char8_t* path, const char8_t* vaultKey, const char8_t* altVaultKey, eastl::string& data);

    bool setServerCert(eastl::string& cert, eastl::string& key);
    void refreshServerCert();

    int Ssl_smart_shutdown(SSL *ssl) const;

    uint32_t getSSLProtocolOptions(SSLContextConfig::SSLProtocolVersion minProtocolVersion) const;

    SSLContextConfig::SSLServerPathType mPathType;
    bool mExternalCert;
    char8_t* mCertPath;
    char8_t* mKeyPath;
    char8_t* mCaCertDir;
    char8_t* mCaFile;
    eastl::string mCertData;
    eastl::string mKeyData;

    typedef eastl::hash_set<eastl::string> StringSet;
    StringSet mTrustedHashes;

    uint32_t mRefCount;

    friend void intrusive_ptr_add_ref(SslContext* ptr);
    friend void intrusive_ptr_release(SslContext* ptr);
};

inline void intrusive_ptr_add_ref(SslContext* ptr)
{
    ptr->mRefCount++;
}

inline void intrusive_ptr_release(SslContext* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

} // Blaze


#endif // _BLAZE_SSLCONTEXT_H

