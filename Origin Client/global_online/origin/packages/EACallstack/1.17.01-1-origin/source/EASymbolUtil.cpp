///////////////////////////////////////////////////////////////////////////////
// EASymbolUtil.cpp
//
// Copyright (c) 2003-2005 Electronic Arts Inc.
// Created by Paul Pedriana
// Symbol quote/unquote/remove/collapse/sanitize is based on work by Avery Lee.
///////////////////////////////////////////////////////////////////////////////


#include <EABase/eabase.h>
#include <EACallstack/EASymbolUtil.h>
#include <EASTL/fixed_substring.h>
#include <EASTL/fixed_string.h>
#include <EACallstack/internal/DemangleCodeWarrior.h>
#include <EACallstack/internal/DemangleGCC.h>
#include <assert.h>

#if defined(EA_PLATFORM_MICROSOFT) && !defined(EA_PLATFORM_XENON)
    #ifdef _MSC_VER
        #pragma warning(push, 0)
    #endif

    #include <Windows.h>

    #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP) // DbgHelp and psapi are supported with desktop libraries only.
        #include <DbgHelp.h>
        #ifdef _MSC_VER
            #pragma comment(lib, "dbghelp.lib")
            #pragma comment(lib, "psapi.lib")
        #endif
    #endif

    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#endif


#if defined(EA_PLATFORM_XENON) && EA_XBDM_ENABLED // __unDName is not in xbdm.lib, but we treat it as if it was a debug function anyway.

    /// Undocumented libc function (libc.lib/undname.obj)
    ///
    /// \param unMangledName        Output unmangled name
    /// \param mangledName          Input mangled C++ name
    /// \param size                 Output unMangledName buffer size
    /// \param malloc               Malloc callback
    /// \param free                 Free callback
    /// \param flags                UNDNAME_*
    ///
    extern "C" void __unDName(char* unMangledName, const char* mangledName, int size, 
                              void* (*malloc)(size_t), void (*free)(void*), unsigned short flags);

    #define UNDNAME_32_BIT_DECODE_          0x0800 // Undecorate 32-bit decorated names.
    #define UNDNAME_COMPLETE_               0x0000 // Enable full undecoration.
    #define UNDNAME_NAME_ONLY_              0x1000 // Crack only the name for primary declaration. Returns [scope::]name. Does expand template parameters.
    #define UNDNAME_NO_ACCESS_SPECIFIERS_   0x0080 // Disable expansion of access specifiers for members.
    #define UNDNAME_NO_ALLOCATION_LANGUAGE_ 0x0010 // Disable expansion of the declaration language specifier.
    #define UNDNAME_NO_ALLOCATION_MODEL_    0x0008 // Disable expansion of the declaration model.
    #define UNDNAME_NO_ARGUMENTS_           0x2000 // Do not undecorate function arguments.
    #define UNDNAME_NO_CV_THISTYPE_         0x0040 // Disable expansion of CodeView modifiers on the this type for primary declaration.
    #define UNDNAME_NO_FUNCTION_RETURNS_    0x0004 // Disable expansion of return types for primary declarations.
    #define UNDNAME_NO_LEADING_UNDERSCORES_ 0x0001 // Remove leading underscores from Microsoft keywords.
    #define UNDNAME_NO_MEMBER_TYPE_         0x0200 // Disable expansion of the static or virtual attribute of members.
    #define UNDNAME_NO_MS_KEYWORDS_         0x0002 // Disable expansion of Microsoft keywords.
    #define UNDNAME_NO_MS_THISTYPE_         0x0020 // Disable expansion of Microsoft keywords on the this type for primary declaration.
    #define UNDNAME_NO_RETURN_UDT_MODEL_    0x0400 // Disable expansion of the Microsoft model for user-defined type returns.
    #define UNDNAME_NO_SPECIAL_SYMS_        0x4000 // Do not undecorate special names, such as vtable, vcall, vector, metatype, and so on.
    #define UNDNAME_NO_THISTYPE_            0x0060 // Disable all modifiers on the this type.
    #define UNDNAME_NO_THROW_SIGNATURES_    0x0100 // Disable expansion of throw-signatures for functions and pointers to functions.

#endif


namespace EA
{
    namespace Callstack
    {
        EACALLSTACK_API bool IsCIdentifierChar(int c)
        {
            return  (c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c == '_');
        }


