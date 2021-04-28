//******************************************************************************************
// FifaCrypto
// Encrypts and decrypts data using hybrid encryption.
// This library uses the following algorithms/encoding standards:
// AES, RSA, Zip, SHA-2 Base64Encode
// (c) 2015 Electronic Arts Inc.
//******************************************************************************************
#ifndef FIFA_CRYPTO_H
#define FIFA_CRYPTO_H
#include "stdint.h"
#include "stdio.h"
#include "string.h"

#include "fifakeyparser.h"

namespace FifaOnline
{
	namespace Security
	{

		enum FifaCryptoConstants
		{
			ERR_OK = 0,
			ERR_INVALID_ARGS = -1,
			ERR_LIB= -2,
			ERR_INSUFFICIENT_SIZE = -3,
			ERR_INVALID_HASH = -4,
			

			AES_KEY_SIZE = 32,
			AES_BLOCK_SIZE = 16,

			SHA256_HASH_SIZE = 32,
			RSA2048_DATA_SIZE = 256		
		};

		struct FifaDecryptedPayload
		{
			FifaDecryptedPayload() :
				mDataPayload(NULL)
				,mDataPayloadSize(0)
				,mSignature(NULL)
				,mSignatureSize(0)
				,mTotalPayload(NULL)
				,mTotalSize(0)
			{
			}

			FifaDecryptedPayload(int32_t size) :
				mDataPayload(NULL)
				,mDataPayloadSize(0)
				,mSignature(NULL)
				,mSignatureSize(0)
				,mTotalPayload(NULL)
				,mTotalSize(size - RSA2048_DATA_SIZE)
			{
				mTotalPayload = new unsigned char[mTotalSize];
				printf("%d\n", mTotalSize);
			}

			~FifaDecryptedPayload()
			{
				Clear();
			}

			void Clear()
			{
				if(mTotalPayload != NULL)
				{
					memset(mTotalPayload, 0, mTotalSize);
					delete[] mTotalPayload;
					mTotalPayload = NULL;
				}

				mTotalSize = 0;
				mSignatureSize = 0;
				mDataPayloadSize = 0;
				
				mSignature = NULL;
				mDataPayload = NULL;
			}

			// Pointer markers to mTotalPayload
			unsigned char mInitVector[AES_BLOCK_SIZE];
			
			unsigned char *mDataPayload;
			int32_t  mDataPayloadSize;

			unsigned char *mSignature;
			int32_t  mSignatureSize;
			
			// Pointer to where actual payload is held
			unsigned char *mTotalPayload;
			int32_t  mTotalSize;
		};

		class FifaCrypto
		{
		public:
			FifaCrypto();
			virtual ~FifaCrypto();

			//!  Encrypts the data, loosely based on the OpenPGP format.  
			//   Inputs
			//		- publicKeyData:		RSA public key string from external party's public PEM file to be used to encrypt the data 
			//		- privateKeySign:		RSA private key string from local private key PEM file used to sign the data
			//		- dataBuffer:			data to encrypt
			//		- dataBufferSize:		amount of data to encrypt in bytes
			//		- encryptedBuffer:		the buffer where the resulting encrypted data will be written to
			//   Return
			//		- Size of bytes encrypted.  Will return a value <= 0 if failed.
			int32_t Encrypt(const char* publicKeyData, const char* privateKeySign, const char* dataBuffer, int32_t dataBufferSize, unsigned char** encryptedBuffer);
		
			//!  Decrypts data, encrypted by the Encrypt method. Loosely based on the OpenPGP format.  
			//   Inputs
			//		- publicKeySign:			RSA public key string from external party's public PEM file to be used to decrypt the digital signature
			//		- privateKeyData:			RSA private key string from local private key PEM file used to decrypt the data
			//		- encryptDataBuffer:		data to decrypt
			//		- encryptDataBufferSize:	size of encryptDataBuffer
			//		- payload:					Resulting output.
			//   Return
			//		- returns 0 on success.  Any other result is a failure.
			int32_t Decrypt(const char* publicKeySign, const char* privateKeyData, unsigned char* encryptDataBuffer, int32_t encryptDataBufferSize, FifaDecryptedPayload *payload);


			//!   Obfuscates and Deobfuscates a string 
			//	  Outbuffer must be a multiple of 16
			int32_t ObfuscateString(const char* cipher, const char* inBuffer, int32_t inBufferSize, char* outBuffer, int32_t outBufferSize);
			
		private:
			// Data Encryption
			int32_t EncryptDataAndSignature(const unsigned char* sessionKey, unsigned char* dataBuffer, int32_t dataBufferSize);
			int32_t EncryptSessionKey(const char* publickey, const unsigned char* sessionKey, unsigned char* encryptedBuffer);
			int32_t GenerateSignatureHash(const char* privateKey, const char* dataBuffer, int32_t dataBufferSize, unsigned char* encryptedBuffer, int32_t encryptedBufferSize);
			
			// Data Decryption
			int32_t VerifySignature(const char* publicKey, FifaDecryptedPayload* fifaEncryptPayload);
			int32_t DecryptSessionKey(const char* privateKey, const unsigned char* encryptSessionKey, unsigned char* decryptBuffer, int32_t decryptBufferSize);
			int32_t DecryptDataAndSignature(const unsigned char* sessionKey, FifaDecryptedPayload* encryptPayload, int32_t actualPayloadSize);

			FifaKeyParser mKeyParser;

		};
	}
}

#endif //FIFA_CRYPTO_H
