///////////////////////////////////////////////////////////////////////////////
//
//  Copyright � 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: LsxRijndaelEncryptor.h
//	LSX encryptor derived class. Encryption based on fixed key and Rijndael encryption method.
//	
//	Author: Steve Lord


#ifndef _LSX_RIJNDAELENCRYPTOR_H_
#define _LSX_RIJNDAELENCRYPTOR_H_

#include "LsxEncryptor.h"
#include <QByteArray>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class RijndaelEncryptor : public Encryptor
            {
            public:
                //
                //	decode string. 
                //
                virtual QByteArray decrypt( const QByteArray& source, int seed = 0 );

                //
                //	encode string
                //	
                virtual QByteArray encrypt ( const QByteArray& source, int seed = 0 );
            };
        }
    }
}

#endif //_LSX_RIJNDAELENCRYPTOR_H_
