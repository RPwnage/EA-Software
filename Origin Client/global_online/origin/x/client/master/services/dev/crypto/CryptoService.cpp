//  Crypto.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "services/crypto/CryptoService.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/rsa.h"
#include "openssl/err.h"

#include "rijndael.h"

using namespace Origin::Services;

static bool encryptSymmetric(QByteArray& dst, QByteArray const& src, QByteArray const& key, EVP_CIPHER const* cipher);
static bool decryptSymmetric(QByteArray& dst, QByteArray const& src, QByteArray const& key, EVP_CIPHER const* cipher);
static QByteArray pad(QByteArray const& from, int requiredLength);
bool decryptPublic( QByteArray& dst, QByteArray const& src, RSA *key);

static const unsigned char iv_data[] =
{ 
    157,  215,   67,   70,  227,  192,  223,   29,    6,  219,  195,   72,  156,  130,  249,  217,
    218,  236,  107,  197,   58,  181,   87,  243,  116,   32,  149,  174,  179,  184,  129,   62,
    161,   25,  163,   97,  238,  112,  232,  183,   93,  213,  182,  104,  235,  233,  164,  241,
    122,   56,  198,  222,  136,   57,   82,   24,  126,   86,  162,   14,  242,   33,  168,  128,
    147,   77,   71,   94,  111,  224,  203,  193,   19,  207,   59,   46,  115,   34,  127,   17,
    169,   81,  178,   30,   91,   21,   11,  212,  153,  142,  251,    2,   63,    0,   85,  245,
    103,   52,  110,  144,  199,  119,  154,  150,  124,  108,   45,   28,   48,   37,  253,   64,
    40,  255,   22,  211,  204,   43,   78,    1,  118,  137,  132,   89,  225,   74,  123,  202,
    23,  234,   50,   18,  177,  201,   44,  216,   66,   69,  175,    5,   98,   76,  247,   31,
    214,  155,  226,  231,  134,   36,   20,  152,  246,   35,   61,  186,   38,  244,  221,  237,
    102,  176,  125,    3,  206,   10,   15,   88,  143,  208,  240,  139,  114,  254,  133,  196,
    12,  191,    9,  239,   79,  146,   65,   95,   53,  172,  145,  173,  140,  229,  252,   96,
    158,    8,   68,   42,  220,   99,  180,  105,   16,   27,  171,   60,  109,  151,  160,  209,
    228,    4,  189,   75,  190,  167,  165,  113,  141,   55,  200,  117,   83,  248,   92,  205,
    39,  166,   73,  131,  138,  170,  121,   90,   13,   54,  106,   51,   84,  194,    7,  185,
    148,  135,  188,  210,  250,  159,   26,  187,   47,  120,  230,   41,   49,  101,   80,  100
};
static const QByteArray sInitializationVector(reinterpret_cast<const char*>(iv_data), sizeof(iv_data));

EVP_CIPHER const* sCipherMap[CryptoService::NUM_CIPHERS][CryptoService::NUM_CIPHER_MODES] =
{
    /* RC2 */       {   EVP_rc2_cbc(),  EVP_rc2_ecb(),  EVP_rc2_cfb(),  EVP_rc2_ofb()   },
    /* BLOWFISH */  {   EVP_bf_cbc(),   EVP_bf_ecb(),   EVP_bf_cfb(),   EVP_bf_ofb()    }
};

class ScopedEvpCipherContext
{
public:

    ScopedEvpCipherContext();
    ~ScopedEvpCipherContext();

    operator EVP_CIPHER_CTX*();

private:

    EVP_CIPHER_CTX ctx;
};

inline ScopedEvpCipherContext::ScopedEvpCipherContext()
{
    EVP_CIPHER_CTX_init(&ctx);
}

inline ScopedEvpCipherContext::~ScopedEvpCipherContext()
{
    EVP_CIPHER_CTX_cleanup(&ctx);
}

inline ScopedEvpCipherContext::operator EVP_CIPHER_CTX*()
{
    return &ctx;
}

class ScopedBio
{
public:

    ScopedBio(BIO_METHOD *method);
    ~ScopedBio();

    operator BIO*();

private:

    BIO* bio;
};

inline ScopedBio::ScopedBio(BIO_METHOD *method)
{
    bio = BIO_new(method);
}