        EACALLSTACK_API eastl::string& QuoteString(eastl::string& s, bool bUnilateral)
        {
            if(bUnilateral || (s.length() < 2) || (s[0] != '"') || (s[s.length() - 1] != '"'))
            {
                s.insert(s.begin(), '"');
                s += '"';
            }
            return s;
        }

        EACALLSTACK_API FixedString8& QuoteString(FixedString8& s, bool bUnilateral)
        {
            if(bUnilateral || (s.length() < 2) || (s[0] != '"') || (s[s.length() - 1] != '"'))
            {
                s.insert(s.begin(), '"');
                s += '"';
            }
            return s;
        }


        EACALLSTACK_API eastl::string& UnquoteString(eastl::string& s)
        {
            if((s.length() >= 2) && (s[0] == '"') && (s[s.length() - 1] == '"'))
            {
                s.erase(s.begin());
                s.erase(s.length() - 1, 1);
            }
            return s;
        }

        EACALLSTACK_API FixedString8& UnquoteString(FixedString8& s)
        {
            if((s.length() >= 2) && (s[0] == '"') && (s[s.length() - 1] == '"'))
            {
                s.erase(s.begin());
                s.erase(s.length() - 1, 1);
            }
            return s;
        }


        EACALLSTACK_API eastl::string& RemoveSubstring(eastl::string& s, const char* pSubstring)
        {
            const eastl_size_t substringLength = (eastl_size_t)strlen(pSubstring);
            eastl_size_t pos = 0;

            while((pos = s.find(pSubstring, pos, substringLength)) != eastl::string::npos)
                s.erase(pos, substringLength);

            return s;
        }

        EACALLSTACK_API FixedString8& RemoveSubstring(FixedString8& s, const char* pSubstring)
        {
            const eastl_size_t substringLength = (eastl_size_t)strlen(pSubstring);
            eastl_size_t pos = 0;

            while((pos = s.find(pSubstring, pos, substringLength)) != FixedString8::npos)
                s.erase(pos, substringLength);

            return s;
        }


        EACALLSTACK_API eastl::string& CollapseStringSpace(eastl::string& s)
        {
            // This could be implemented by a faster algorithm, but speed 
            // isn't a primary concern of ours at this time.
            eastl_size_t pos = 0;

            while((pos = s.find("  ", pos)) != eastl::string::npos)
                s.erase(pos, 1);

            return s;
        }

        EACALLSTACK_API FixedString8& CollapseStringSpace(FixedString8& s)
        {
            // This could be implemented by a faster algorithm, but speed 
            // isn't a primary concern of ours at this time.
            eastl_size_t pos = 0;

            while((pos = s.find("  ", pos)) != FixedString8::npos)
                s.erase(pos, 1);

            return s;
        }


        EACALLSTACK_API eastl::string& RemoveCIdentifier(eastl::string& s, const char* p)
        {
            const eastl::string  match(p);
            eastl_size_t         pos(0), len(0), matchsize(match.size());

            while((pos = s.find(match, pos)) < (len = s.length()))
            {
                // If the match is preceded by C identifiers, then it is not a 
                // standalone keyword but just text after some other name.
                if(pos > 0 && IsCIdentifierChar(s[pos - 1]))
                {
                    pos++;
                    continue;
                }

                // If the match is followed by C identifiers, then it is not a 
                // standalone keyword but just text before some other name.
                if(((pos + matchsize) < len) && IsCIdentifierChar(s[pos + matchsize]))
                {
                    pos++;
                    continue;
                }

                s.erase(pos, matchsize);
            }

            return s;
        }

        EACALLSTACK_API FixedString8& RemoveCIdentifier(FixedString8& s, const char* p)
        {
            const FixedString8  match(p);
            eastl_size_t        pos(0), len(0), matchsize(match.size());

            while((pos = s.find(match, pos)) < (len = s.length()))
            {
                // If the match is preceded by C identifiers, then it is not a 
                // standalone keyword but just text after some other name.
                if(pos > 0 && IsCIdentifierChar(s[pos - 1]))
                {
                    pos++;
                    continue;
                }

                // If the match is followed by C identifiers, then it is not a 
                // standalone keyword but just text before some other name.
                if(((pos + matchsize) < len) && IsCIdentifierChar(s[pos + matchsize]))
                {
                    pos++;
                    continue;
                }

                s.erase(pos, matchsize);
            }

            return s;
        }


