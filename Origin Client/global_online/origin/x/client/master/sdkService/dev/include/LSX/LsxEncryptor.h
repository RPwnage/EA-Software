///////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2011 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: LsxEncryptor.h
//	LSX encryptor base class. Defaults to pass through, ie no encryption/decryption of string.
//	
//	Author: Steve Lord


#ifndef _LSX_ENCRYPTOR_H_
#define _LSX_ENCRYPTOR_H_

#include <QByteArray>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
	        class Encryptor
	        {
	        public:
		        //
		        //	decode string. 
		        //
		        virtual QByteArray decrypt( const QByteArray& source, int seed = 0 ) { return source; } ;

		        //
		        //	encode string
		        //	
		        virtual QByteArray encrypt ( const QByteArray& source, int seed = 0  ) { return source; } ;
	        };
        }
    }
}

#endif //_LSX_ENCRYPTOR_H_
