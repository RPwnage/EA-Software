#include "fifacrypto.h"

#include "EAStdC/EAString.h"

#include "openssl/aes.h"
#include "openssl/rsa.h"
#include "openssl/sha.h"
#include "openssl/rand.h"
#include "openssl/bio.h"

namespace FifaOnline
{
namespace Security
{

FifaCrypto::FifaCrypto()
{
}

FifaCrypto::~FifaCrypto()
{
}

////////////////////////////////////////////////// Encryption

int32_t FifaCrypto::Encrypt(const char* publicKeyData, const char* privateKeySign, const char* dataBuffer, int32_t dataBufferSize, unsigned char** encryptedBuffer)
{
	int result = -1;
	if ( publicKeyData != NULL && privateKeySign != NULL && dataBuffer != NULL && encryptedBuffer != NULL && dataBufferSize > 0)
	{
		///// Generate Session Key	
		unsigned char sessionKey[AES_KEY_SIZE];
		RAND_bytes(sessionKey, AES_KEY_SIZE);

		///// Generate Signature
		unsigned char signature[RSA2048_DATA_SIZE];
		result = GenerateSignatureHash(privateKeySign, dataBuffer, dataBufferSize, signature, RSA2048_DATA_SIZE);

		if(result == ERR_OK)
		{
			///// Encrypt Data + Signature
			// AES Data + Signature Format
			// |------------------|------- total unencrypted data + sig size to encrypt------|
			// | init vector (16) | digital signature (256) | payload size (4) | payload (n) |
			int32_t totalDataSigSize = RSA2048_DATA_SIZE + sizeof(uint32_t) + dataBufferSize;

			// encrypt buffer must be of multiple of 16
			int32_t payloadEncryptSize = totalDataSigSize + (AES_BLOCK_SIZE - (totalDataSigSize % AES_BLOCK_SIZE));
			int32_t totalAesDataSize = payloadEncryptSize + AES_BLOCK_SIZE; // include the init vector size;

			unsigned char *encryptedPayload = new unsigned char[totalAesDataSize];
			unsigned char* data = encryptedPayload + AES_BLOCK_SIZE;

			// Copy signature
			memcpy(data, signature, RSA2048_DATA_SIZE);

			// Copy payload size
			data += RSA2048_DATA_SIZE;
			memcpy(data, &dataBufferSize, sizeof(uint32_t));

			// Copy payload
			data += sizeof(uint32_t);
			memcpy(data, dataBuffer, dataBufferSize);
			encryptedPayload[totalAesDataSize-1] = '\0';

			///// Encrypt Payload and base encode it
			result = EncryptDataAndSignature(sessionKey, encryptedPayload, payloadEncryptSize );
			if(result == ERR_OK)
			{
				int32_t baseEncodeSize = mKeyParser.Base64Encode((const char*)encryptedPayload, totalAesDataSize, NULL); // Get size first
				char *baseEncryptedPayload = new char[baseEncodeSize];
				
				mKeyParser.Base64Encode( (const char*)encryptedPayload, totalAesDataSize, baseEncryptedPayload );
				memset(encryptedPayload, 0, totalAesDataSize);
				delete[] encryptedPayload;
				encryptedPayload = NULL;

				///// Encrypt Session Key
				unsigned char encryptSessionKey[RSA2048_DATA_SIZE];
				result = EncryptSessionKey(publicKeyData, sessionKey, encryptSessionKey);

				if(result == ERR_OK)
				{
					///// Amalgamate Encrypt Session Key, Encrypted Signature + Data
					int32_t totalDataSize = RSA2048_DATA_SIZE + baseEncodeSize;
					printf("%d\n", baseEncodeSize);
					printf("%d\n", totalDataSize);
					
					*encryptedBuffer = new unsigned char[totalDataSize];
					memcpy(*encryptedBuffer, encryptSessionKey, RSA2048_DATA_SIZE);
					memcpy(*encryptedBuffer + RSA2048_DATA_SIZE, baseEncryptedPayload, baseEncodeSize);
					memset(baseEncryptedPayload, 0, baseEncodeSize);
					delete[] baseEncryptedPayload;
					baseEncryptedPayload = NULL;
					result = totalDataSize;
					
				}
			}
		}
	}

	return result;
}

int32_t FifaCrypto::GenerateSignatureHash(const char* privateKey, const char* dataBuffer, int32_t dataBufferSize, unsigned char* encryptedBuffer, int32_t encryptedBufferSize)
{
	int result = ERR_INVALID_ARGS;
	if(privateKey != NULL && dataBuffer != NULL && encryptedBuffer != NULL && dataBufferSize > 0 && encryptedBufferSize > 0)
	{
		RSA * rsa = mKeyParser.ParsePrivateKey(privateKey, (int32_t)strlen(privateKey));

		// Generate Hash
		unsigned char bareSignatureHash[SHA256_DIGEST_LENGTH];
		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		SHA256_Update(&sha256, dataBuffer, dataBufferSize);
		SHA256_Final(bareSignatureHash, &sha256);

		// Encrypt Hash with private key
		result = ERR_LIB;
		if( rsa != NULL)
		{
			int32_t padding = RSA_PKCS1_PADDING;
			result = RSA_private_encrypt(SHA256_HASH_SIZE, (const unsigned char*)bareSignatureHash, encryptedBuffer, rsa, padding);
			result = result > 0 ? ERR_OK : ERR_LIB;
		}
	}
	return result;
}

int32_t FifaCrypto::EncryptSessionKey(const char* publickey, const unsigned char* sessionKey, unsigned char* encryptedBuffer)
{
	int result = ERR_INVALID_ARGS;
	if(publickey != NULL && sessionKey != NULL && encryptedBuffer != NULL)
	{
		int padding = RSA_PKCS1_PADDING;
		RSA * rsa = mKeyParser.ParsePublicKey(publickey, (int32_t)strlen(publickey));
	
		result = ERR_LIB;
		if(rsa != NULL)
		{
			result = RSA_public_encrypt(AES_KEY_SIZE,(const unsigned char*)sessionKey, encryptedBuffer,rsa, padding);
			result = result < 0 ? ERR_LIB : ERR_OK;
		}
	}

	return result;
}

int32_t FifaCrypto::EncryptDataAndSignature(const unsigned char* sessionKey, unsigned char *dataBuffer, int32_t dataBufferSize)
{
	int result = ERR_INVALID_ARGS;

	if( sessionKey != NULL && dataBuffer != NULL && dataBufferSize > 0)
	{
		// Generate init chain and prepend the initialization vector
		unsigned char initChainBuffer[AES_BLOCK_SIZE];
		RAND_bytes(initChainBuffer, AES_BLOCK_SIZE);
		memcpy((void*)dataBuffer, initChainBuffer, AES_BLOCK_SIZE);

		// Encrypt Data + Signature
		const unsigned char* encryptStartPtr = (const unsigned char*)dataBuffer + AES_BLOCK_SIZE;
		unsigned char *outBuffer = new unsigned char[dataBufferSize];
	
		AES_KEY encKey;
		AES_set_encrypt_key((const unsigned char*)sessionKey, AES_KEY_SIZE * 8, &encKey);
		AES_cbc_encrypt(encryptStartPtr, outBuffer, dataBufferSize, &encKey, initChainBuffer, AES_ENCRYPT);
	
		// overwrite contents in databuffer with encrypted value
		memcpy((void*)encryptStartPtr, outBuffer, dataBufferSize);
		memset(outBuffer, 0, dataBufferSize);
		delete[] outBuffer;
		outBuffer = NULL;

		result = ERR_OK;
	}

	return result;
}

////////////////////////////////////////////////// Decryption

int32_t FifaCrypto::Decrypt(const char* publicKeySign, const char* privateKeyData, unsigned char* encryptDataBuffer, int32_t encryptDataBufferSize, FifaDecryptedPayload *payload)
{
	int result = ERR_INVALID_ARGS;
	if ( publicKeySign != NULL && privateKeyData != NULL && encryptDataBuffer != NULL && payload != NULL && payload->mTotalPayload != NULL && encryptDataBufferSize > 0 && payload->mTotalSize > 0)
	{
		// Get Session Key
		unsigned char sessionKey[AES_KEY_SIZE];
		result = DecryptSessionKey(privateKeyData, (const unsigned char*)encryptDataBuffer, sessionKey, AES_KEY_SIZE);
		if(result == ERR_OK)
		{
			// Decrypt payload and signature
			int32_t totalPayloadSize = encryptDataBufferSize - RSA2048_DATA_SIZE;
			result = mKeyParser.Base64Decode((const char*)encryptDataBuffer + RSA2048_DATA_SIZE, totalPayloadSize, (char*)payload->mTotalPayload);
			if( result > 0)
			{
				result = DecryptDataAndSignature(sessionKey, payload, result);
				if( result == ERR_OK )
				{
					// Verify that payload matches the signature
					result = VerifySignature(publicKeySign, payload);
					if(result != ERR_OK)
					{
						payload->Clear();
					}
				}
			}
			else
			{
				result = ERR_LIB;
			}
		}
	}
	return result;
}

int32_t FifaCrypto::DecryptSessionKey(const char* privateKey, const unsigned char* encryptSessionKey, unsigned char* decryptBuffer, int32_t decryptBufferSize)
{
	int result = ERR_INVALID_ARGS;
	if(privateKey != NULL && encryptSessionKey != NULL && decryptBuffer != NULL)
	{
		RSA * rsa = mKeyParser.ParsePrivateKey(privateKey, (int32_t)strlen(privateKey));

		result = ERR_LIB;
		if(rsa != NULL)
		{
			int32_t padding = RSA_PKCS1_PADDING;
			result = RSA_private_decrypt(RSA2048_DATA_SIZE, encryptSessionKey,decryptBuffer, rsa, padding);
			result = result > 0 ? ERR_OK : ERR_LIB;
			
			RSA_free(rsa);
			rsa = NULL;
		}
	}

	return result;
}

int32_t FifaCrypto::DecryptDataAndSignature(const unsigned char* sessionKey, FifaDecryptedPayload* encryptPayload, int32_t actualPayloadSize)
{
	int result = ERR_INVALID_ARGS;
	if( sessionKey != NULL && encryptPayload != NULL && encryptPayload->mTotalPayload != NULL && encryptPayload->mTotalSize > 0 && actualPayloadSize > 0)
	{
		// extract initialization vector from payload
		memcpy(encryptPayload->mInitVector, encryptPayload->mTotalPayload, AES_BLOCK_SIZE);
		unsigned char *actualPayload = encryptPayload->mTotalPayload + AES_BLOCK_SIZE;
	
		// Decrypt Data + Signature
		unsigned char *tmpOutBuffer = new unsigned char[encryptPayload->mTotalSize];
		memset(tmpOutBuffer, 0, encryptPayload->mTotalSize);

		AES_KEY decryptKey;
		AES_set_decrypt_key(sessionKey, AES_KEY_SIZE * 8, &decryptKey);
		AES_cbc_encrypt(actualPayload, tmpOutBuffer, actualPayloadSize, &decryptKey, encryptPayload->mInitVector, AES_DECRYPT );
		memcpy(actualPayload, tmpOutBuffer, encryptPayload->mTotalSize - AES_BLOCK_SIZE);
		memset(tmpOutBuffer, 0, encryptPayload->mTotalSize);
		
		delete[] tmpOutBuffer;
		tmpOutBuffer = NULL;

		// Point to beginning of encrypted signature
		encryptPayload->mSignature = actualPayload;
		encryptPayload->mSignatureSize = RSA2048_DATA_SIZE;

		// Get size of data payload
		uint32_t size = 0;
		memcpy(&size, actualPayload + encryptPayload->mSignatureSize, sizeof(uint32_t));

		// Point to start of data payload;
		encryptPayload->mDataPayload = actualPayload + encryptPayload->mSignatureSize + sizeof(uint32_t);
		encryptPayload->mDataPayload[size] = '\0';
		encryptPayload->mDataPayloadSize = size;

		result = ERR_OK;
	}

	return result;
}

int32_t FifaCrypto::VerifySignature(const char* publicKey, FifaDecryptedPayload* fifaEncryptPayload)
{
	int result = ERR_INVALID_ARGS;
	bool bDoSignatureCheck = false;

	if(!bDoSignatureCheck)
	{
		result = ERR_OK;
	}
	else if(publicKey != NULL && fifaEncryptPayload != NULL && fifaEncryptPayload->mSignature != NULL)
	{
		// Decrypt Signature
		RSA * rsa = mKeyParser.ParsePublicKey(publicKey, (int32_t)strlen(publicKey));
	
		result = ERR_LIB;
		unsigned char decryptedHash[SHA256_HASH_SIZE];
		if (rsa != NULL)
		{
			int32_t padding = RSA_PKCS1_PADDING;
			result = RSA_public_decrypt(fifaEncryptPayload->mSignatureSize, fifaEncryptPayload->mSignature, decryptedHash, rsa, padding);
			result = result > 0 ? ERR_OK : ERR_LIB;

			RSA_free(rsa);
			rsa = NULL;
		}

		// Generate Local Hash
		if(result == ERR_OK)
		{
			unsigned char localSignatureHash[SHA256_HASH_SIZE];
			SHA256_CTX sha256;
			SHA256_Init(&sha256);
			SHA256_Update(&sha256, fifaEncryptPayload->mDataPayload, fifaEncryptPayload->mDataPayloadSize);
			SHA256_Final(localSignatureHash, &sha256);

			result = ERR_OK;
			for(int i = 0; i < SHA256_HASH_SIZE; i++)
			{
				if(decryptedHash[i] != localSignatureHash[i])
				{
					result = ERR_INVALID_HASH;
				}
			}
		}
	}
	return result;
}


// ObfuscatePrivateKey
// Cipher must be 16 bytes
int32_t FifaCrypto::ObfuscateString(const char* cipher, const char* inBuffer, int32_t inBufferSize, char* outBuffer, int32_t outBufferSize)
{
	int result = -1;

	if(cipher != NULL && inBuffer != NULL && outBuffer != NULL && inBufferSize <= outBufferSize )
	{
		if((outBufferSize % AES_BLOCK_SIZE) == 0)
		{
			// first copy data to second buffer
			memcpy(outBuffer, inBuffer, inBufferSize);
			int32_t blocks = outBufferSize / AES_BLOCK_SIZE;

			uint64_t* ptr = (uint64_t*)outBuffer;
			// Obfuscate by cipher.
			for(int i = 0; i < blocks * 2; i++)
			{
				*ptr++ ^= ((uint64_t*)cipher)[i % 2];
			}
			result = 0;
		}
	}
	return result;
}


}
}
