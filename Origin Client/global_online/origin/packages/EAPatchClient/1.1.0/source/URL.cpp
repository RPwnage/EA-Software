///////////////////////////////////////////////////////////////////////////////
// URL.cpp
//
// Copyright (c) 2004 Electronic Arts Inc.
// Written and maintained by Paul Pedriana
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// While the syntax for the rest of the URL may vary depending on the
// particular scheme selected, URL schemes that involve the direct use
// of an IP-based protocol to a specified host on the Internet use a
// common syntax for the scheme-specific data:
//
//    <scheme:>//<user>:<password>@<host>:<port>/<url-path>
//
// Some or all of the parts "<user>:<password>@", ":<password>",
// ":<port>", and "/<url-path>" may be excluded. Additionally, some 
// implementations try to be smart and guess the scheme if it isn't 
// present.
//
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/URL.h>
#include <EAPatchClient/Allocator.h>
#include <EAPatchClient/Debug.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EACType.h>


#if defined(_MSC_VER)
    #pragma warning(disable: 4267) // conversion from 'size_t' to 'eastl::size_type', possible loss of data
    #pragma warning(disable: 4390) // empty controlled statement found; is this the intent?
#endif


namespace EA
{

namespace Patch
{



const bool kStrictConformanceDefault  = true;
const int  kMaxComponentLengthDefault = 128;

#if !defined(URL_MIN)
    #define URL_MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


///////////////////////////////////////////////////////////////////////////////
// IsValidASCII
//
static inline bool IsValidASCII(const char8_t c)
{
    return ((uint8_t)c <= 127);
}


///////////////////////////////////////////////////////////////////////////////
// URL
//
URL::URL()
  : mbStrictConformance(kStrictConformanceDefault),
    mnMaxComponentLength(kMaxComponentLengthDefault),
    mbParsed(false),
    mURLType(kURLTypeUndefined)
{
    // Empty
}


///////////////////////////////////////////////////////////////////////////////
// URL
//
URL::URL(const char8_t* pURL)
  : mbStrictConformance(kStrictConformanceDefault),
    mnMaxComponentLength(kMaxComponentLengthDefault),
    mbParsed(false),
    mURLType(kURLTypeUndefined),
    msURL()
{
    if(pURL)
        msURL.assign(pURL);

    #if EASTL_NAME_ENABLED
        for(size_t i = 0; i < kComponentCount; ++i)
            msComponent[i].get_allocator().set_name(EAPATCHCLIENT_ALLOC_PREFIX "URL");
    #endif
}



///////////////////////////////////////////////////////////////////////////////
// URL
//
URL::URL(const URL& url)
{
    operator=(url);
}


///////////////////////////////////////////////////////////////////////////////
// operator=
//
URL& URL::operator=(const URL& url)
{
    if(&url != this)
    {
        mbStrictConformance  = url.mbStrictConformance;
        mnMaxComponentLength = url.mnMaxComponentLength;
        mbParsed             = url.mbParsed;
        mURLType             = url.mURLType;
        msURL                = url.msURL;
        for(size_t i = 0; i < kComponentCount; i++)
            msComponent[i] = url.msComponent[i];
    }
    return *this;
}


///////////////////////////////////////////////////////////////////////////////
// GetOption
//
int URL::GetOption(Options option) const
{
    switch(option)
    {
        case kOptionStrictConformance:
            return mbStrictConformance ? 1 : 0;

        case kOptionMaxComponentLength:
            return mnMaxComponentLength;
        case kOptionNone:
            break;
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// SetOption
//
void URL::SetOption(Options option, int value)
{
    switch(option)
    {
        case kOptionStrictConformance:
            mbStrictConformance = (value != 0);
            break;

        case kOptionMaxComponentLength:
            mnMaxComponentLength = value;
            break;
        case kOptionNone:
            break;
    }
}


///////////////////////////////////////////////////////////////////////////////
// IsURLRelative
//
// Returns true if the given URL is relative, else absolute.
//
bool URL::IsURLRelative() const
{
    if(!mbParsed)
    {
        String sURLCopy = msURL;
        ParseURLConst(sURLCopy);
    }
    return (mURLType == kURLTypeRelative);
}


///////////////////////////////////////////////////////////////////////////////
// IsURLRelative
//
// Returns true if the given URL is relative, else absolute.
//
bool URL::IsURLRelative(const char8_t* pURL)
{
    const URL url(pURL);
    return url.IsURLRelative();
}


///////////////////////////////////////////////////////////////////////////////
// URLType
//
URL::URLType URL::GetURLType() const
{
    if(!mbParsed)
    {
        String sURLCopy(msURL);
        ParseURLConst(sURLCopy);
    }
    return mURLType;
}


///////////////////////////////////////////////////////////////////////////////
// SetURLType
//
void URL::SetURLType(URLType urlType){
    mURLType = urlType;
}


///////////////////////////////////////////////////////////////////////////////
// FullURL
//
const String& URL::GetFullURL() const
{
    // We don't need to build the URL, as the URL string is always built.
    // If this string is empty then it means the user set it to be empty.
    return msURL;
}


///////////////////////////////////////////////////////////////////////////////
// SetFullURL
//
void URL::SetFullURL(const char8_t* pFullURL, bool bShouldReparse)
{
    if(bShouldReparse)
        mbParsed = false; // Else leave 'mbParsed' as is.
    if(pFullURL)
        msURL = pFullURL;
    else
        msURL.clear();
}


///////////////////////////////////////////////////////////////////////////////
// GetComponent
//
const String& URL::GetComponent(Component component) const
{
    if(!mbParsed)
    {
        String sURLCopy(msURL);
        ParseURLConst(sURLCopy);
    }
    return msComponent[component];
}


///////////////////////////////////////////////////////////////////////////////
// SetComponent
//
void URL::SetComponent(Component component, const char8_t* pComponent)
{
    if(pComponent)
        msComponent[component] = pComponent;
    else
        msComponent[component].clear();
}


///////////////////////////////////////////////////////////////////////////////
// GetPort
//
uint32_t URL::GetPort() const
{
    return EA::StdC::AtoU32(msComponent[kComponentPort].c_str());
}


///////////////////////////////////////////////////////////////////////////////
// SetPort
//
void URL::SetPort(uint32_t nPort)
{
    char8_t port[20];
    EA::StdC::Sprintf(port, "%u", (unsigned)nPort);
    msComponent[kComponentPort] = port;
}


///////////////////////////////////////////////////////////////////////////////
// ChangeURL
//
// Please see RFC 1808 "Relative Uniform Resource Locators" for a mostly 
// complete description of relative URLs. Basically, a relative URL is 
// a URL where you use a path with things like "/.." to specify the parent
// directory of wherever the current base directory is. This function doesn't
// try to know how the base URL was made or how it came about; it simply 
// uses it.
//
// As an example, taken from the RFC itself, given a well-defined base URL of:
//     "http://a/b/c/d;p?q#f"
//
// the following relative URLs to this base would be resolved as follows:
//     "g:h"             => "g:h"                       // The scheme has changed, so leave as is.
//     "g"               => "http://a/b/c/g"            // Same as "./g"
//     "./g"             => "http://a/b/c/g"            //
//     "g/"              => "http://a/b/c/g/"           // Same as "./g/"
//     "/g"              => "http://a/g"                // 'a' is the server. thus a path of '/g' simply means "scheme://<server>/g"
//     "//g"             => "http://g"                  // I think this is wrong. See comments below.
//     "?y"              => "http://a/b/c/d;p?y"        // All we do is simply paste a different searchdata (a.k.a. querydata) in the old one's place.
//     "g?y"             => "http://a/b/c/g?y"          //
//     "g?y/./x"         => "http://a/b/c/g?y/./x"      //
//     "#s"              => "http://a/b/c/d;p?q#s"      // All we do is simply paste a different fragment in the old one's place.
//     "g#s"             => "http://a/b/c/g#s"          // Here we replace both the file and the fragment.
//     "g#s/./x"         => "http://a/b/c/g#s/./x"      //
//     "g?y#s"           => "http://a/b/c/g?y#s"        //
//     ";x"              => "http://a/b/c/d;x"          // All we do here is simply paste in a different params data in the old one's place.
//     "g;x"             => "http://a/b/c/g;x"          //
//     "g;x?y#s"         => "http://a/b/c/g;x?y#s"      //
//     "."               => "http://a/b/c/"             // Simply the current directory
//     "./"              => "http://a/b/c/"             //
//     ".."              => "http://a/b/"               // Simply the parent directory
//     "../"             => "http://a/b/"               //
//     "../g"            => "http://a/b/g"              //
//     "../.."           => "http://a/"                 // Note that this references a directory, not a file.
//     "../../"          => "http://a/"                 //
//     "../../g"         => "http://a/g"                // 'c' is current base dir, b is parent 1, a is parent 2.
//     "../../b/../g"    => "http://a/g"                //
//     "/./g"            => "http://a/./g"              // This is malformed, but is the 'correct' answer.
//     "/../g"           => "http://a/../g"             // This is malformed, but is the 'correct' answer.
//     "./../g"          => "http://a/b/g"              //
//     "./g/."           => "http://a/b/c/g/"           //
//     "g/./h"           => "http://a/b/c/g/h"          //
//     "g/../h"          => "http://a/b/c/h"            //
//
bool URL::ChangeURL(const char8_t* pNewURL, const char8_t* pBaseURL)
{
    using namespace eastl;

    // First clear out all settings 
    mbParsed = false;
    mURLType = kURLTypeUndefined;

    msURL.clear();
    msComponent[kComponentScheme].clear();
    msComponent[kComponentPassword].clear();
    msComponent[kComponentServer].clear();
    msComponent[kComponentPort].clear();
    msComponent[kComponentAbsolutePath].clear();

    if(pBaseURL && pBaseURL[0])
    {
        if(!pNewURL || !pNewURL[0])  
             msURL = pBaseURL;                                                  // RFC 1808, section 4, step 2, part 'a' says to do this.
        else
        {
            String sTempNewURL(pNewURL);                                       // Make a URL out of the new URL string.
            URL     relativeURL(pNewURL);                                       // First we parse the relative URL
            relativeURL.ParseURL(sTempNewURL);                                  // RFC 1808, section 4, step 2 says to do this.
            relativeURL.SetFullURL(NULL, false);                                // Clear out the full URL, since it is going to change anyway.

            String sTempBaseURL(pBaseURL);                                     // Make a URL out of the base URL string.
            URL     baseURL(pBaseURL);
            baseURL.ParseURL(sTempBaseURL);                                     // RFC 1808, section 4, step 2 says to do this.

            if(relativeURL.GetComponent(kComponentScheme).length())             // If the embedded (relative) URL starts with a scheme name, it is an absolute URL.
                msURL = pNewURL;                                                // RFC 1808, section 4, step 2, part 'b' says so.
            else //Else now it is probably a relative URL
            {
                const String& sScheme(baseURL.GetComponent(kComponentScheme));

                relativeURL.SetComponent(kComponentScheme, sScheme.c_str());            // RFC 1808, section 4, step 2, part 'c' says so.

                if(relativeURL.GetComponent(kComponentServer).length() == 0)            // RFC 1808, section 4, step 3.
                {                                                                       // Basically, the relative URL inherits any portion of 
                    const String& sServer(baseURL.GetComponent(kComponentServer));     // the "net location" that it didn't have. Usually, this
                    relativeURL.SetComponent(kComponentServer, sServer.c_str());        // is just the server name (e.g. "www.maxis.com").
                }

                if(relativeURL.GetComponent(kComponentUsername).length() == 0)
                {
                    const String& sUsername(baseURL.GetComponent(kComponentUsername));
                    relativeURL.SetComponent(kComponentUsername, sUsername.c_str());
                }

                if(relativeURL.GetComponent(kComponentPassword).length() == 0)
                {
                    const String& sPassword(baseURL.GetComponent(kComponentPassword));
                    relativeURL.SetComponent(kComponentPassword, sPassword.c_str());
                }

                if(relativeURL.GetPort() == 0)
                    relativeURL.SetPort(baseURL.GetPort());

                const String& sAbsolutePath = relativeURL.GetComponent(kComponentAbsolutePath);
                if(sAbsolutePath.length() >= 1) // RFC 1808, section 4, step 4.
                {
                    if(sAbsolutePath[0] != '/') // Beginning with '/' means that it is an absolute path (not a relative path).
                    {
                        // Now it *must* be a relative URL.

                        // We remove anything after the base URL's last '/' char, as per RFC 1808, section 4, step 6.
                        String sBaseURLDirectory(baseURL.GetComponent(kComponentAbsolutePath));
                        const size_t nLastForwardSeparatorPosition(sBaseURLDirectory.rfind('/'));

                        if(nLastForwardSeparatorPosition < sBaseURLDirectory.length()) //Erase anything after this last slash.
                            sBaseURLDirectory.erase(nLastForwardSeparatorPosition+1, sBaseURLDirectory.length()-(nLastForwardSeparatorPosition+1));

                        // We append the relative URLs path in its place, as per RFC 1808, section 4, step 6.
                        sBaseURLDirectory += relativeURL.GetComponent(kComponentAbsolutePath); // Now we have something like "/dir1/dir2/dir3/../../dirX/fileX.html"
                        CanonicalizePath(sBaseURLDirectory.c_str(), sBaseURLDirectory);
                        relativeURL.SetComponent(kComponentAbsolutePath, sBaseURLDirectory.c_str());
                    }
                    else
                    {
                        // Fall through to the MakeURLStringFromComponents() call below.
                    }
                }
                else
                {
                    // According to RFC 1808, section 4, step 5, if the embedded URL absolute path is empty 
                    // we should copy it from the base -- RFC 1808, section 4, step 5. However, in the examples
                    // given in section 5.1, it says that relative="//g", base="http://a/b/c/d;p?q#f", the result is "http://g"
                    // According to the standard, "//g" is a server or net location, not a path. Thus, the 
                    // relative path is empty and the result should be "http://a/b/c/d;p?q#f". The only way I can 
                    // make sense of this is if "//g" somehow does not refer to a server but instead is the 
                    // path. However, section 2.4.3 of RFC 1808 specifically says that "//" absolutely precedes 
                    // the server.
                    const String& sAbsolutePath2(baseURL.GetComponent(kComponentAbsolutePath));
                    relativeURL.SetComponent(kComponentAbsolutePath, sAbsolutePath2.c_str());

                    if(relativeURL.GetComponent(kComponentParameters).length() == 0)              // Step 5.a
                    {
                        relativeURL.SetComponent(kComponentParameters, baseURL.GetComponent(kComponentParameters).c_str());

                        if(relativeURL.GetComponent(kComponentQuery).length() == 0)               // Step 5.b
                        {
                            relativeURL.SetComponent(kComponentQuery, baseURL.GetComponent(kComponentQuery).c_str());

                            if(relativeURL.GetComponent(kComponentFragment).length() == 0)         // Step 5.c, which actually doesn't exist, but it seems that it ought to, at least for HTTP.
                                relativeURL.SetComponent(kComponentFragment, baseURL.GetComponent(kComponentFragment).c_str());
                        }
                    }
                    // Fall through to the MakeURLStringFromComponents call below.
                }

                // RFC 1808, section 4, step 7, says to just use the resulting combination URL components from base and new.
                relativeURL.MakeURLStringFromComponents(); // Glue the components properly to form a new single full string.
                *this = relativeURL; // Just copy it into ourselves.
            }
        }

        return true;
    }

    // There is no base URL, so just copy new URL. RFC 1808, section 4, step 1 says to do this.
    String sTemp(pNewURL);
    if(pNewURL)
        msURL = pNewURL;          
    else
        msURL.clear();
    return ParseURL(sTemp); // ParseURL will modify sTemp as it goes, in the sake of speed. 
}


///////////////////////////////////////////////////////////////////////////////
// CanonicalizePath
//
// Given an absolute path, this function converts "/.."  and '/." sequences appropriately.
//
bool URL::CanonicalizePath(const char8_t* pPathSource, String& sPathResult)
{
    if(sPathResult.c_str() != pPathSource)
        sPathResult = pPathSource;

    // RFC 1808 Section 4, Step 6.a: All occurrences of "./", where "." is a complete path 
    // segment, are removed. Don't match "./" strings that are within "../" sequences. 
    if(sPathResult.length() >= 1)
    {
        size_t nLastPosition(0);

        for(;;)
        {
            const size_t nCurrentDirectoryMarker(sPathResult.find("./", nLastPosition));

            if(nCurrentDirectoryMarker >= sPathResult.length())
                break;

            nLastPosition = nCurrentDirectoryMarker + 1;

            if((nCurrentDirectoryMarker == 0) || (sPathResult[nCurrentDirectoryMarker - 1] != '.')) // If it is not part of "../"
                sPathResult.erase(nCurrentDirectoryMarker, 2); // Remove the "./" outright. 
        }
    }

    // Step 6.b: If the path ends with "." as a complete path segment, that "." is removed.
    if(sPathResult.length() >= 1)
    {
        if(sPathResult[sPathResult.length() - 1] == '.' && 
            ((sPathResult.length() == 1) || (sPathResult[sPathResult.length() - 2] == '/')))
        {
            sPathResult.erase(sPathResult.length()-1, 1); // Remove the trailing '.'.
        }
    }

    //Step 6.c: All occurrences of "<segment>/../", where <segment> is a
    //             complete path segment not equal to "..", are removed.
    //             Removal of these path segments is performed iteratively,
    //             removing the leftmost matching pattern on each iteration,
    //             until no matching pattern remains. 
    //             Note by Paul P: I don't think it is possible for "<segment>/../" 
    //             to occur in any (valid) case where <segment> could ever be "..". 
    //             The reason is simply that no valid full path should ever have ".." 
    //             anywhere in it. So I think it is pointless to compare <segment> to "..".
    //             Section 5.2 of RFC1808 suggests that the URL simply be left with
    //             the ".." in front of it, though it will be a malformed URL.
    if(sPathResult.length() >= 4)
    {
        #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_PLAYSTATION2)
            // Here we deal with the case of a URL in the form of: "file:///C:/Blah/Busy Overpass.jpg"
            // and thus an sURL of the form: "/C:/Blah/Busy Overpass.jpg"
            if((sPathResult.length() < 2) || (!EA::StdC::Isalpha(sPathResult[0]) && !(sPathResult[1] == ':')))
                EAPATCH_ASSERT(sPathResult[0] == '/');
        #else
            EAPATCH_ASSERT(sPathResult[0] == '/');
        #endif


        for(;;)
        {
            const size_t nLastSeparatorDotDotSeparatorPosition(sPathResult.find("/../"));
            if(nLastSeparatorDotDotSeparatorPosition < sPathResult.length()) // If found...
            {
                if(nLastSeparatorDotDotSeparatorPosition == 0) // If there is nothing before the "/../" text, then the path is in error.
                    sPathResult.erase(0, 3); // Just erase the beginning "/.." part. The URL is invalid anyway.
                else
                {
                    // We have something that could be one of the following forms: "/a/../b/c", "/a/b/../c", "/a/b/c/../", or "/a/b/c/../../b/../g"
                    // These get converted respectively, to: "/b/c", "/a/c", "/a/b/", and "/a/g"
                    size_t nBeginningOfParentSegment(nLastSeparatorDotDotSeparatorPosition-1);
                    size_t nWraparoundValue(0); //This will store the result of what happens when the unsigned value of 0 gets decremented by one.
                    nWraparoundValue--;

                    while((nBeginningOfParentSegment != nWraparoundValue) && (sPathResult[nBeginningOfParentSegment] != '/'))
                        nBeginningOfParentSegment--;

                    nBeginningOfParentSegment++; // Since we exited the loop on the first non-parent char, we go forward back to the first parent char.
                    sPathResult.erase(nBeginningOfParentSegment, nLastSeparatorDotDotSeparatorPosition + 4 - nBeginningOfParentSegment); // Remove the "parent_segment/../" sequence.
                }
            }
            else
                break;
        }
    }

    //Step 6.d: If the path ends with "<segment>/..", where <segment> is a
    //             complete path segment not equal to "..", that "<segment>/.." is removed.
    //             Note by Paul P: I don't think it is possible for "<segment>/.." 
    //             to occur in any (valid) case where <segment> could ever be "..". 
    //             The reason is simply that no valid full path should ever have ".." 
    //             anywhere in it. So I think it is pointless to compare <segment> to "..".
    if(sPathResult.length() >= 4) // If there is room for the pattern "<segment>/.."
    {
        const size_t nLastSeparatorDotDotPosition(sPathResult.length() - 3);

        // This is what we want to say, but the SGI STL 2.0 string class rfind function is broken and can't do rfind on substrings at the end of main string!
        // if(sPathResult.rfind("/..") == nLastSeparatorDotDotPosition){ // If path ends with "/.." 

        // Alternative version that works with SGI STL 2.0 string class:
        if((sPathResult.rfind('/') == nLastSeparatorDotDotPosition) && // If path ends with "/.." 
            sPathResult[nLastSeparatorDotDotPosition+1] == '.'      &&
            sPathResult[nLastSeparatorDotDotPosition+2] == '.')
        {
            size_t nBeginningOfParentSegment(nLastSeparatorDotDotPosition-1);
            size_t nWraparoundValue(0); // This will store the result of what happens when the unsigned value of 0 gets decremented by one.
            nWraparoundValue--;
            while((nBeginningOfParentSegment != nWraparoundValue) && (sPathResult[nBeginningOfParentSegment] != '/'))
                nBeginningOfParentSegment--;
            nBeginningOfParentSegment++; // Since we exited the loop on the first non-parent char, we go forward back to the first parent char.
            sPathResult.erase(nBeginningOfParentSegment, nLastSeparatorDotDotPosition+3-nBeginningOfParentSegment); // Remove the "parent_segment/.." sequence.
        }
    }

    return true;
}                                                 


///////////////////////////////////////////////////////////////////////////////
// ChangeURL
//
bool URL::ChangeURL(const char8_t* pNewURL, URL* pBaseURL)
{
    if(pBaseURL)
    {
        const String& sBaseURL = pBaseURL->GetFullURL();
        if(sBaseURL.length())
            return ChangeURL(pNewURL, sBaseURL.c_str());
    }
    return ChangeURL(pNewURL, (char8_t*)NULL);
}


///////////////////////////////////////////////////////////////////////////////
// MakeURLStringFromComponents
// 
// Rebuilds a proper string from separate components. This is useful for taking
// the parts of a relative URL and base URL and making the true final URL.
//
// Here are full URL formats:
//     http://<username>:<password>@<host>:<port>/<path>?<searchpart>#<fragment>
//     ftp://<username>:<password>@<host>:<port>/<path>;type=<typecode>
//     nntp://<username>:<password>@<host>:<port>/<newsgroup-name>/<article-number>
//     irc://<username>:<password>@<host>:<port>
//     mailto:<rfc822-address-specification>
// where any part but the scheme may be absent.
//
bool URL::MakeURLStringFromComponents()
{
    if(!mbParsed)
    {
        String sURLCopy = msURL;
        ParseURL(sURLCopy);
    }

    msURL.clear();

    // Scheme (e.g. http://)
    if(!msComponent[kComponentScheme].empty())
    {
        msURL = msComponent[kComponentScheme];
        msURL += ':';
        if(mURLType != kURLTypeMailTo) // The 'mailto' scheme is the only scheme (of our supported set) that doesn't use '//'.
            msURL += "//";
    }

    // User name 
    if(msComponent[kComponentUsername].length() || msComponent[kComponentPassword].length()) // Technically speaking, a password without a username is malformed.
    {
        msURL += msComponent[kComponentUsername];

        if(msComponent[kComponentPassword].length())
        {
            msURL += ':';
            msURL += msComponent[kComponentUsername];
        }

        msURL += '@';
    }

    // Server
    msURL += msComponent[kComponentServer];
    if(GetPort() != 0)
    {
        msURL += ':';
        msURL += msComponent[kComponentPort];
    }

    msURL += msComponent[kComponentAbsolutePath];
    msURL += msComponent[kComponentParameters];
    msURL += msComponent[kComponentQuery];
    msURL += msComponent[kComponentFragment];

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// MakeRelativeURLFromBaseAndTarget
//
// sBase and sTarget should be full URL strings or simply paths and extra data.
// The paths must be converted to URL standard form before calling this function.
// Thus, if you want to use Windows file paths, you must first call the 
// 'ConvertPathFromLocalToURLForm()' on them before calling this. This is important
// because it allows the algorithm to work (virtually impossible otherwise) because
// of special character rules and case sensitivity issues.
//
// Paul's rules for converting target path to relative path:
//     1 Parse base and target as regular URLs. The absolute path and extra data
//        portions of each of these are what we are interested in.
//
//     2 Remove everything after the last '/' of the base absolute path. Basically,
//        we want the base path directory alone.
//
//     3 If target begins with non-'/' (i.e. doesn't have a directory specified),
//        simply make the result be the the target pasted onto the result of step two
//        above. Then we are done. If target begins with '/' (normally so), proceed on.
//
//     4 Take both base result from step 2 and target and catalog their directories.
//        For example, "/a/b/c/" gets cataloged as "/", "a/", "b/", and "/c".
//
//     5 Count how many of the dirs in the base are not in the target. The result 
//        is 0 or more; we'll call this number "n".
//
//     6 Simply put n "../"s in front of the part of the target that is 
//        not in the base part. That's it for the directory part. Pretty simple, huh?
//
//        Example: Base = "/a/b/c/d", target1 = "k/m", target2 = "/a/b/x", target3 = "/a/b/c/f/g"
//        The result of step 2 is "/a/b/c/"
//        The result of step 3 is target 1 gets converted to the final result1 of "/a/b/c/k/m".
//        The result of step 5 is that for target2, 1 dir is not in the base,and for target2, 0 dirs aren't in base.
//        The resulting relative URLs are thus: result2 = "../x". result3 = "f/g"
//
//     7 We deal with 'extra data' (typedata (';'), query info('?'), fragment(#)) separately...
//        (As of this writing I don't have the details of this in place yet).
//
bool URL::MakeRelativeURLFromBaseAndTarget(const char8_t* pBase, const char8_t* pTarget, String& sResult)
{
    String sTemp;
    URL      baseURL(pBase);
    URL      targetURL(pTarget);

    sResult.clear();

    // Do step 1 above.
    sTemp = pBase;
    baseURL.ParseURL(sTemp);
    sTemp = pTarget;
    targetURL.ParseURL(sTemp);

    String sBaseURLDirectory(baseURL.GetComponent(kComponentAbsolutePath));
    String sTargetURLPath(targetURL.GetComponent(kComponentAbsolutePath));

    // Do step 2 above.
    const size_t nLastForwardSeparatorPosition(sBaseURLDirectory.rfind('/'));
    if(nLastForwardSeparatorPosition < sBaseURLDirectory.length()) //Erase anything after this last slash.
        sBaseURLDirectory.erase(nLastForwardSeparatorPosition + 1, sBaseURLDirectory.length() - (nLastForwardSeparatorPosition + 1));

    if(sTargetURLPath.length())
    {
        if(sTargetURLPath[0] == '/')
        {
            size_t i, iEnd, nLastDirectoryStartPosition;

            // Do step 4 above
            StringArray baseDirectoryList, targetDirectoryList;

            for(i = nLastDirectoryStartPosition = 0, iEnd = sBaseURLDirectory.length(); i < iEnd; i++)
            {
                if((sBaseURLDirectory[i] == '/') || (i == (iEnd - 1))) // If at directory end or at last char of string...
                {
                    const String sCurrentDirectory(sBaseURLDirectory.data() + nLastDirectoryStartPosition, i - nLastDirectoryStartPosition+1);
                    baseDirectoryList.push_back(sCurrentDirectory);
                    nLastDirectoryStartPosition = i+1;
                }
            }
            for(i = nLastDirectoryStartPosition = 0, iEnd = sTargetURLPath.length(); i < iEnd; i++)
            {
                if((sTargetURLPath[i] == '/') || (i == (iEnd - 1))) // If at directory end or at last char of string...
                {
                    const String sCurrentDirectory(sTargetURLPath.data() + nLastDirectoryStartPosition, i - nLastDirectoryStartPosition+1);
                    targetDirectoryList.push_back(sCurrentDirectory);
                    nLastDirectoryStartPosition = i+1;
                }
            }

            // Do step 5 above.
            for(i = 0, iEnd = URL_MIN(baseDirectoryList.size(), targetDirectoryList.size()); i < iEnd; i++)
            {
                if(baseDirectoryList[i] != targetDirectoryList[i])
                    break;
            } // Upon exit of loop. 'i' is number of diretories in common between the two lists.

            const size_t nCountOfDirectoriesInBaseThatAreNotInTarget = iEnd - i;

            // Do step 6 above.
            for(i = 0, iEnd = URL_MIN(sBaseURLDirectory.length(), sTargetURLPath.length()); i < iEnd; i++)
            {
                if(sBaseURLDirectory[i] != sTargetURLPath[i])
                    break;
            } // Upon exit of this loop, 'i' will be equal to count of common chars, with min=0 and max=base dir length.

            sTargetURLPath.erase(0, i);
            for(i = 0; i < nCountOfDirectoriesInBaseThatAreNotInTarget; i++)
                sResult += "../";
            sResult += sTargetURLPath;
        }
        else // Do step 3 above.
        {
            sResult  = sBaseURLDirectory;
            sResult += sTargetURLPath;
        }

        // Tack on extra data if present. The rules for how to do this are less clear, but it seems
        // that any part that we leave out of the relative should be picked up from the base.
        // Thus, we only append this extra data if it is present in the relative URL. It is less
        // clear how to deal with the base having extra data but the relative not having extra
        // data, but presumably, the base extra data must go with the relative data upon reconstruction
        // of the relative URL to a full URL.
        if(targetURL.GetComponent(kComponentParameters).length())
            sResult += targetURL.GetComponent(kComponentParameters);
        if(targetURL.GetComponent(kComponentQuery).length())
            sResult += targetURL.GetComponent(kComponentQuery);
        if(targetURL.GetComponent(kComponentFragment).length())
            sResult += targetURL.GetComponent(kComponentFragment);

        return true;
    }
    else // Target is empty. Nothing we can really do except copy base.
        sResult = pBase;
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// ParseURLConst
//
// Exists for the sole purpose of being callable from a const function. 
// The obiviates the need for what would otherwise be a mess of mutable 
// declarations within this class.
//
bool URL::ParseURLConst(String& sURL) const
{
    URL* const pThisNonConst = const_cast<URL*>(this);
    return pThisNonConst->ParseURL(sURL);
}


///////////////////////////////////////////////////////////////////////////////
// ParseURL
//
// This function takes a potentially full URL of the form, by example:
//    "http://someuser:mypassword@www.maxis.com:80/SimCity/SimCity.html"
// and breaks it down into all the subcomponents.
//
bool URL::ParseURL(String& sURL)
{
    bool bResult(false);
    
    // First clear out the entities.
    mURLType = kURLTypeUndefined;

    msComponent[kComponentScheme]      .clear();
    msComponent[kComponentUsername]    .clear();
    msComponent[kComponentPassword]    .clear();
    msComponent[kComponentServer]      .clear();
    msComponent[kComponentAbsolutePath].clear();
    msComponent[kComponentParameters]  .clear();
    msComponent[kComponentQuery]       .clear();
    msComponent[kComponentFragment]    .clear();
    msComponent[kComponentPort]        .clear();

    ParseScheme(sURL);                      // Parse out the scheme if present. It should be present to be proper.

    if(mURLType == kURLTypeHTTP || 
       mURLType == kURLTypeHTTPS)
    {
        ParseUsernameAndPassword(sURL);     // Parse out the username and password if present.
        ParseServerAndPort(sURL);           // Parse out the server and port if present.
        bResult = ParseURLHTTP(sURL);
    }
    else if(mURLType == kURLTypeFile)
    {
        ParseServerAndPort(sURL);
        bResult = ParseURLFILE(sURL);
    }
    else if(mURLType == kURLTypeFTP)
    {
        ParseUsernameAndPassword(sURL);
        ParseServerAndPort(sURL);
        bResult = ParseURLFTP(sURL); 
    }
    else if(mURLType == kURLTypeMailTo)
    {
        bResult = ParseURLMailTo(sURL); 
    }
    else if(mURLType == kURLTypeRelative)
    {
        ParseServerAndPort(sURL);
        bResult = ParseURLRelative(sURL);
    }
    else if(mURLType == kURLTypeUnknown)
    {
        bResult = ParseURLUnknown(sURL);
    }

    mbParsed = bResult;

    return bResult; 
}


///////////////////////////////////////////////////////////////////////////////
// ParseScheme
//
// This function takes a potentially full URL of the form, by example:
//    "http://someuser:mypassword@www.maxis.com:80/SimCity/SimCity.html"
// and pulls out the scheme and leaves the string as:
//    "//someuser:mypassword@www.maxis.com:80/SimCity/SimCity.html"
//
bool URL::ParseScheme(String& sURL){
    //First clear out the entities.
    mURLType = kURLTypeUndefined;
    msComponent[kComponentScheme].clear();

    if((sURL.find("URL:") == 0) || (sURL.find("url:") == 0)) //If the string begins with "URL:" (case sensitive)
        sURL.erase(0, 4); //The standard suggests that "url:" at the beginning of a URL is legal, so we strip it.

    //It is easier to usually check for "://" in the scheme than just the ':',
    //    because in the real world, URLs sometimes don't include schemes and 
    //    we must deduce them. As a result, searching for the first ':' alone 
    //    in the URL can yield one that isn't part of the scheme. However, "://"
    //    is closer to being unique, and so we usually search for that instead.
    //    The "mailto" scheme is unique among common schemes in that it doesn't 
    //    have "://", but instead only ":".
    
    if(sURL.find("mailto:") == 0) // If the string begins with "mailto:"
    {
        msComponent[kComponentScheme].assign(sURL.data(), 6);
        sURL.erase(0, 7);
        return true;
    }
    else
    {
        const size_t nPosition(sURL.find("://"));

        if(nPosition < sURL.length()) // If found...
        {
            for(size_t i=0; i<nPosition; i++)  // The scheme is case insensitive, so we convert to all lower if we find one.
                msComponent[kComponentScheme] += (char)EA::StdC::Tolower(sURL[i]);
            sURL.erase(0, nPosition+1); // Leave the string so that it starts with "//"
        }
    }

    //Parse the scheme-specific data.
    if(msComponent[kComponentScheme].length())
    {
        if(msComponent[kComponentScheme] == "http")
            mURLType = kURLTypeHTTP;
        else if(msComponent[kComponentScheme] == "https")
            mURLType = kURLTypeHTTPS;
        else if(msComponent[kComponentScheme] == "file")
            mURLType = kURLTypeFile;
        else if(msComponent[kComponentScheme] == "ftp")
            mURLType = kURLTypeFTP;
        else if(msComponent[kComponentScheme] == "mailto")
            mURLType = kURLTypeMailTo;
        // Else leave undefined.
    }

    if(mURLType == kURLTypeUndefined)
    {
        if(!mbStrictConformance)
        {
            // No known scheme. Perhaps the URL is in the form of "www.maxis.com". 
            // In this case we would assume it is http scheme. Doing this is 
            // technically non-standard and in rare cases could result in 
            // misinterpretation of the URL.
            if(sURL.find("www.") == 0)
                mURLType = kURLTypeHTTP;
            else if(sURL.find("ftp.") == 0)
                mURLType = kURLTypeFTP;
            // Else leave undefined.

            // Since a proper URL of one of these types needs to begin with <scheme:>//,
            // here we fake the presence of the '//' part. This is so the further parsing
            // code sees things properly.
            sURL.insert(0, "//");

            return true;
        }

        // We can't tell by looking at it, so it could be a relative URL, an unknown type, or an error.
        // From RFC 1808:
        //  If the parse string contains a colon ":" after the first character
        //  and before any characters not allowed as part of a scheme name (i.e.,
        //  any not an alphanumeric, plus "+", period ".", or hyphen "-"), the
        //  <scheme> of the URL is the substring of characters up to but not
        //  including the first colon. These characters and the colon are then
        //  removed from the parse string before continuing.
        mURLType = kURLTypeRelative; //We'll call it relative until proven otherwise below.

        const size_t nColonPosition(sURL.find(':'));

        if((nColonPosition < sURL.length()) && (nColonPosition >= 1))
        {
            size_t nCurrentPosition(nColonPosition - 1);
            size_t nWraparoundValue(0);
            nWraparoundValue--;

            while(nCurrentPosition != nWraparoundValue)
            {
                if(!IsValidASCII(sURL[nCurrentPosition]) ||
                    (!EA::StdC::Isalnum(sURL[nCurrentPosition]) && sURL[nCurrentPosition] != '+' &&
                                                                   sURL[nCurrentPosition] != '.' &&
                                                                   sURL[nCurrentPosition] != '-'))
                {
                    break; // It is a char that can't be part of a scheme.
                }
                nCurrentPosition--;
            }

            if(nCurrentPosition == nWraparoundValue)
            {
                mURLType = kURLTypeUnknown; // There is a scheme present, but it is of an unknown type.
                msComponent[kComponentScheme].assign(sURL.data(), nColonPosition); // Copy the unknown scheme and 
                sURL.erase(0, nColonPosition+1);                  // erase it from the URL, leaving the "//" at the beginning of the string, if it was there.
            }
        }
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// ParseUsernameAndPassword
//
// This function takes a URL of the form, by example:
//    "//someuser:mypassword@www.maxis.com:80/SimCity/SimCity.html"
// and pulls out the user and password and leaves the string as:
//    "www.maxis.com:80/SimCity/SimCity.html"
//
bool URL::ParseUsernameAndPassword(String& sURL){
    // First clear out the entities.
    msComponent[kComponentUsername].clear();
    msComponent[kComponentPassword].clear();
 
    // Look for username and password. Usually these are not present.
    // Username and password shouldn't be here unless there is a '//' at the beginning of the string.
    if(sURL.find("//") == 0) // Only if the portion of the url after the scheme begins with "//" is the portion the user, password, and server (a.k.a. net location).
    {
        const size_t nAtPosition(sURL.find('@'));
        const size_t nSeparatorPosition(sURL.find('/', 2));
 
        if((nAtPosition < sURL.length()) && ((nSeparatorPosition == String::npos) || (nAtPosition < nSeparatorPosition))) // If there is an @ char and it is before the next / char...
        {
            msComponent[kComponentUsername].assign(sURL.data() + 2, nAtPosition - 2);
            sURL.erase(2, nAtPosition - 1);  // Remove the "someuser@somepassword@" segment from sURL.

            const size_t nColonPosition(msComponent[kComponentUsername].find(':'));
 
            if(nColonPosition < msComponent[kComponentUsername].length()) //If there is a password given...
            {
                msComponent[kComponentPassword].assign(msComponent[kComponentUsername].data() + nColonPosition + 1, msComponent[kComponentUsername].length() - nColonPosition - 1);
                msComponent[kComponentUsername].erase(nColonPosition, msComponent[kComponentUsername].length() - nColonPosition);
            }
        }
    }
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// ParseServerAndPort
//
// This function takes a URL of the form, by example:
//    "//www.maxis.com:80/SimCity/SimCity.html"
// and pulls out the server and port and leaves the string as:
//    "/SimCity/SimCity.html"
//
bool URL::ParseServerAndPort(String& sURL){
    size_t nColonPosition, nSeparatorPosition;

    // First clear out the entities.
    msComponent[kComponentServer].clear();
    msComponent[kComponentPort]  .clear();

    if(sURL.find("//") == 0){ // If it begins with "//". Only if the portion of the url after the scheme begins with "//" is the portion the server or net location.
        sURL.erase(0, 2); // Simply erase the leading "//". We won't be needing it any more.
        const size_t nLengthInitial(sURL.length());

        if(mURLType == kURLTypeFile) // We may have an sURL of something like "C:/Blah/Blah.html"
            return true; // In this case, there is no server and the file is simply a path from here on out.

        // Look for port and absolute path slash delimiter.
        // ChangePathSeparators(sURL, kSeparatorTypeBackward, kSeparatorTypeForward); // I'm don't think we want to do this, as '\\' is considered
        nColonPosition     = sURL.find(':');                                // by the standard to be a safe char that doesn't need encoding.
        nSeparatorPosition = sURL.find('/');

        if(nSeparatorPosition < nColonPosition) // Ignore any colon that might appear after a slash. 
            nColonPosition = nLengthInitial;    // Technically, it would be illegal to do have one anyway.

        if(nColonPosition < nLengthInitial) // If a colon was found... (e.g. "www.maxis.com:80/somedir/file.html")
        {
            size_t i, j;

            for(i = nColonPosition + 1, j = 0; i < nLengthInitial && IsValidASCII(sURL[i]) && EA::StdC::Isdigit(sURL[i]); i++, j++)
                msComponent[kComponentPort] += sURL[i];

            msComponent[kComponentServer].assign(sURL.data(), nColonPosition); // Set the server to be everything to the left of the colon.
            sURL.erase(0, nColonPosition + j + 1);   // Set the URL to be everything after the port digits.
        }
        else if(nSeparatorPosition < nLengthInitial){ // If a slash was found before any colon... (e.g. "www.maxis.com/somedir/file.html")
            msComponent[kComponentServer].assign(sURL.data(), nSeparatorPosition); // e.g. msComponent[kComponentServer] is now "www.maxis.com"
            sURL.erase(0, nSeparatorPosition);                                     // e.g. sURL is now "/somedir/file.html"
        }
        else // Else neither a slash nor a colon was found.
        {
            // We are more or less stuck writing some code here that needs to check the 
            // scheme before being able to parse appropriately. This is because the 
            // URL standard says that that either the server and/or path part of the URL
            // may be absent. Well, that leaves us in a small bind, since the remaining 
            // portion of the URL may have a different format depending on the scheme.
            // Luckily, only HTTP and FTP have special trailing extra data fields.
            if((mURLType == kURLTypeHTTP) || (mURLType == kURLTypeHTTPS))
            {
                // We have something of the form of any of these (under http):
                //    www.maxis.com
                //    www.maxis.com?searchinfo#fragment
                //    www.maxis.com?searchinfo
                //    www.maxis.com#fragment
                const size_t nSemicolonPosition(sURL.find(';')); // RFC 1738 doesn't suggest ';' is part of HTTP extra data, but RFC 1808 does.

                if(nSemicolonPosition < sURL.length()) // Note that 'nLengthInitial' may no longer be valid here.
                {
                    msComponent[kComponentServer].assign(sURL.data(), nSemicolonPosition);
                    sURL.erase(0, nSemicolonPosition);
                }

                if(msComponent[kComponentServer].length() == 0) // If the above semicolon wasn't found...
                {
                    const size_t nQuestionMarkPosition(sURL.find('?'));

                    if(nQuestionMarkPosition < sURL.length()){ // Note that 'nLengthInitial' may no longer be valid here.
                        msComponent[kComponentServer].assign(sURL.data(), nQuestionMarkPosition);
                        sURL.erase(0, nQuestionMarkPosition);
                    }
                }

                if(msComponent[kComponentServer].length() == 0) // If the above semicolon wasn't found...
                {
                    const size_t nPoundPosition(sURL.find('#'));

                    if(nPoundPosition < sURL.length()) // Note that 'nLengthInitial' may no longer be valid here.
                    {
                        msComponent[kComponentServer].assign(sURL.data(), nPoundPosition);
                        sURL.erase(0, nPoundPosition);
                    }
                }
            }
            else if((mURLType == kURLTypeHTTP) || (mURLType == kURLTypeHTTPS))
            {
                // We have something of the form of any of these (under ftp):
                //    www.maxis.com
                //    www.maxis.com;typecode
                const size_t nSemicolonPosition(sURL.find(';'));

                if(nSemicolonPosition < sURL.length()) //Note that 'nLengthInitial' may no longer be valid here.
                {
                    msComponent[kComponentServer].assign(sURL.data(), nSemicolonPosition);
                    sURL.erase(0, nSemicolonPosition);
                }
            }

            if(!msComponent[kComponentServer].length()) // If the above code didn't find any special trailing symbols (like '?'. '#', ';')...
            {
                msComponent[kComponentServer] = sURL;
                sURL.clear();
            }
        }
    }
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// ParseURLHTTP
//
// This function takes a URL of the form, by example:
//    "/SimCity/SimCity.html?formdata"
// and pulls out the path and 'searchpart' leaves the string empty
//
// A full HTTP URL takes the form:
//    http://<username>:<password>@<host>:<port>/<path>;<parameters>?<searchpart>#<fragment>
//
// where <host> and <port> are as described in Section 3.1. If :<port>
// is omitted, the port defaults to 80.  No user name or password is
// allowed.  <path> is an HTTP selector, and <searchpart> is a query
// string. The <path> is optional, as is the <searchpart> and its
// preceding "?". If neither <path> nor <searchpart> is present, the "/"
// may also be omitted.
//
// Within the <path> and <searchpart> components, "/", ";", "?" are
// reserved. The "/" character may be used within HTTP to designate a
// hierarchical structure.
//
// We make the assumption that either the searchpart, fragment or neither
// may be present, but assume that both will not be present at any time.
// If both are present at once, then the parsing will leave both of them
// together in the extra data string. If this is a problem, we can always 
// fix it later.
//
// Note that we leave the '?' or '#' in the extra data, rather than leave
// it out. This is because it allows us to easily reconstruct the original
// URL path and allows user to know which of '?' or '#' was used.
//
bool URL::ParseURLHTTP(String& sURL)
{
    // First clear out the entities.
    msComponent[kComponentAbsolutePath].clear();
    msComponent[kComponentParameters]  .clear();
    msComponent[kComponentQuery]       .clear();
    msComponent[kComponentFragment]    .clear();

    if(sURL.length())
    {
        size_t nSemicolonPosition, nQuestionMarkPosition, nPoundPosition;

        //Look for "parameter" data
        nSemicolonPosition    = sURL.find(';');    //RFC 1738 doesn't suggest ';' is part of HTTP extra data, but RFC 1808 and RFC 2068 do.
        nQuestionMarkPosition = sURL.find('?');
        nPoundPosition        = sURL.find('#');

        if(nSemicolonPosition < sURL.length()          && // If semicolon was found...
          (nSemicolonPosition < nQuestionMarkPosition) && // It is possible that other semicolons may be after a '?' symbol.
          (nSemicolonPosition < nPoundPosition))          // We assume that it is possible that other semicolons may be after a '#' symbol.
        {
            size_t nEndPosition;

            if(nQuestionMarkPosition < sURL.length())
                nEndPosition = nQuestionMarkPosition;
            else if(nPoundPosition < sURL.length())
                nEndPosition = nPoundPosition;
            else
                nEndPosition = sURL.length();

            if(!msComponent[kComponentAbsolutePath].length()) //It should be empty at this point.
                msComponent[kComponentAbsolutePath].assign(sURL.data(), nSemicolonPosition);

            msComponent[kComponentParameters].assign(sURL.data() + nSemicolonPosition, nEndPosition - nSemicolonPosition);
            sURL.erase(0, nEndPosition);
        }

        // Look for "query" data
        nQuestionMarkPosition = sURL.find('?');
        nPoundPosition        = sURL.find('#');

        if(nQuestionMarkPosition < sURL.length() &&    // If semicolon was found...
          (nQuestionMarkPosition < nPoundPosition))    // We assume that it is possible that other question marks may be after a '#' symbol.
        {
            size_t nEndPosition;

            if(nPoundPosition < sURL.length())
                nEndPosition = nPoundPosition;
            else
                nEndPosition = sURL.length();

            if(!msComponent[kComponentAbsolutePath].length())
                msComponent[kComponentAbsolutePath].assign(sURL.data(), nQuestionMarkPosition);

            msComponent[kComponentQuery].assign(sURL.data() + nQuestionMarkPosition, nEndPosition - nQuestionMarkPosition);
            sURL.erase(0, nEndPosition);
        }

        // Look for "fragment" data
        nPoundPosition = sURL.find('#');

        if(nPoundPosition < sURL.length())
        {
            if(!msComponent[kComponentAbsolutePath].length())
                msComponent[kComponentAbsolutePath].assign(sURL.data(), nPoundPosition);

            msComponent[kComponentFragment].assign(sURL.data() + nPoundPosition, sURL.length() - nPoundPosition);
            sURL.clear(); //We have eaten the entire thing.
        }

        //If we found none of the above special data items, we just assign the entire rest of the URL to the absolute path.
        if(!msComponent[kComponentAbsolutePath].length()) //If nothing was found above.
            msComponent[kComponentAbsolutePath] = sURL;
    }
    else
    {
        if(mURLType != kURLTypeRelative) // The URI standard states that an empty abs path can (should?) be set to '/'.
            msComponent[kComponentAbsolutePath] = '/';
    }

    sURL.clear(); //We have eaten the entire string. Clear it just to make sure.
    return true;
}


///////////////////////////////////////////////////////////////////////////////
// ParseURLFILE
//
// A file URL takes the form:
//      file://<host>/<path>
// where <host> is the fully qualified domain name of the system on
// which the <path> is accessible, and <path> is a hierarchical
// directory path of the form <directory>/<directory>/.../<name>.
//
// For example, a VMS file
//      DISK$USER:[MY.NOTES]NOTE123456.TXT
// might become
//      <"file://vms.host.edu/disk$user/my/notes/note12345.txt"
//
// As a special case, <host> can be the string "localhost" or the empty
// string; this is interpreted as `the machine from which the URL is
// being interpreted'.
//
// The file URL scheme is unusual in that it does not specify an
// Internet protocol or access method for such files; as such, its
// utility in network protocols between hosts is limited.
//
// Here are examples of a full file URL (encoding not shown):
//    "file:///C|/Blah/Busy Overpass.jpg"
//    "file:///C:/Blah/Busy Overpass.jpg"  // This is annoying, as it is ambiguous, but it is sometimes seen.
//    "file://C:/Blah/Busy Overpass.jpg"
//    "file://C:\Blah\Busy Overpass.jpg"
//    "C:\Blah\Busy Overpass.jpg"
//
// This function takes a URL of the form, by example:
//    "/SimCity/SimCity.html"
// and pulls out the path (the whole thing) and leaves the string empty
// Other examples of input:
//    "/Blah/Busy.jpg"
//    "/C|/Blah/Busy%20Overpass.jpg"
//    "/C:/Blah/Busy%20Overpass.jpg"         // This is annoying, as it is ambiguous, but it is sometimes seen.
//    "C:/Blah/Busy Overpass.jpg"
//    "C:\Blah\Busy%20Overpass.jpg"
//    "C:\Blah\Busy Overpass.jpg"
//
bool URL::ParseURLFILE(String& sURL)
{
    if(msComponent[kComponentServer] == "localhost") // Case sensitive compare. Standard says that 'localhost' means same as empty.
        msComponent[kComponentServer].clear();

    #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_PLAYSTATION2)
        // Here we deal with the case of a URL in the form of: "file:///C:/directory/file.jpg"
        // and thus an input sURL of the form: "/C:/directory/file.jpg".
        // We also deal with a UNC URL of the form: "file:///\\share/volume/directory/file.jpg" 
        // and thus an input sURL of the form: "/\\share/volume/directory/file.jpg"
        // To consider: support a URL of the form file://///share/volume/directory/file.jpg, though such a 
        // URL is not correct, because the leading '\\' chars of a UNC path are not file path separators and
        // thus aren't something that are specified as '/' chars.

        if((sURL.length() >= 3) && (sURL[0] == '/') && 
             (EA::StdC::Isalpha(sURL[1]) && (sURL[2] == ':')) ||
             ((sURL[1] == '\\') && (sURL[2] == '\\')))
        {
            sURL.erase(0, 1);
        }
    #endif

    msComponent[kComponentParameters].clear();
    msComponent[kComponentQuery]     .clear();
    msComponent[kComponentFragment]  .clear();
    msComponent[kComponentAbsolutePath] = sURL;
    sURL.clear();

    return true;
}


//////////////////////////////////////////////////////////////////
// ParseURLFTP
//
// A full ftp URL takes the form:
//    ftp://<username>:<password>@<host>:<port>/<path>;type=<typecode>
//
// The url-path of a FTP URL has the following syntax:
//    <cwd1>/<cwd2>/.../<cwdN>/<name>;type=<typecode>
//
//    Where <cwd1> through <cwdN> and <name> are (possibly encoded) strings
//    and <typecode> is one of the characters "a", "i", or "d". The part
//    ";type=<typecode>" may be omitted. The <cwdx> and <name> parts may be
//    empty. The whole url-path may be omitted, including the "/"
//    delimiting it from the prefix containing user, password, host, and
//    port.
//
bool URL::ParseURLFTP(String& sURL)
{
    //First clear out the entities.
    msComponent[kComponentAbsolutePath].clear();
    msComponent[kComponentParameters]  .clear();
    msComponent[kComponentQuery]       .clear();
    msComponent[kComponentFragment]    .clear();

    if(sURL.length() == 0) // The URI standard states that an empty abs path can (should?) be set to '/'.
        sURL = '/';
    else
    {
        const size_t nSemicolonPosition = sURL.find(';');

        if(nSemicolonPosition < sURL.length())
        {
            msComponent[kComponentAbsolutePath].assign(sURL.data(), nSemicolonPosition);
            msComponent[kComponentParameters].assign(sURL.data() + nSemicolonPosition, sURL.length() - nSemicolonPosition);
        }
        else
            msComponent[kComponentAbsolutePath] = sURL;
    }
    sURL.clear(); //We have eaten the entire string.
    return true;
}


//////////////////////////////////////////////////////////////////
// ParseURLMailTo
//
bool URL::ParseURLMailTo(String& sURL)
{
    // First clear out the entities.
    msComponent[kComponentAbsolutePath].clear();
    msComponent[kComponentParameters].clear();
    msComponent[kComponentQuery].clear();
    msComponent[kComponentFragment].clear();

    msComponent[kComponentAbsolutePath] = sURL;
    sURL.clear();
    return (msComponent[kComponentAbsolutePath].length() != 0);
}


//////////////////////////////////////////////////////////////////
// ParseURLUnknown
//
bool URL::ParseURLUnknown(String& sURL)
{
    msComponent[kComponentAbsolutePath] = sURL;
    sURL.clear();
    return true;
}


//////////////////////////////////////////////////////////////////
// ParseURLRelative
//
bool URL::ParseURLRelative(String& sURL)
{
    //Using HTTP ought to work, as the sURL should be only path and possibly extra data.
    //The problem is if you have a relative URL of the form "w3.blah.com/../a/b/c".
    //How do you know whether the first segment is a directory or server name? It certainly
    //  looks like a server name but in fact is also a valid directory name. I may be wrong,
    //  but the URL standard seems to provide no clear way to tell them apart. Section 2.4.3
    //  of RFC 1808 suggests that the "w3.blah.com" above is in fact a directory name and 
    //  would have to be prefaced by "//" in order for it to be a server or "net location".
    //We thus assume that whatever code executed before this call did in fact recognize this
    //  properly and deal with it.
    return ParseURLHTTP(sURL); 
}


//////////////////////////////////////////////////////////////////
// ConvertPathFromURLToLocalForm
//
// Given a file absolute path in proper URL form, this function 
// converts it to a form that is usable on the current machine.
// The source can be the same as the dest.
// 
// This function really needs to know about the operating system 
// it runs on. But many are the same as each other.
//
// For example, on Windows, the following conversion happens:
//     "/C|/Blah/Busy%20Overpass.jpg"
// becomes:
//     "C:\Blah\Busy Overpass.jpg"
//
void URL::ConvertPathFromURLToLocalForm(const char8_t* pPathSource, String& sPathResult)
{
    String sTempDest(pPathSource); // Note that pPathSource may point to sPathResult.

    #if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_PLAYSTATION2)
        if((sTempDest.length() >= 3) && (sTempDest[0] == '/') && (sTempDest[2] == '|'))
        {
            sTempDest[2] = ':';
            sTempDest.erase(0, 1);  // Remove the leading '/' char.
        }

        ConvertPathFromEncodedForm(sTempDest.c_str(), sTempDest); //Converts things like %20 to ' ', etc.
        ChangePathSeparators(sTempDest.c_str(), sTempDest, kSeparatorTypeForward, kSeparatorTypeBackward);

    #else
        ConvertPathFromEncodedForm(sTempDest.c_str(), sTempDest);
        //ChangePathSeparators(sTempDest.c_str(), sTempDest, kSeparatorTypeBackward, kSeparatorTypeForward); // We can't do this because '\' is a 'safe' char that doesn't need to be encoded. Thus, it can *never* be a path directory delimiter. Unfortunately, some Windows HTML authors sometimes use it as such.

    #endif

    sPathResult = sTempDest;
}


///////////////////////////////////////////////////////////////////////////////
// ConvertPathFromLocalToURLForm
//
// Given a file absolute path in local form, this function converts it to 
// a URL-proper form. Source and dest can be same string.
//
// If you want to pass in a non-full path (e.g. relative path or incomplete path)
// you should call the Fullpath function (provided by framework) on the path
// to make it an absolute path. This function, for coherency, only accepts
// full absolute paths, which is what most code works with anyway.
// 
// This function really needs to know about the operating system 
// it runs on. But many are the same as each other.
//
// For example, on Windows, the following conversion happens:
//     "C:\Blah\Blah Blah.jpg"
// becomes:
//     "C|/Blah/Blah%20Blah.jpg"
//
// With Unix-like paths, the following conversion happens:
//     "C:\Blah\Blah Blah.jpg"
// becomes:
//     "C|/Blah/Blah%20Blah.jpg"
//
void URL::ConvertPathFromLocalToURLForm(const char8_t* pPathSource, String& sPathResult)
{
    String sTempDest(pPathSource); // Note that pPathSource may point to sPathResult.

    #if defined(EA_PLATFORM_MICROSOFT) || defined(EA_PLATFORM_PLAYSTATION2)
        // To consider: Do a findfirst() on the file and the directory. If found, correct case if needed.

        if((sTempDest.length() >= 2) &&  IsValidASCII(sTempDest[0]) && EA::StdC::Isalpha(sTempDest[0]) && (sTempDest[1] == ':'))
            sTempDest.insert((String::size_type)0, (String::size_type)1, '/');  // Three params: insertion_position, repetion_count, char_to_insert

        if((sTempDest.length() >= 3) && (sTempDest[0] == '/') && (sTempDest[2] == ':'))
        {
            sTempDest[2] = '|';
            sTempDest.erase(0, 1);
        }

        const char8_t* const pCharsToNotEncode("/\\|");  // Windows uses either slash character as a directory delimiter, so we leave them as-is and don't encode them. 
                                                         // Also, since we are potentially adding a '|' to the string (and '|' is an invalid 
                                                         // filename character anyways), we must not encode it 

        ChangePathSeparators(sTempDest.c_str(), sTempDest, kSeparatorTypeBackward, kSeparatorTypeForward); 
        ConvertPathToEncodedForm(sTempDest.c_str(), sTempDest, pCharsToNotEncode);
    #else
        const char8_t* const pCharsToNotEncode("/");  // Unix uses exclusively '/' as a directory delimiter, so leave them as is and don't encode them.
        // ChangePathSeparators(sTempDest.c_str(), sTempDest, kSeparatorTypeForward, kSeparatorTypeForward); // Not necessary, since they are already done as forward slashes.
        ConvertPathToEncodedForm(pPathSource, sTempDest, pCharsToNotEncode);
    #endif

    sPathResult = sTempDest;
}


///////////////////////////////////////////////////////////////////////////////
// ChangePathSeparators
//
void URL::ChangePathSeparators(const char8_t* pPathSource, String& sPathResult, SeparatorType slashTypeToChangeFrom, SeparatorType slashTypeToChangeTo)
{
    char8_t theOldSeparator;
    char8_t theNewSeparator;

    const String sPathSource(pPathSource);
    sPathResult.clear();

    if(slashTypeToChangeFrom == kSeparatorTypeForward)
        theOldSeparator = '/';
    else if(slashTypeToChangeFrom == kSeparatorTypeBackward)
        theOldSeparator = '\\';
    else
        theOldSeparator = '/';

    if(slashTypeToChangeTo == kSeparatorTypeForward)
        theNewSeparator = '/';
    else if(slashTypeToChangeTo == kSeparatorTypeBackward)
        theNewSeparator = '\\';
    else
        theNewSeparator = '/';

    for(size_t i = 0, iEnd = sPathSource.length(); i < iEnd; i++)
    {
        if(sPathSource[i] == theOldSeparator)
            sPathResult += theNewSeparator;
        else
            sPathResult += sPathSource[i];
    }
}



///////////////////////////////////////////////////////////////////////////////
// ConvertPathToEncodedForm
//
// The URL:
//     "http://blah/test file.txt"
// becomes:
//     "http:/blah/test%20file.txt"
//
void URL::EncodeSpacesInURL(const char8_t* pURLSource, String& sURLResult)
{
    if(EA::StdC::Strchr(pURLSource, ' '))
    {
        URL url(pURLSource);

        String sPath = url.GetComponent(URL::kComponentAbsolutePath);
        for(size_t i; (i = sPath.find(" ")) != String::npos; )
            sPath.replace(i, 1, "%20");
        url.SetComponent(URL::kComponentAbsolutePath, sPath.c_str());

        url.MakeURLStringFromComponents(); // Currently a weakness in the URL class makes us call this before calling GetFullURL.
        sURLResult = url.GetFullURL();
    }
    else if(sURLResult.c_str() != pURLSource)
        sURLResult = pURLSource;
}


///////////////////////////////////////////////////////////////////////////////
// ConvertPathToEncodedForm
//
// See RFC 1738 for the standard encoding rules.
//
// The file:
//     "/blah/test file.txt"
// becomes:
//     "%2Fblah%2Ftest%20file.txt"
//
void URL::ConvertPathToEncodedForm(const char8_t* pPathSource, String& sPathResult, const char8_t* pCharsToNotEncode)
{
    // To do: Make this function more efficient by not using temp String objects here.
    const  String source(pPathSource);
    const  String sCharsToNotEncode(pCharsToNotEncode ? pCharsToNotEncode : "");
    const  String sSafeChars("$-_.+!*(),");            // These chars are always safe. Note that we intentionally leave out '\' from this list, 
                                                        // even though the URL standard suggests that it is a safe char. 
    sPathResult.clear(); // Do this assignment after pPathSource assignment above, as pPathSource may be the same as sPathResult.

    for(size_t i = 0, iEnd = source.length(); i < iEnd; i++)
    { 
        const char8_t c = source[i];

        if((IsValidASCII(c) && EA::StdC::Isalnum(c)) ||
            sSafeChars.find(c) < sSafeChars.length() ||
            (sCharsToNotEncode.find(c) < sCharsToNotEncode.length()))
        {
            sPathResult += source[i]; //Char is safe because it is alphanumeric, safe, or one of the not-to-encode chars.
        }
        else
        {
            sPathResult += '%';      
                                                // Basically, we do a quick binary to character hex conversion here.
            uint8_t nTemp = (uint8_t)c >> 4;    // Get first hex digit.
            nTemp = '0' + nTemp;                // Shift it to be starting at '0' instead of 0.
            if(nTemp > '9')                     // If digit needs to be in 'A' to 'F' range.
                nTemp += 7;                     // Shift it to the 'A' to 'F' range.
            sPathResult += (char8_t)nTemp;

            nTemp = (uint8_t)c % 16;
            nTemp = '0' + nTemp;
            if(nTemp > '9')
                nTemp += 7;
            sPathResult += (char8_t)nTemp;
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
// ConvertPathFromEncodedForm
//
// See RFC 1738 for the standard encoding rules.
//
// The file:
//     "%2Fblah%2Ftest%20file.txt"
// becomes:
//     "/blah/test file.txt"
//
void URL::ConvertPathFromEncodedForm(const char8_t* pPathSource, String& sPathResult)
{
    String sTempSource(pPathSource);

    sPathResult.clear(); // Do this after assignment from pPathSource, as pPathSource may point to sPathResult.

    for(size_t i = 0, iEnd = sTempSource.length(); i < iEnd; i++)
    {
        const char8_t c = sTempSource[i];

        if((c == '%') && ((i + 2) < iEnd))
        {
            char8_t pTemp[3];
            char8_t chResult;

            pTemp[0] = sTempSource[i + 1];
            pTemp[1] = sTempSource[i + 2];
            pTemp[2] = 0;

            if(IsValidASCII(pTemp[0]) && IsValidASCII(pTemp[1]) && EA::StdC::Isxdigit(pTemp[0]) && EA::StdC::Isxdigit(pTemp[1]))
                chResult = (char8_t) (EA::StdC::StrtoU32(pTemp, NULL, 16));
            else
                chResult = ' ';

            sPathResult += chResult;
            i += 2; // Move past the %XX portion.
        }
        else
            sPathResult += sTempSource[i];
    }
}


///////////////////////////////////////////////////////////////////////////////
// GetDefaultPortForProtocol
//
// See RFC 1700 for more standard ports for protocols.
//
uint32_t URL::GetDefaultPortForProtocol(URLType urlType)
{
    if(urlType == kURLTypeHTTP)
        return 80;      // 8080 is sometimes a secondary port.
    else if(urlType == kURLTypeHTTPS)
        return 443;     // Secure HTTP
    else if(urlType == kURLTypeFile)
        return 0;        // NA
    else if(urlType == kURLTypeFTP)
        return 21;      // This is the control port, not the data transfer port.
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// GetQueryValue
//
// Given a query component of the form "?KEY1=VALUE1[&KEY2=VALUE2...&KEYn=VALUEn]"
// returns the value associated with given key
//
bool URL::GetQueryValue(const char8_t* pQuery, const char8_t* pKey, String& sValueResult, bool bCaseSensitiveKey)
{
    if (pQuery && pKey)
    {
        String sQuery(pQuery);
        String sKey(pKey);
        sKey += "=";            // For cases where a substring matches in a partial parameter.  example:  ?aa=aa&a=a      
 
        if (!bCaseSensitiveKey)
        {
            sQuery.make_lower();
            sKey.make_lower();
        }
 
        // So that we don't mistakenly find a partial key
        // example finding key "a=" in the string "?aaa=value"
        // we'll first look for ?KEY= in position 1
        // then look for &KEY= in the remaining positions
        uint32_t nKeyStart;

        if (sQuery.compare(1, sKey.length(), sKey) == 0)
        {
            nKeyStart = 1;
        }
        else
        {
            sKey = "&" + sKey;
            nKeyStart = sQuery.find(sKey);

            if (nKeyStart >= sQuery.length())
                return false;
        }
 
        uint32_t nValueStart = nKeyStart + sKey.length();
        uint32_t nValueEnd   = sQuery.find("&", nValueStart);
 
        if (nValueEnd > sQuery.length())        // If this is the last key/value pair there will be no more '&'
            nValueEnd = sQuery.length();
 
        sValueResult.assign(pQuery+nValueStart, nValueEnd - nValueStart);
        return true;        
    }
 
    return false;
}


EAPATCHCLIENT_API String& StripFileNameFromURLPath(String& sURL)
{
    eastl_size_t urlFileNamePos = sURL.rfind('/'); // e.g. sURL = http://patch.ea.com:1234/Basebrawl_2014/Patches/Patch1.eaPatchImpl

    if(urlFileNamePos != String::npos)
        sURL.resize(urlFileNamePos + 1);           // e.g. sURL = http://patch.ea.com:1234/Basebrawl_2014/Patches/

    return sURL;
}



///////////////////////////////////////////////////////////////////////////////
// MakeHTTPURLFromComponents
// 
// A full HTTP URL takes the form:
//    http://<username>:<password>@<host>:<port>/<path>?<searchpart>#<fragment>
//
// This function assumes that the path you pass in has been converted to 
// proper URL form. Simply call 'ConvertPathFromLocalToURLForm' on the 
// path string to do this conversion for you.
//
EAPATCHCLIENT_API bool MakeHTTPURLFromComponents(String& sResult,
                                                const char8_t* pServer,
                                                uint32_t nPort,
                                                const char8_t* pPort,
                                                const char8_t* pPath,
                                                const char8_t* pParameters,
                                                const char8_t* pQuery,
                                                const char8_t* pFragment,
                                                const char8_t* pUsername,
                                                const char8_t* pPassword)
{
    sResult = "http://";

    if(pUsername && *pUsername)
    {
        sResult += pUsername;
        sResult += ':';

        if(pPassword && *pPassword)
            sResult += pPassword;
        sResult += '@';
    }

    sResult += pServer;

    if(pPort && *pPort)
    {
        if(*pPort != ':')
            sResult += ':';
        sResult += pPort;
        nPort = 0; // Ignore the nPort value, as a port string overrides the port value.
    }

    if(nPort)
    {
        sResult += ':';
        char8_t buffer[20];
        sResult += EA::StdC::U32toa(nPort, buffer, 10);
    }

    if(pPath && *pPath)
        sResult += pPath;

    if(pParameters && *pParameters)
    {
        if(*pParameters != ';')
            sResult += ';';
        sResult += pParameters;
    }

    if(pQuery && *pQuery)
    {
        if(*pParameters != '?')
            sResult += '?';
        sResult += pQuery;
    }

    if(pFragment && *pFragment)
    {
        if(*pParameters != '#')
            sResult += '#';
        sResult += pFragment;
    }

    return true;
}



} // namespace Internet

} // namespace EA


