        EACALLSTACK_API void CollapseTemplate(eastl::string& s, const char* t)
        {
            const eastl::string match(eastl::string(t) + "<");
            eastl_size_t        pos(0), len(0);

            while((pos = s.find(match, pos)) < (len = s.length()))
            {
                // If the match is preceded by C identifiers, then it is not 
                // the template we are looking for but just text after some other name.
                if((pos > 0) && IsCIdentifierChar(s[pos - 1]))
                {
                    pos++;
                    continue;
                }

                int nestlevel = 1;
                const eastl_size_t start = pos;
                  
                pos += match.size();

                while(pos < len)
                {
                    const char c = s[pos++];

                    if(c == '<')
                        ++nestlevel;
                    else if(c == '>')
                    {
                        if(--nestlevel == 0)
                            break;
                    }
                }

                if(pos < len)
                {
                    s.replace(start, pos - start, "*");
                    pos++;
                }
            }
        }

        EACALLSTACK_API void CollapseTemplate(FixedString8& s, const char* t)
        {
            const FixedString8 match(FixedString8(t) + "<");
            eastl_size_t       pos(0), len(0);

            while((pos = s.find(match, pos)) < (len = s.length()))
            {
                // If the match is preceded by C identifiers, then it is not 
                // the template we are looking for but just text after some other name.
                if((pos > 0) && IsCIdentifierChar(s[pos - 1]))
                {
                    pos++;
                    continue;
                }

                int nestlevel = 1;
                const eastl_size_t start = pos;
                  
                pos += match.size();

                while(pos < len)
                {
                    const char c = s[pos++];

                    if(c == '<')
                        ++nestlevel;
                    else if(c == '>')
                    {
                        if(--nestlevel == 0)
                            break;
                    }
                }

                if(pos < len)
                {
                    s.replace(start, pos - start, "*");
                    pos++;
                }
            }
        }


        EACALLSTACK_API void CollapseTemplateArg(eastl::string& s, const char* t)
        {
            const eastl::string match(eastl::string(t) + "<");
            eastl_size_t        pos(0), len(0);

            while((pos = s.find(match, pos)) < (len = s.length()))
            {
                // If the match is preceded by C identifiers, then it is not 
                // the template we are looking for but just text after some other name.
                if((pos > 0) && IsCIdentifierChar(s[pos - 1]))
                {
                    pos++;
                    continue;
                }

                pos += match.size();

                const eastl_size_t start = pos;
                int nestlevel = 1;

                while(pos < len)
                {
                    const char c = s[pos];

                    if(c == '<')
                        ++nestlevel;
                    else if(c == '>')
                    {
                        if(--nestlevel == 0)
                            break;
                    }

                    ++pos;
                }

                if(pos < len)
                {
                    s.replace(start, pos - start, "*");
                    pos++;
                }
            }
        }

        EACALLSTACK_API void CollapseTemplateArg(FixedString8& s, const char* t)
        {
            const FixedString8 match(FixedString8(t) + "<");
            eastl_size_t       pos(0), len(0);

            while((pos = s.find(match, pos)) < (len = s.length()))
            {
                // If the match is preceded by C identifiers, then it is not 
                // the template we are looking for but just text after some other name.
                if((pos > 0) && IsCIdentifierChar(s[pos - 1]))
                {
                    pos++;
                    continue;
                }

                pos += match.size();

                const eastl_size_t start = pos;
                int nestlevel = 1;

                while(pos < len)
                {
                    const char c = s[pos];

                    if(c == '<')
                        ++nestlevel;
                    else if(c == '>')
                    {
                        if(--nestlevel == 0)
                            break;
                    }

                    ++pos;
                }

                if(pos < len)
                {
                    s.replace(start, pos - start, "*");
                    pos++;
                }
            }
        }


        EACALLSTACK_API void RemoveArguments(eastl::string& s, bool bKeepParentheses)
        {
            // Find the first '(' char. Then find the matching end ')' char.
            // Ignore embedded "()" pairs in-between these.

            eastl_size_t pos = s.find('(');

            if(pos != eastl::string::npos)
            {
                const eastl_size_t end = s.length();
                eastl_size_t i;
                eastl_size_t depth = 0;

                for(i = pos; i < end; i++)
                {
                    if(s[i] == '(') // If this is the first or an embedded '('...
                        ++depth;
                    else if(s[i] == ')')
                    {
                        if(--depth == 0) // If this is the matching ')'...
                        {
                            ++i;
                            break;
                        }
                    }
                }

                if(bKeepParentheses)
                {
                    ++pos;
                    if(depth == 0) // If there was a matching ')'...
                        --i;
                }

                s.erase(pos, i - pos);
            }
        }

