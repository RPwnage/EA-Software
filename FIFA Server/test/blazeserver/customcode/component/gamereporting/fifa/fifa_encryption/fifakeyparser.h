// Code From DirtySDK protossl.h
#ifndef FIFA_KEY_PARSER_H
#define FIFA_KEY_PARSER_H

#include "stdio.h"
#include "stdint.h"
#include "openssl/ossl_typ.h"

namespace FifaOnline
{
	namespace Security
	{
		//! a binary large object, such as a modulus or exponent

		class FifaKeyParser
		{
		public:
			FifaKeyParser(){}
			virtual ~FifaKeyParser(){}

			RSA* ParsePrivateKey(const char *pPrivateKeyData, int32_t iPrivateKeyLen);
			RSA* ParsePublicKey(const char *pPublicKeyData, int32_t iPublicKeyLen);

			int32_t Base64Encode(const char *pInput, int32_t iInputLen, char *pOutput);
			int32_t Base64Decode(const char *pInput, int32_t iInputLen, char *pOutput);
		};
	}
}

#endif //FIFA_KEY_PARSER_H