inline ScopedBio::~ScopedBio()
{
    if ( bio )
        BIO_free(bio);
}

inline ScopedBio::operator BIO*()
{
    return bio;
}

class ScopedRSA
{
public:

    ScopedRSA(RSA* key);
    ~ScopedRSA();

    operator bool();
    operator RSA*();

private:
    RSA* m_rsa;
};

inline ScopedRSA::ScopedRSA(RSA* key) : m_rsa(key)
{
}

inline ScopedRSA::~ScopedRSA()
{
    if (m_rsa)
        RSA_free(m_rsa);
    m_rsa = NULL;
}

inline ScopedRSA::operator bool()
{
    return m_rsa != NULL;
}

inline ScopedRSA::operator RSA*()
{
    return m_rsa;
}

class ScopedX509
{
public:

    ScopedX509(X509* key);
    ~ScopedX509();

    operator bool();
    operator X509*();

    RSA* rsa();

private:

    X509* m_x509;
    RSA* m_rsa;
};

inline ScopedX509::ScopedX509(X509* k) : m_x509(k), m_rsa(NULL)
{
}

inline ScopedX509::~ScopedX509()
{
    X509_free(m_x509);
    if ( m_rsa )
        RSA_free(m_rsa);
}

inline ScopedX509::operator bool()
{
    return m_x509 != NULL;
}

inline ScopedX509::operator X509*()
{
    return m_x509;
}

inline RSA* ScopedX509::rsa()
{
    if (!m_rsa)
    {
        EVP_PKEY* pkey = X509_get_pubkey(m_x509);
        if ( pkey )
        {
            m_rsa = EVP_PKEY_get1_RSA(pkey);
        }
    }

    return m_rsa;
}

int CryptoService::keyLengthFor(Cipher cipher, CipherMode mode)
{
    ORIGIN_ASSERT(0 <= cipher && cipher < CryptoService::NUM_CIPHERS);
    ORIGIN_ASSERT(0 <= mode && mode < CryptoService::NUM_CIPHER_MODES);

    EVP_CIPHER const* evpCipher = sCipherMap[cipher][mode];
    ORIGIN_ASSERT_MESSAGE(evpCipher, "Unsupported encryption");

    if (!evpCipher)
        return -1;

    ScopedEvpCipherContext ctx;
    if (!EVP_EncryptInit_ex(ctx, evpCipher, NULL, NULL, NULL))
    {
        ORIGIN_ASSERT(false);
        return -1;
    }

    return EVP_CIPHER_key_length(evpCipher);
}

QByteArray CryptoService::padKeyFor(QByteArray const& key, Cipher cipher, CipherMode mode)
{
    ORIGIN_ASSERT(0 <= cipher && cipher < CryptoService::NUM_CIPHERS);
    ORIGIN_ASSERT(0 <= mode && mode < CryptoService::NUM_CIPHER_MODES);

    EVP_CIPHER const* evpCipher = sCipherMap[cipher][mode];
    ORIGIN_ASSERT_MESSAGE(evpCipher, "Unsupported encryption");

    if (!evpCipher)
        return QByteArray();

    ScopedEvpCipherContext ctx;
    if (!EVP_EncryptInit_ex(ctx, evpCipher, NULL, NULL, NULL))
    {
        ORIGIN_ASSERT(false);
        return QByteArray();
    }

    int requiredKeyLength = EVP_CIPHER_key_length(evpCipher);
    return pad(key, requiredKeyLength);
}


bool CryptoService::encryptSymmetric(QByteArray& dst, QByteArray const& src, QByteArray const& key, Cipher cipher, CipherMode mode)
{
    ORIGIN_ASSERT(0 <= cipher && cipher < CryptoService::NUM_CIPHERS);
    ORIGIN_ASSERT(0 <= mode && mode < CryptoService::NUM_CIPHER_MODES);

    EVP_CIPHER const* evpCipher = sCipherMap[cipher][mode];
    ORIGIN_ASSERT_MESSAGE(evpCipher, "Unsupported encryption");

    if (evpCipher)
    {
        return ::encryptSymmetric(dst, src, key, evpCipher);
    }

    return false;
}

