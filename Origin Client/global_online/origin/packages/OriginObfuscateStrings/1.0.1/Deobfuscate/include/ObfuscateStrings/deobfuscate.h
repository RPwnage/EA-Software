///////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2015 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: deobfuscate.h
//	Static String Obfusctation
//
//	Author: Alex Zvenigorodsky

#ifndef _DEOBFUSCATE_H_
#define _DEOBFUSCATE_H_

#include <string>

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
#define OBFUSCATE_STRING(x) Origin::ObfuscateStrings::deobfuscate("**OB_START**" x "***OB_END***").c_str()
#define OBFUSCATE_WSTRING(x) Origin::ObfuscateStrings::deobfuscate(L"**OB_START**" x L"***OB_END***").c_str()

namespace Origin {
    namespace ObfuscateStrings {
        std::string deobfuscate(const std::string& source);
        std::wstring deobfuscate(const std::wstring& source);

    }
};

#endif //_DEOBFUSCATE_H_