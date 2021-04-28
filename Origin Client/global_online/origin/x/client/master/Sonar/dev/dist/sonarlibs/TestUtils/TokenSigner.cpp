#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/objects.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <SonarCommon/Common.h>
#include <string.h>

#include "TokenSigner.h"

namespace sonar
{

struct TokenSigner::Private
{
	SonarVector<char> privateKey;
	RSA *rsaPrivKey;
};

TokenSigner::TokenSigner()
{
	sonar::String path;

#ifdef _WIN32
	path += getenv("HOMEDRIVE");
	path += getenv("HOMEPATH");
	path += "\\";
#else
	path += getenv("HOME");
	path += "/";
#endif

	path += "sonarmaster.prv";

	char privateKey[4096];

	FILE *file = fopen(path.c_str(), "rb");

	if (file == NULL)
	{
		throw KeyNotFoundException();
	}

	size_t len = fread(privateKey, 1, sizeof(privateKey), file);
	fclose(file);

	if (len == 0)
	{
		throw KeyNotFoundException();
	}

	m_private = new Private();
	m_private->rsaPrivKey = NULL;
	m_private->privateKey.resize(len);
	memcpy(&m_private->privateKey[0], privateKey, len);
	BIO *bioPk = BIO_new_mem_buf( (void *) privateKey, (int) len);
	PKCS8_PRIV_KEY_INFO *pki = NULL;
	pki = d2i_PKCS8_PRIV_KEY_INFO_bio(bioPk, NULL);
	EVP_PKEY *pkey = EVP_PKCS82PKEY (pki);
	m_private->rsaPrivKey = EVP_PKEY_get1_RSA(pkey);
	PKCS8_PRIV_KEY_INFO_free(pki);
	EVP_PKEY_free(pkey);
	BIO_free(bioPk);
}

CString TokenSigner::base64Encode(const void *data, size_t cbData)
{
	String ret;
	BIO *bmem, *b64;
	BUF_MEM *bptr;

	b64 = BIO_new(BIO_f_base64());
	BIO_set_flags(b64,BIO_FLAGS_BASE64_NO_NL);
	bmem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, bmem);
	BIO_write(b64, data, cbData);
	BIO_flush(b64);
	BIO_get_mem_ptr(b64, &bptr);
	ret = String(bptr->data, bptr->length);
	BIO_free_all(b64);
	return ret;
}


TokenSigner::~TokenSigner(void)
{
	if (m_private->rsaPrivKey)
		RSA_free(m_private->rsaPrivKey);
	delete m_private;
}

CString TokenSigner::sign(const Token &token)
{
	if (m_private->rsaPrivKey == NULL)
	{
		return token.toString();
	}

	CString msg = token.toString();

	SHA_CTX sha_ctx = { 0 };
	unsigned char digest[SHA_DIGEST_LENGTH];

	int rc;

	rc = SHA1_Init(&sha_ctx);
	rc = SHA1_Update(&sha_ctx, msg.c_str(), msg.size());
	rc = SHA1_Final(digest, &sha_ctx);

	unsigned char sign[256] = { 0 };
	unsigned int signLen = 20;
		
	if (!RSA_sign(NID_sha1, digest, SHA_DIGEST_LENGTH, sign, &signLen, m_private->rsaPrivKey))
	{
		return "";
	}

	sonar::String ss;
	ss += msg;
	ss += base64Encode(sign, signLen);	

	return ss;
}

}
