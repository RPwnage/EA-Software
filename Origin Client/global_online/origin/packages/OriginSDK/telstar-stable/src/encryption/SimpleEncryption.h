///////////////////////////////////////////////////////////////////////////////\
//
//  Copyright (C) 2009 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////\
//
//  File: SimpleEncryption.h
//	Simple encryption class that uses hard coded key. other party should use the same key
//
//	Author: Hans van Veenendaal

#ifndef _SIMPLE_DECRYPTION_H_
#define _SIMPLE_DECRYPTION_H_

#include "Rijndael.h"
#include <string>
#include "../impl/OriginSDKMemory.h"

class SimpleEncryption
{
public:
	SimpleEncryption(int key = 0);
	~SimpleEncryption();
	
	//
	//	decode string. source string is zero terminated string containing hex codes
	//
	Origin::AllocString decrypt(const Origin::AllocString& source);

	//
	//	encode string
	//	
	Origin::AllocString encrypt(const Origin::AllocString& source);

    void srand(uint32_t s);
    uint32_t rand();

private:
    uint32_t m_next;

	Rijndael m_rijndael;
	unsigned char m_key[16]; 
};

#endif //_SIMPLE_DECRYPTION_H_