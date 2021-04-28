///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2009 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: SimpleEncryption.h
//	Simple encryption class that uses hardcoded key. other party should use the same key
//
//	Author: Sergey Zavadski

#ifndef _SIMPLE_DECRYPTION_H_
#define _SIMPLE_DECRYPTION_H_

#include <string>

#include "services/plugin/PluginAPI.h"

class Rijndael;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following macro is a simple way to obfuscate static strings in code while still making them easy to read and work with during development.
// A post-build step will need to run the ObfuscateStrings.exe utility to find and obfuscate all of the strings in any built binaries.
// The code will work as expected whether or not the utility is run to obfuscate the strings.
// 
// Tech Brief: https://confluence.ea.com/pages/viewpage.action?pageId=453188356
//
// usage:  
// before:   std::string testString = "This is a string for obfuscation.";
// after:    std::string testString = OBFUSCATE_STRING( "This is a string for obfuscation." );
#define OBFUSCATE_STRING(x) Origin::Services::Crypto::SimpleEncryption::deobfuscate("**OB_START**" x "***OB_END***").c_str()



namespace Origin {
namespace Services {
namespace Crypto {

    class ORIGIN_PLUGIN_API SimpleEncryption
    {
    public:
        SimpleEncryption(int seed = 0);
        ~SimpleEncryption();
        
        //
        //	decode string. source string is zero terminated string containing hex codes
        //
        std::string decrypt(const std::string& source);

        //
        //	encode string
        //	
        std::string encrypt(const std::string& source);

        static std::string deobfuscate(const std::string& source);
    private:
        static inline uint32_t ShiftBits(uint32_t nValue);

        Rijndael* m_rijndael;
        unsigned char m_key[16];
    };
}
}
}

#endif //_SIMPLE_DECRYPTION_H_