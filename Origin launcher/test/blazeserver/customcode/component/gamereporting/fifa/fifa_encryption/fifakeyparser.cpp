#include "fifakeyparser.h"

#include "EAStdC/EAString.h"

#include "openssl/rsa.h"
#include "openssl/pem.h"
#include "openssl/bio.h"

#include "string.h"

namespace FifaOnline
{
	namespace Security
	{
RSA *FifaKeyParser::ParsePrivateKey(const char *pPrivateKeyData, int32_t iPrivateKeyLen)
{
	RSA *rsa = NULL;
	BIO *keybio = BIO_new_mem_buf((void*)pPrivateKeyData, iPrivateKeyLen);

	if (keybio == NULL)
	{
		printf( "Failed to create key BIO");
		return NULL;
	}

	rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
	BIO_free_all(keybio);
	return rsa;
}


RSA *FifaKeyParser::ParsePublicKey(const char *pPublicKeyData, int32_t iPublicKeyLen)
{
	RSA *rsa = NULL;
	BIO *keybio = BIO_new_mem_buf((void*)pPublicKeyData, iPublicKeyLen);

	if (keybio == NULL)
	{
		printf( "Failed to create key BIO");
		return NULL;
	}
	
	rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
	
	BIO_free_all(keybio);
	return rsa;
}

int32_t FifaKeyParser::Base64Encode(const char *pInput, int32_t iInputLen, char *pOutput)
{
	int result = -1;
	if(pInput != NULL )
	{
		BIO *bio, *b64;
		BUF_MEM *bufferPtr;

		b64 = BIO_new(BIO_f_base64());
		bio = BIO_new(BIO_s_mem());
		bio = BIO_push(b64, bio);

		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
		BIO_write(bio, pInput, iInputLen);
		(void)BIO_flush(bio);
		BIO_get_mem_ptr(bio, &bufferPtr);
		(void)BIO_set_close(bio, BIO_NOCLOSE);
		BIO_free_all(bio);
		
		if(pOutput != NULL)
		{
			memcpy(pOutput, (*bufferPtr).data, (*bufferPtr).length);
		}
		
		result = (int32_t)(*bufferPtr).length; //success
	}

	return result;
}

int32_t FifaKeyParser::Base64Decode(const char *pInput, int32_t iInputLen, char *pOutput)
{
	int result = -1;
	if(pInput != NULL && pOutput != NULL)
	{
		BIO *bio, *b64;

		bio = BIO_new_mem_buf((void*)pInput, iInputLen);
		b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);

		BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
		int32_t length = BIO_read(bio, pOutput, iInputLen);
		result = length;
		BIO_free_all(bio);
	}

	return result; //success
}

	}
}