bool CryptoService::decryptSymmetric(QByteArray& dst, QByteArray const& src, QByteArray const& key, Cipher cipher, CipherMode mode)
{
    ORIGIN_ASSERT(0 <= cipher && cipher < CryptoService::NUM_CIPHERS);
    ORIGIN_ASSERT(0 <= mode && mode < CryptoService::NUM_CIPHER_MODES);

    EVP_CIPHER const* evpCipher = sCipherMap[cipher][mode];
    ORIGIN_ASSERT_MESSAGE(evpCipher, "Unsupported encryption");

    if (evpCipher)
    {
        return ::decryptSymmetric(dst, src, key, evpCipher);
    }

    return false;
}

bool CryptoService::decryptPublic( QByteArray& dst, QByteArray const& src, QByteArray certPem)
{
    ScopedBio biomem(BIO_s_mem());
    BIO_puts(biomem, certPem.data());
    ScopedX509 x509(PEM_read_bio_X509(biomem, NULL, NULL, NULL));

    if ( x509 )
        return ::decryptPublic(dst, src, x509.rsa());
    else if ( certPem.isEmpty() )
        ORIGIN_LOG_ERROR << "Could not read certificate";
    else
        ORIGIN_LOG_ERROR << "Could not parse certificate";

    return false;
}


int CryptoService::decryptAES128(QByteArray& dst, QByteArray const& src, QByteArray const& key)
{
    Rijndael rijndael;

    unsigned int length = (unsigned int )src.size();
    if (length == 0) return -8;

    unsigned char * output = new unsigned char [length];
    memset( output, 0, sizeof(char) * length);

    unsigned char* input = (unsigned char*)src.data();    
    unsigned char* key_data = (unsigned char*)key.data();

    rijndael.init(Rijndael::ECB, Rijndael::Decrypt, key_data, Rijndael::Key16Bytes);
	int decrypted_bytes_len = rijndael.padDecrypt(input, length, output);
    
    dst.clear();
    if ( decrypted_bytes_len > 0 )
        dst.append((const char*)output, decrypted_bytes_len);

    delete[] output;

    // decrypted_bytes_len will be negative in case of error
    return decrypted_bytes_len;
}

int CryptoService::encryptAsymmetricRSA(QByteArray& dst, QByteArray const& src, QByteArray const& pubKey)
{
    ScopedBio biomem(BIO_s_mem());
    BIO_puts(biomem, pubKey.data());
    ScopedRSA rsaPubKey(PEM_read_bio_RSA_PUBKEY(biomem, NULL, NULL, NULL));
    if (!rsaPubKey)
        return -1;

    int encSize = RSA_size(rsaPubKey);
    dst.resize(encSize);
    
    // RSA is limited by key size
    if ((src.length()+1) > encSize)
        return -1;

    size_t encrypt_len = RSA_public_encrypt(src.length()+1, (unsigned char*)src.data(), (unsigned char*)dst.data(), rsaPubKey, RSA_PKCS1_PADDING);

    // Did the API return gibberish?
    if (encrypt_len > encSize)
        return -1;

    return encrypt_len;
}

QString CryptoService::sslError()
{
    char buf[1024];
    ERR_error_string(ERR_get_error(), buf);
    return QString(buf);
}

bool encryptSymmetric(QByteArray& dst, QByteArray const& src, QByteArray const& key, EVP_CIPHER const* cipher)
{
    ScopedEvpCipherContext ctx;
    if (!EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL))
    {
        ORIGIN_ASSERT(false);
        return false;
    }

    QByteArray paddedKey = pad(key, EVP_CIPHER_key_length(cipher));
    QByteArray paddedIv = pad(sInitializationVector, EVP_CIPHER_iv_length(cipher));

    if (!EVP_EncryptInit_ex(ctx, NULL, NULL, reinterpret_cast<unsigned char*>(paddedKey.data()), reinterpret_cast<unsigned char*>(paddedIv.data())))
    {
        ORIGIN_ASSERT(false);
        return false;
    }

    const int MAX_BYTES_TO_PROCESS = 1024;
    const int cipherBlockSize = EVP_CIPHER_block_size(cipher);
    QScopedArrayPointer<unsigned char> dstBytes(new unsigned char[MAX_BYTES_TO_PROCESS + cipherBlockSize + 1]);
    int dstBytesLen;
    unsigned char const* srcBytes = reinterpret_cast<unsigned char const*>(src.data());

    for (int bytesRemaining = src.count(); bytesRemaining; )
    {
        int bytesToProcess = std::min(bytesRemaining, MAX_BYTES_TO_PROCESS);
        if(!EVP_EncryptUpdate(ctx, dstBytes.data(), &dstBytesLen, srcBytes, bytesToProcess))
        {
            ORIGIN_ASSERT(false);
            return false;
        }
        
        dst.append(reinterpret_cast<char const*>(dstBytes.data()), dstBytesLen);
        bytesRemaining -= bytesToProcess;
        srcBytes += bytesToProcess;
    }

    // Buffer passed to EVP_EncryptFinal() must be after data just
    // encrypted to avoid overwriting it.
    if(!EVP_EncryptFinal_ex(ctx, dstBytes.data(), &dstBytesLen))
    {
        ORIGIN_ASSERT(false);
        return false;
    }

    dst.append(reinterpret_cast<char const*>(dstBytes.data()), dstBytesLen);

    return true;
}

