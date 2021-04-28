/////////////////////////////////////////////////////////////////////////////
// ValidationFileHTTP.h
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////



#ifndef EAPATCH_VALIDATIONFILEHTTP_H
#define EAPATCH_VALIDATIONFILEHTTP_H


#include <EAPatchClient/Base.h>
#include <EAIO/EAFileStream.h>
#include <UTFInternet/HTTPServer.h>
#include <UTFInternet/HTTPServerPlugin.h>


namespace EA
{
    namespace Internet
    {

        ///////////////////////////////////////////////////////////////////////////////
        /// HTTPDataSourceFakeValidationFile
        ///
        /// Implements 'reading' from a virtual (non-existent) ValidationFile.
        /// A ValidationFile is a file whose contents are rigged so that you can verify
        /// each byte's veracity simply by its position within the file. It's useful 
        /// for verifying that file copies (including partial copies) succeeded.
        /// The benefit of having this class is that it can be used to test transfer and
        /// storage of downloaded files on the client side, including huge files, without
        /// consuming disk space for the files on the server.
        ///
        /// This class fields requestes for file paths whose file name are of the form:
        ///     <name>_<size>.fakeValidationFile
        /// And returns to the HTTP client a ValidationFile of the given size. The name
        /// portion is ignored and can be anything useful. For example, an HTTP client
        /// asks for http://127.0.0.1/SomeDir/SomeFile_23000.fakeValidationFile, and this
        /// class tells HTTP server that it knows how to serve this file, and it proceeds
        /// to return to the HTTP client a 23000 byte file in the format of ValidationFile.
        ///
        class HTTPDataSourceFakeValidationFile : public HTTPDataSourceBase
        {
        public:
            static const InterfaceId kIID =  InterfaceId(0x0e36bd43);

            HTTPDataSourceFakeValidationFile();

            /// Verifies that the path specified by the URL exists in an accessible
            /// place in the file system. If you override this class, you may need
            /// to override the CanServeURL function.
            static int CanServeURLStatic(HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url, HTTPMethod httpMethod);

            int               CanServeURL         (HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url, HTTPMethod httpMethod);
            EA::IO::size_type GetReadSize         (HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url);
            uint32_t          GetCustomHeaderLines(HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL&, StringArray& headerLineArray);
            uint32_t          ReadBegin           (HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url, HTTPMethod);
            EA::IO::size_type Read                (HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url, void* pData, EA::IO::size_type nSize);
            void              ReadEnd             (HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url);

        protected:
            // To do: Use FakeValidationFile instead of the three variables below, now that we have the FakeValidationFile class.
            uint64_t mnFileSize;         /// The size of the file upon the call to ReadBegin.
            uint64_t mnFileReadBegin;    /// The begin of our read from mFileStream.
            uint64_t mnFileReadEnd;      /// The end of our read from mFileStream. This is one-past the last read byte index, and (mnFileReadEnd - mnFileReadBegin) == read size.
            bool     mbContentRange;     /// True if the user asked for a content range instead of the entire file.
        };

        /// Creates an HTTPDataSourceFakeValidationFile.
        /// Used by HTTPServer for implementing plugins, so we use it for unit testing.
        IHTTPServerDataSource* FakeValidationFileFactory(const char8_t* pURLPath, void* /*pContext*/);

    } // namespace Internet

} // namespace EA


#endif // Header include guard