        EACALLSTACK_API void RemoveArguments(FixedString8& s, bool bKeepParentheses)
        {
            // Find the first '(' char. Then find the matching end ')' char.
            // Ignore embedded "()" pairs in-between these.

            eastl_size_t pos = s.find('(');

            if(pos != FixedString8::npos)
            {
                const eastl_size_t end = s.length();
                eastl_size_t i;
                eastl_size_t depth = 0;

                for(i = pos; i < end; i++)
                {
                    if(s[i] == '(') // If this is the first or an embedded '('...
                        ++depth;
                    else if(s[i] == ')')
                    {
                        if(--depth == 0) // If this is the matching ')'...
                        {
                            ++i;
                            break;
                        }
                    }
                }

                if(bKeepParentheses)
                {
                    ++pos;
                    if(depth == 0) // If there was a matching ')'...
                        --i;
                }

                s.erase(pos, i - pos);
            }
        }


        EACALLSTACK_API void RemoveScoping(eastl::string& s, int retainCount)
        {
            (void)s;
            (void)retainCount;

            //To do. 
            //const eastl_size_t pos = s.rfind("::", eastl::string::npos);
            //
            //while(retainCount > 0)
            //{
            //}
        }

        EACALLSTACK_API void RemoveScoping(FixedString8& s, int retainCount)
        {
            (void)s;
            (void)retainCount;

            //To do. 
            //const eastl_size_t pos = s.rfind("::", FixedString8::npos);
            //
            //while(retainCount > 0)
            //{
            //}
        }


        EACALLSTACK_API bool IsSymbolUseful(const eastl::string& symbol)
        {
            // To do: Add GCC symbol support to this. GCC doesn't use terms  
            // like __ehhandler, but has its own terms that we want to deal with.

            if(symbol.find("`vftable'") != eastl::string::npos)
                return false;

            if(symbol.compare(0, 11, "__ehhandler") == 0)
                return false;

            return true;
        }

        EACALLSTACK_API bool IsSymbolUseful(const FixedString8& symbol)
        {
            // To do: Add GCC symbol support to this. GCC doesn't use terms  
            // like __ehhandler, but has its own terms that we want to deal with.

            if(symbol.find("`vftable'") != FixedString8::npos)
                return false;

            if(symbol.compare(0, 11, "__ehhandler") == 0)
                return false;

            return true;
        }


        EACALLSTACK_API size_t UnmangleSymbol(const char* pSymbol, char* buffer, size_t bufferCapacity, CompilerType ct)
        {
            buffer[0] = 0;

            if(ct == kCompilerTypeMSVC)
            {
                #if EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
                    // Use the MS DbgHelp UndecorateSymbolName function to do the work for us.
                    DWORD count = UnDecorateSymbolName(pSymbol, buffer, (DWORD)bufferCapacity, UNDNAME_NO_THISTYPE | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_MEMBER_TYPE);
                    if(count == 0)
                        buffer[0] = 0; // Must re-clear the buffer.
                    return (size_t)count;

                #elif defined(EA_PLATFORM_XENON) && EA_XBDM_ENABLED
                    __unDName(buffer, pSymbol, (int)(unsigned)bufferCapacity, malloc, free, UNDNAME_NO_THISTYPE_ | UNDNAME_NO_ACCESS_SPECIFIERS_ | UNDNAME_NO_MEMBER_TYPE_);
                    return strlen(buffer);
                #else
                    // We don't currently have code to manually demangle VC++ symbols, though such code  
                    // could be implemented, as the format is reasonably well documented online.
                #endif
            }
            else if((ct == kCompilerTypeGCC) || 
                    (ct == kCompilerTypeSN)  ||
                    (ct == kCompilerTypeQNX) ||
                    (ct == kCompilerTypeEDG)) 
            {
                #if EACALLSTACK_GCC_DEMANGLE_ENABLED
                    return Demangle::UnmangleSymbolGCC(pSymbol, buffer, bufferCapacity);
                #endif
            }
            else if(ct == kCompilerTypeCodeWarrior)
            {
                #if EACALLSTACK_CODEWARRIOR_DEMANGLE_ENABLED
                    return Demangle::UnmangleSymbolCodeWarrior(pSymbol, buffer, bufferCapacity);
                #endif
            }
            else if(ct == kCompilerTypeGreenHills)
            {
                #if EACALLSTACK_GREENHILLS_DEMANGLE_ENABLED
                    // return DemangeGreenHillsToDo(pSymbol, buffer, bufferCapacity);
                    //
                    // {CAFE_SDK_ROOT}\ghs\multi539\demangle_ghs.so (this is a Windows DLL)
                    // "C:\projects\EAOS\Cafe\packages\cafesdk\2.02\installed\ghs\multi539\demangle_ghs.so"
                    //
                    // Until we write a full demangle implementation write code to load and call demangle_ghs.so,
                    // it turns out we can provide a bonehead version which just reads until the first __ sequence.
                    //     example: AsInt32__Q3_2EA4StdC9uint128_tCFv -> AsInt32()
                #endif
            }

            strncpy(buffer, pSymbol, bufferCapacity);
            buffer[bufferCapacity - 1] = 0;

            return strlen(buffer);
        }


