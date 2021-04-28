//  Crypto.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef CRYPTOSERVICE_H
#define CRYPTOSERVICE_H

#include <QByteArray>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        /// \class Crypto
        /// \brief Defines the cryptography service. 
        class ORIGIN_PLUGIN_API CryptoService
        {
        public:

            /// \enum Cipher
            enum Cipher
            {
                RC2,
                BLOWFISH,
                NUM_CIPHERS
            };

            /// \enum CipherMode
            enum CipherMode
            {
                CIPHER_BLOCK_CHAINING,
                ELECTRONIC_CODE_BOOK,
                CIPHER_FEED_BACK,
                OUTPUT_FEED_BACK,
                NUM_CIPHER_MODES
            };

            /// \fn keyLengthFor
            /// \brief Returns the key length required for the given cipher and mode.
            /// \param cipher The encryption algorithm being inquired about.
            /// \param mode The encryption of the encryption algorithm.
            static int keyLengthFor(Cipher cipher, CipherMode mode);

            /// \fn padKeyFor
            /// \brief Pads the given key to a length suitable for the given cipher and mode.
            /// \param key The encryption key to be padded.
            /// \param cipher The encryption algorithm being used.
            /// \param mode The encryption of the encryption algorithm.
            static QByteArray padKeyFor(QByteArray const& key, Cipher cipher, CipherMode mode);

            /// \brief Encrypts the given data using the specified cipher and cipher mode.
            /// \param dst The destination buffer for the encrypted data.
            /// \param src The source buffer containing the unencrypted data.
            /// \param key TBD.
            /// \param cipher The encryption algorithm to use for the encryption.
            /// \param mode The encryption mode to use for the encryption.
            static bool encryptSymmetric(
                QByteArray& dst,
                QByteArray const& src,
                QByteArray const& key,
                Cipher cipher = BLOWFISH,
                CipherMode mode = CIPHER_BLOCK_CHAINING);
            
            /// \brief Decrypts the given data using the specified cipher and cipher mode.
            /// \param dst The destination buffer for the unencrypted data.
            /// \param src The source buffer containing the encrypted data.
            /// \param key TBD.
            /// \param cipher The encryption algorithm to use for the encryption.
            /// \param mode The encryption mode to use for the encryption.
            static bool decryptSymmetric(
                QByteArray& dst,
                QByteArray const& src,
                QByteArray const& key,
                Cipher cipher = BLOWFISH,
                CipherMode mode = CIPHER_BLOCK_CHAINING);

            /// \brief Decrypt the given data using the public key from the certificate
            /// \param dst The destination buffer for the unencrypted data.
            /// \param src The source buffer containing the encrypted data.
            /// \param certPem The X509 certificate in PEM format
            static bool decryptPublic(
                QByteArray& dst,
                QByteArray const& src,
                QByteArray certPem);

            /// \brief Decrypt src using AES with a 128 bit key.
            /// \param dst The destination buffer for the unencrypted data.
            /// \param src The source buffer containing the encrypted data.
            /// \param key Buffer containing the secrety key
            static int decryptAES128(
                QByteArray& dst,
                QByteArray const& src,
                QByteArray const& key);

            static int encryptAsymmetricRSA(
                QByteArray& dst,
                QByteArray const& src,
                QByteArray const& pubKey);
                                   
            /// \brief Return the most recent OpenSSL error string
            static QString sslError();
        };
    }
}

#endif // CRYPTOSERVICE_H
