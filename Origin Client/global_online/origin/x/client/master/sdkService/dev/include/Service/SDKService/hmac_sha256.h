#ifndef __HMAC_SHA256_H__
#define __HMAC_SHA256_H__

/////////////////////////////////////////////////////////////////////////////////////////
/// \brief Calculate the HMAC_SHA256 of the message.
/// 
/// \param text The text to calculate the hash over.
/// \param text_len The length of the text to calculate the hash over.
/// \param key The key used in the hash calculation.
/// \param key_len The length of the key. 
/// \param digest The destination where the digest will be written to. Must be at least SHA256_DIGEST_LENGTH in size.

void hmac_sha256(const unsigned char * text, int text_len, const unsigned char * key, int key_len, unsigned char * digest);

#endif //__HMAC_SHA256_H__