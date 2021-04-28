///////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: LsxRijndaelEncryptor.cpp
//	LSX Encryptor implementation using fixed key and Rijndael encryption method.
//	
//	Author: Steve Lord

#include "LSX/LsxRijndaelEncryptor.h"
#include "services/crypto/SimpleEncryption.h"

#include <QByteArray>
#include <string>
#include <services/log/LogService.h>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {

            //
            //	decode string. 
            //
            QByteArray RijndaelEncryptor::decrypt( const QByteArray& source, int seed )
            { 
                Services::Crypto::SimpleEncryption se(seed);

                std::string decrypted = se.decrypt(source.data());
                QByteArray decryptedArray(decrypted.c_str());

                return decryptedArray;
            };

            //
            //	encode string
            //	
            QByteArray RijndaelEncryptor::encrypt ( const QByteArray& source, int seed)
            {
                Services::Crypto::SimpleEncryption se(seed);

                std::string encrypted = se.encrypt(source.data());
                QByteArray encryptedArray(encrypted.c_str());
                //encryptedArray.append('\0');
                return encryptedArray;
            };
        }
    }
}
