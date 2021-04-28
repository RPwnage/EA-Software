///////////////////////////////////////////////////////////////////////////////
// URL.h
//
// This file is a copy of UTFInternet/URL.h with mostly only "Internet" -> "EAPatch" 
// name changes, and exists here to avoid adding a dependency on the 
// UTFInternet package.
//
// Copyright (c) 2004 Electronic Arts Inc.
// Written and maintained by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_URL_H
#define EAPATCHCLIENT_URL_H


#include <EAPatchClient/Base.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
    namespace Patch
    {
        ///////////////////////////////////////////////////////////////////////////////
        // URL
        //
        // This class encapsulates a Universal Resource Location (URL), defined in 
        // RFC 1738 and RFC 1808, with specific applications to HTTP in RFC 2068.
        // Please read these documents fully before embarking on seriously modifying
        // this code. 
        //
        // A URL is a platform-independent way to describe a location of an entity
        // which is usually a file. Please keep the standard in mind when modifying 
        // this class and remember that this code needs to work on platforms beyond
        // just Windows.
        //
        // Here are some simple example URLs:
        //   1 "http://www.maxis.com:80/Blah/SimCity.html"
        //   2 "file:///C|/Blah/Busy Overpass.jpg"
        //   3 "file://C:/Blah/Busy%20Overpass.jpg"
        //   4 "C:\Blah\Busy%20Overpass.jpg"
        //   5 "ftp://ftp.maxis.com/Blah/"
        //   6 "ftp://ftp.maxis.com:21/Blah/Blah.txt"
        //   7 "irc://irc.maxis.com:6667"
        //   8 "news://news.maxis.com"
        //   9 "nntp://news.maxis.com:119/<newsgroup-name>/<article-number>
        //  10 "mailto:joe@bandit.com"
        //
        // An HTTP URL takes the form:
        //    http://<username>:<password>@<host>:<port>/<path>;<parameters>?<searchpart>#<fragment>
        //
        // An ftp URL takes the form:
        //    ftp://<username>:<password>@<host>:<port>/<path>;type=<typecode>
        //
        // A file URL takes the form:
        //    file://<host>/<path>
        //
        class EAPATCHCLIENT_API URL
        {
            public:
                enum URLType
                {
                    kURLTypeUndefined,  // The URL is not yet defined or hasn't been parsed yet.
                    kURLTypeUnknown,    // The URL is of a type that is not in our enumerated list. Different from kURLTypeUndefined.
                    kURLTypeHTTP,       // The URL is for HTTP protocol (RFC 2068)
                    kURLTypeHTTPS,      // The URL is for secure HTTP protocol (RFC 2660, 2818).
                    kURLTypeFile,       // The URL is a path to a local file
                    kURLTypeFTP,        // The URL is for FTP protocol (RFC 414, and others)
                    kURLTypeMailTo,     // The URL is for mail (RFC 822)
                    kURLTypeRelative    // The URL appears to be a relative URL (RFC 1808)
                };

                enum Component
                {
                    kComponentScheme,       /// e.g. "http" or "ftp".
                    kComponentUsername,     /// e.g. "someuser". May be empty. 
                    kComponentPassword,     /// e.g. "mypassword". May be empty.
                    kComponentServer,       /// e.g. "www.ea.com" or "ftp.ea.com". Also known as host. 
                    kComponentPort,         /// e.g. "80". 0 means use default. We always work with ports in local endian form, despite that some sockets APIs work with them in "network order".
                    kComponentAbsolutePath, /// e.g. "/somedir/index.html". Path to resource. If URL is kURLTypeFile or kURLTypeFTP, this holds the full path to the file or full directory.
                    kComponentParameters,   /// e.g. ";blah". Begins with ';'. FTP uses this to hold commands known as 'type data'.
                    kComponentQuery,        /// e.g. "?button1=result1;button2=result2". Begins with '?'. HTTP uses this to mark form data.
                    kComponentFragment,     /// e.g. "#Bananas". Begins with '#'. HTTP uses this to reference a position within an HTML file.
                    kComponentCount         /// Count of component types.
                };

                enum SeparatorType
                {
                    kSeparatorTypeForward  = '/',   // Used by Unix as a directoy delimiter.
                    kSeparatorTypeBackward = '\\'   // Used by Windows as a directory delimiter.
                };

                /// Options
                /// Allows the user to control the way the class works.
                enum Options
                {
                    kOptionNone,                /// No option in particular.
                    kOptionStrictConformance,   /// Default is enabled. You want strict conformance when interpreting URLs within HTML, but non-strict when interpreting user-input URLs, since they may have slop.
                    kOptionMaxComponentLength   /// Default is 128. The max length of any given component. A form of security attack is to give very large URLs to applications.
                };

            public:
                /// URL
                /// Constructs an empty URL. All parameters are empty.
                URL();

                /// URL
                /// Constructs a URL from a string.
                URL(const char8_t* pURL);

                /// URL
                /// Constructs a URL from another URL.
                URL(const URL& url);

                /// URL
                /// Assigns a URL from another URL.
                URL& operator=(const URL& url);

            public:
                /// GetOption
                /// Gets the given option value.
                int GetOption(Options option) const;

                /// SetOption
                /// Sets options of enum Options.
                void SetOption(Options option, int value);

                /// IsURLRelative
                /// Returns true if the given URL is relative, else it is absolute.
                bool IsURLRelative() const;

                /// GetURLType
                URLType GetURLType() const;

                /// SetURLType
                void SetURLType(URLType urlType);

                /// GetFullURL
                /// Simply returns the current URL, unparsed.
                const String& GetFullURL() const;

                /// SetFullURL
                /// There may very well be situations where we *don't* want to reparse.
                void SetFullURL(const char8_t* pFullURL, bool bShouldReparse = true);

                /// GetComponent
                /// Returns a string of the given component.
                const String& GetComponent(Component component) const;

                /// SetComponent
                /// Sets the string for the given component.
                void SetComponent(Component component, const char8_t* pComponent);

                /// GetPort
                /// Returns the port of the URL. If the port is absent, then the default
                /// for the scheme is used. If the scheme is unknown, the return value 
                /// is zero. 
                /// The port is specified in local byte ordering. If you don't know that 
                /// that means, pretend you didn't read it. 
                /// Example port: 80 (HTTP)
                uint32_t GetPort() const;

                /// SetPort
                /// Sets the port.
                void SetPort(uint32_t nPort);

                /// ChangeURL
                /// Change self from current value to new. Uses base URL (if present) to 
                /// resolve relative paths.
                bool ChangeURL(const char8_t* pNewURL, const char8_t* pBaseURL = NULL);

                /// ChangeURL
                /// Change self to new value. Possibly pass in base value as well, 
                /// since some URLs are relative paths. The pBaseURL argument is 
                /// not const because reading from a URL object may change it due
                /// to it doing lazy component building.
                bool ChangeURL(const char8_t* pNewURL, URL* pBaseURL = NULL);

            public: // static helper functions

                /// IsURLRelative
                /// Tells you if the URL string is a relative URL, else absolute.
                static bool IsURLRelative(const char8_t* pURL);

                /// CanonicalizePath
                /// Given an absolute path, this function converts any "/.."  and '/." sequences within it appropriately.
                /// The source and destination paths may refer to the same memory.
                static bool CanonicalizePath(const char8_t* pPathSource, String& sPathResult);

                /// MakeRelativeURLFromBaseAndTarget
                /// Makes a relative URL from a base URL and the target URL, putting the 
                /// newly made relative URL into sResult. sResult can be combined with 
                /// the base to yield the original target.
                static bool MakeRelativeURLFromBaseAndTarget(const char8_t* pBase, const char8_t* pTarget, String& sResult);

                /// GetDefaultPortForProtocol
                /// Returns port number in local byte ordering. If you are using the returned value
                /// for calls to socket API functions, you may need to convert it to network byte
                /// ordering, depending on the API.
                static uint32_t GetDefaultPortForProtocol(URLType urlType);

                /// ChangePathSeparators
                /// Changes separators from one type to the other.
                /// The source and result paths may refer to the same memory.
                static void ChangePathSeparators(const char8_t* pPathSource, String& sPathResult, 
                                                 SeparatorType separatorTypeToChangeFrom, SeparatorType separatorTypeToChangeTo);

                /// EncodeSpacesInURL
                /// Takes URL that may have spaces in its path and encodes them as %20.
                /// Source and dest can be same string. 
                static void EncodeSpacesInURL(const char8_t* pURLSource, String& sURLResult);

                /// ConvertPathToEncodedForm
                /// Converts spaces and other unsafe chars to "%20", etc. Source and dest 
                /// can be same string. If 'pCharsToNotEncode' is used, it allows you to 
                /// specify additional chars to not encode, in addition to the usual safe ones.
                /// This applies to storing a file path in a URL as an argument, and doesn't apply to the URL path itself.
                static void ConvertPathToEncodedForm(const char8_t* pPathSource, String& sPathResult, const char8_t* pCharsToNotEncode = NULL);

                /// ConvertPathFromEncodedForm
                /// Converts %20 to spaces, etc. Source and result can be same string.
                /// This applies to storing a file path in a URL as an argument, and doesn't apply to the URL path itself.
                static void ConvertPathFromEncodedForm(const char8_t* pPathSource, String& sPathResult);

                /// ConvertPathFromURLToLocalForm
                /// Given a file absolute path in proper URL form, this function converts 
                /// it to a form that is usable on the current machine. Source and result 
                /// can be same string.
                static void ConvertPathFromURLToLocalForm(const char8_t* pPathSource, String& sPathResult);

                /// ConvertPathFromLocalToURLForm
                /// Given a file absolute path in local form, this function converts it to
                /// a URL-proper form. Source and result can be same string.
                static void ConvertPathFromLocalToURLForm(const char8_t* pPathSource, String& sPathResult);

                /// MakeURLStringFromComponents
                /// Makes a string from member components.
                bool MakeURLStringFromComponents();

                /// GetQueryValue
                /// Given a query component of the form "?KEY1=VALUE1[&KEY2=VALUE2...&KEYn=VALUEn]"
                /// returns the value associated with given key
                static bool GetQueryValue(const char8_t* pQuery, const char8_t* pKey, String& sValueResult, bool bCaseSensitiveKey = false);

            protected:
                /// ParseURL
                /// Parses argument URL into member sub-parts. 
                /// Does not modify msURL, but does modify other members.
                bool ParseURL(String& sURL);

                // The following are used internally to progressively parse a URL string.
                bool ParseURLConst           (String& sURL) const;
                bool ParseScheme             (String& sURL);
                bool ParseUsernameAndPassword(String& sURL);
                bool ParseServerAndPort      (String& sURL);
                bool ParseURLHTTP            (String& sURL);
                bool ParseURLFILE            (String& sURL);
                bool ParseURLFTP             (String& sURL);
                bool ParseURLMailTo          (String& sURL);
                bool ParseURLUnknown         (String& sURL);
                bool ParseURLRelative        (String& sURL);

            protected:
                // To consider: Convert any of the strings below to char arrays where feasible.
                bool     mbStrictConformance;           /// Whether or not we require strict conformance to the URL standard.
                int      mnMaxComponentLength;          /// Max string length of any component.
                bool     mbParsed;                      /// We delay parsing until the URL parsed values are actually needed.
                URLType  mURLType;                      /// The type, if we know it.
                String   msURL;                         /// The full URL. Example: "http://someuser:mypassword@www.maxis.com:80/SimCity/SimCity.html"
                String   msComponent[kComponentCount];  /// 

        }; // class URL


        /// StripFileNameFromURLPath
        ///
        /// Example:
        ///     http://someuser:mypassword@www.maxis.com:80/SimCity/SimCity.html
        ///       ->
        ///     http://someuser:mypassword@www.maxis.com:80/SimCity/
        ///
        EAPATCHCLIENT_API String& StripFileNameFromURLPath(String& sURL);


        /// MakeHTTPURLFromComponents
        /// If 'pPort' is present, it is used instead of nPort.
        /// if nPort is zero, it is not used.
        EAPATCHCLIENT_API bool MakeHTTPURLFromComponents(String& sResult,
                                                       const char8_t* pServer,
                                                       uint32_t nPort = 0,
                                                       const char8_t* pPort = NULL,
                                                       const char8_t* pPath = NULL,
                                                       const char8_t* pParameters = NULL,
                                                       const char8_t* pQuery = NULL,
                                                       const char8_t* pFragment = NULL,
                                                       const char8_t* pUsername = NULL,
                                                       const char8_t* pPassword = NULL);

    } // namespace Patch

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif



#endif  // Header include guard
