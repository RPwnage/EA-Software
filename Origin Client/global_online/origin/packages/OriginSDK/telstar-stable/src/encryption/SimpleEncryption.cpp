///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2009 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: SimpleEncryption.cpp
//	Simple encryption class that uses hard coded key. other party should use the same key
//
//	Author: Hans van Veenendaal

#include "stdafx.h"
#include "SimpleEncryption.h"
#include "impl/OriginDebugFunctions.h"
#include "../impl/OriginSDKMemory.h"


void SimpleEncryption::srand(uint32_t s)
{
#ifdef ORIGIN_PC
    m_next = s;
#else
    ::srand(s);
#endif
}

uint32_t SimpleEncryption::rand()
{
#ifdef ORIGIN_PC
    // There were some issues where games couldn't connect in debug mode, and that is likely due to the game using 
    // rand() in a separate thread. The MS implementation is not using thread local storage for the 'next' parameter, causing
    // the rand function to be not deterministic. 
    m_next = m_next * 214013 + 2531011;
    return (uint32_t)(m_next >> 16) & RAND_MAX;
#else
    return ::rand();
#endif
}


SimpleEncryption::SimpleEncryption(int key)
{
    if(key == 0)
    {
        //
        //	hard coded key
        //
	    for ( unsigned int i = 0; i < sizeof( m_key ); i++ )
	    {
		    m_key[i] = (unsigned char)i;
	    }
    }
    else
    {
        srand(7);
        srand(rand() + key);
        for ( unsigned int i = 0; i < sizeof( m_key ); i++ )
        {
            m_key[i] = (unsigned char)(rand()&0xFF);
        }
    }
}

SimpleEncryption::~SimpleEncryption()
{

}

Origin::AllocString SimpleEncryption::decrypt( const Origin::AllocString& source )
{
	unsigned int length = (unsigned int )source.length() / 2;
	if (length == 0) return Origin::AllocString();

	unsigned char * output = TYPE_NEW(unsigned char, length + 1);
	memset( output, 0, sizeof(char) * length + 1);
	
	//	
	//	restore binary representation
	//

	unsigned char* input = TYPE_NEW(unsigned char, length);
	
	for ( unsigned int i = 0; i < length; i++ )
	{
		unsigned int temp;
		sscanf( source.substr( i * 2, 2 ).c_str(), "%x", &temp );
		input[i] = (unsigned char) temp;
	}

	m_rijndael.init( Rijndael::ECB, Rijndael::Decrypt, m_key, Rijndael::Key16Bytes );
	m_rijndael.padDecrypt( input, length, output );

	TYPE_DELETE(input);
	
	Origin::AllocString outString((char *)output);

	TYPE_DELETE(output);

	return outString;
}

Origin::AllocString SimpleEncryption::encrypt( const Origin::AllocString& source )
{
	// Output array will be same as source length, rounded up to next 16 byte multiple.
	const int BLOCK_SIZE = 16;
	unsigned int numBlocks = ((unsigned int) source.length()/BLOCK_SIZE) + 1;
	unsigned int outSize = numBlocks * BLOCK_SIZE;

	unsigned char * output = TYPE_NEW(unsigned char, outSize);
	memset( output, 0, sizeof(char) * outSize );

	m_rijndael.init( Rijndael::ECB, Rijndael::Encrypt, m_key, Rijndael::Key16Bytes );	
	int length = m_rijndael.padEncrypt( ( unsigned char* ) source.c_str(), (unsigned int) source.length(), output );

	// Result is string representation of encrypted value. Add 1 for null terminator.
	unsigned int resultSize = length*2 + 1;
	char * result = TYPE_NEW(char, resultSize);
	memset( result, 0, sizeof(char) * resultSize );
	int offset = 0;

	//
	//	convert to string
	//
	for ( int i = 0; i < length; i++ )
	{
		offset += sprintf( result + offset, "%02x", output[i] );
	}

	TYPE_DELETE(output);

	Origin::AllocString strResult(result);
	TYPE_DELETE(result);

	return strResult;
}