        EACALLSTACK_API size_t UnmangleSymbol(const char* pSymbol, eastl::string& s, CompilerType ct)
        {
            char         buffer[1024];
            const size_t requiredStrlen = UnmangleSymbol(pSymbol, buffer, sizeof(buffer), ct);

            s = buffer; // To do: retry with more capacity if needed.

            return requiredStrlen;
        }


        EACALLSTACK_API size_t UnmangleSymbol(const char* pSymbol, FixedString8& s, CompilerType ct)
        {
            char         buffer[1024];
            const size_t requiredStrlen = UnmangleSymbol(pSymbol, buffer, sizeof(buffer), ct);

            s = buffer; // To do: retry with more capacity if needed.

            return requiredStrlen;
        }


        EACALLSTACK_API eastl::string& SanitizeSymbol(eastl::string& s, CompilerType /*ct*/)
        {
            // To do: Add GCC symbol support to this. GCC doesn't use terms  
            // like __fastcall, but has its own terms that we want to deal with.

            // Remove calling conventions
            RemoveCIdentifier(s, "__cdecl");
            RemoveCIdentifier(s, "__stdcall");
            RemoveCIdentifier(s, "__fastcall");
            RemoveCIdentifier(s, "__thiscall");
            RemoveCIdentifier(s, "class");
            RemoveCIdentifier(s, "struct");
            RemoveCIdentifier(s, "enum");

            // Remove various specific known templates
            CollapseTemplate(s, "allocator");
            CollapseTemplate(s, "_Select1st");
            CollapseTemplate(s, "_Nonconst_traits");

            // Remove namespace prefixes
            RemoveSubstring(s, "_STL::");
            RemoveSubstring(s, "std::");
            RemoveSubstring(s, "eastl::");
            RemoveSubstring(s, "EA::");

            // Clear extraneous string space.
            s.trim();
            CollapseStringSpace(s);

            return s;
        }

        EACALLSTACK_API FixedString8& SanitizeSymbol(FixedString8& s, CompilerType /*ct*/)
        {
            // To do: Add GCC symbol support to this. GCC doesn't use terms  
            // like __fastcall, but has its own terms that we want to deal with.

            // Remove calling conventions
            RemoveCIdentifier(s, "__cdecl");
            RemoveCIdentifier(s, "__stdcall");
            RemoveCIdentifier(s, "__fastcall");
            RemoveCIdentifier(s, "__thiscall");
            RemoveCIdentifier(s, "class");
            RemoveCIdentifier(s, "struct");
            RemoveCIdentifier(s, "enum");

            // Remove various specific known templates
            CollapseTemplate(s, "allocator");
            CollapseTemplate(s, "_Select1st");
            CollapseTemplate(s, "_Nonconst_traits");

            // Remove namespace prefixes
            RemoveSubstring(s, "_STL::");
            RemoveSubstring(s, "std::");
            RemoveSubstring(s, "eastl::");
            RemoveSubstring(s, "EA::");

            // Clear extraneous string space.
            s.trim();
            CollapseStringSpace(s);

            return s;
        }

    } // namespace Callstack

} // namespace EA