bool decryptSymmetric(QByteArray& dst, QByteArray const& src, QByteArray const& key, EVP_CIPHER const* cipher)
{
    // Initialization
    ScopedEvpCipherContext ctx;
    if (!EVP_DecryptInit_ex(ctx, cipher, NULL, NULL, NULL))
    {
        ORIGIN_ASSERT(false);
        return false;
    }

    QByteArray paddedKey = pad(key, EVP_CIPHER_key_length(cipher));
    QByteArray paddedIv = pad(sInitializationVector, EVP_CIPHER_iv_length(cipher));

    if (!EVP_DecryptInit_ex(ctx, NULL, NULL, reinterpret_cast<unsigned char*>(paddedKey.data()), reinterpret_cast<unsigned char*>(paddedIv.data())))
    {
        ORIGIN_ASSERT(false);
        return false;
    }

    const int MAX_BYTES_TO_PROCESS = 1024;
    const int cipherBlockSize = EVP_CIPHER_block_size(cipher);
    QScopedArrayPointer<unsigned char> dstBytes(new unsigned char[MAX_BYTES_TO_PROCESS + cipherBlockSize + 1]);
    int dstBytesLen;
    unsigned char const* srcBytes = reinterpret_cast<unsigned char const*>(src.data());

    for (int bytesRemaining = src.count(); bytesRemaining; )
    {
        int numSrcBytesToProcess = std::min(bytesRemaining, MAX_BYTES_TO_PROCESS);
        if (!EVP_DecryptUpdate(ctx, dstBytes.data(), &dstBytesLen, srcBytes, numSrcBytesToProcess))
        {
            ORIGIN_ASSERT(false);
            return false;
        }

        dst.append(reinterpret_cast<char const*>(dstBytes.data()), dstBytesLen);
        bytesRemaining -= numSrcBytesToProcess;
        srcBytes += numSrcBytesToProcess;
    }

    // Stick the final data at the end of the last decrypted data.
    if (!EVP_DecryptFinal_ex(ctx, dstBytes.data(), &dstBytesLen))
    {
        return false;
    }

    dst.append(reinterpret_cast<char const*>(dstBytes.data()), dstBytesLen);

    return true;
}

QByteArray pad(QByteArray const& from, int requiredLength)
{
    ORIGIN_ASSERT(!from.isEmpty());

    QByteArray padded;

    while (requiredLength)
    {
        int numBytesToAppend = std::min(from.count(), requiredLength);
        padded.append(from, numBytesToAppend);
        requiredLength -= numBytesToAppend;
    }

    return padded;
}

bool decryptPublic(QByteArray& dst, QByteArray const& src, RSA *key)
{
    unsigned char const* srcBytes = reinterpret_cast<unsigned char const*>(src.data());
    int dstSize = RSA_size(key) + 1;
    QScopedArrayPointer<unsigned char> dstBytes(new unsigned char[dstSize]);
    int dstBytesLen = RSA_public_decrypt(src.size(), srcBytes, dstBytes.data(), key, RSA_PKCS1_PADDING);
    if ( dstBytesLen < 0 )
        return false;

    if ( dstBytesLen >= dstSize )
    {
        // buffer overflow, crash hard
        ORIGIN_ASSERT(false);
        exit(-1);
    }
    dst.append(reinterpret_cast<char const*>(dstBytes.data()), dstBytesLen);
    return true;
}

