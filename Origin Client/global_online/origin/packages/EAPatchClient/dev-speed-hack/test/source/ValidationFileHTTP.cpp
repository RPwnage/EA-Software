/////////////////////////////////////////////////////////////////////////////
// ValidationFileHTTP.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAIO/EAFileStream.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAStreamAdapter.h>
#include <EAIO/EAStreamBuffer.h>
#include <EAIO/PathString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EAEndian.h>
#include <EAPatchClientTest/ValidationFile.h>
#include <EAPatchClientTest/ValidationFileHTTP.h>
#include <UTFInternet/Allocator.h>


///////////////////////////////////////////////////////////////////////////////
// HTTPDataSourceFakeValidationFile
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
namespace Internet
{

IHTTPServerDataSource* FakeValidationFileFactory(const char8_t* pURLPath, void* /*pContext*/)
{
    EA::Allocator::ICoreAllocator* const pAllocator = EA::Internet::GetAllocator();
	HTTPDataSourceFakeValidationFile* pDataSource = UTFINTERNET_CA_NEW(HTTPDataSourceFakeValidationFile, pAllocator, UTFINTERNET_ALLOC_PREFIX "HTTPDataSourceFakeValidationFile");

	return pDataSource;
}


HTTPDataSourceFakeValidationFile::HTTPDataSourceFakeValidationFile()
  : mnFileSize(0),
    mnFileReadBegin(0),
    mnFileReadEnd(0),
    mbContentRange(false)
{
}


///////////////////////////////////////////////////////////////////////////////
// CanServeURLStatic
//
// We serve all files with the extension .fakeValidationFile.
//
int HTTPDataSourceFakeValidationFile::CanServeURLStatic(HTTPServer* pHTTPServer, HTTPServerJob* /*pHTTPJob*/, const URL& url, HTTPMethod httpMethod)
{
    if((httpMethod == kHTTPMethodGet) || (httpMethod == kHTTPMethodHead))
    {
        StringW sFilePath;

        if(pHTTPServer->GetFilePathFromURL(url, sFilePath) && !sFilePath.empty())
        {
            const wchar_t*    pExtensionW = EA::IO::Path::GetFileExtension(sFilePath.c_str());
            EA::Patch::String sExtension8 = EA::StdC::Strlcpy<EA::Patch::String, wchar_t>(pExtensionW); // Convert to char8_t.

            if(EA::StdC::Stricmp(sExtension8.c_str(), kFakeValidationFileExtension) == 0)
                return kHTTPStatusCodeOK;
        }

        return kHTTPStatusCodeNotFound;
    }

    return kHTTPStatusCodeMethodNotAllowed;
}


///////////////////////////////////////////////////////////////////////////////
// CanServeURL
//
int HTTPDataSourceFakeValidationFile::CanServeURL(HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url, HTTPMethod httpMethod)
{
    return CanServeURLStatic(pHTTPServer, pHTTPJob, url, httpMethod);
}


///////////////////////////////////////////////////////////////////////////////
// GetReadSize
//
IO::size_type HTTPDataSourceFakeValidationFile::GetReadSize(HTTPServer* /*pHTTPServer*/, HTTPServerJob* /*pHTTPJob*/, const URL& /*url*/)
{
    // This code depends on size_type being a 64 bit type.
    return (IO::size_type)(mnFileReadEnd - mnFileReadBegin);
}


///////////////////////////////////////////////////////////////////////////////
// ReadBegin
//
// Range requests:
//     http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.35.2
//     Example: 
//         Range: bytes=21010-47021
//     Example: 
//         Range: bytes=21884-
//
// Content-Range responses:
//     http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.16
//     Example: 
//         HTTP/1.1 206 Partial content
//         Content-Range: bytes 21010-47021/47022
//         Content-Length: 26012
//
//     If the server receives a request (other than one including an If- Range request-header field) 
//     with an unsatisfiable Range request- header field (that is, all of whose byte-range-spec 
//     values have a first-byte-pos value greater than the current length of the selected resource), 
//     it SHOULD return a response code of 416 (Requested range not satisfiable) (section 10.4.17). 
//
uint32_t HTTPDataSourceFakeValidationFile::ReadBegin(HTTPServer* pHTTPServer, HTTPServerJob* pHTTPJob, const URL& url, HTTPMethod)
{
    uint32_t returnValue = 0;
    StringW  sFilePath;

    mnFileReadBegin = 0;
    mnFileReadEnd   = 0;
    mbContentRange  = false; // May be set to true below.

    if(pHTTPServer->GetFilePathFromURL(url, sFilePath))
    {
        returnValue = kHTTPStatusCodeOK; // OK until proven otherwise.

        // If there there is a number in the file name following an underscore, use it as the size.
        // For example: /some_dir/some_234_dir/some_file_2000.fakeValidationFile has a size of 2000.
        // For example: /some_dir/somefile2000.fakeValidationFile has no specified size.
        eastl_size_t dirPos        = sFilePath.find_last_of(L"/\\");
        eastl_size_t underScorePos = sFilePath.rfind(L'_');

        if((underScorePos != StringW::npos) && ((dirPos == StringW::npos) || (dirPos < underScorePos)))
            mnFileSize = EA::StdC::StrtoU64(&sFilePath[underScorePos + 1], NULL, 0);
        else
            mnFileSize = 16386; // This is the default size. If the file name ends with a number, then we use that for the size.

        // We don't need to write the Content-Length header, as HTTPServer does that for us.
        // If the Range header is present then we need to validate the range and write 
        // the Content-Range header, else return kHTTPStatusCodeRequestRangeNotSatisfiable.
        const StringArray& stringArray  = pHTTPJob->GetReceivedHeaderStringArray();
        const char8_t*     pRangeHeader = GetHeaderEntry(stringArray, "range", 0);

		// Attempt to extract HTTP range values which are inclusive.
		uint64_t rangeFirst;    
		uint64_t rangeLast;
		returnValue = HTTPDataSourceBase::ExtractRange(pRangeHeader, mnFileSize, &rangeFirst, &rangeLast);

		// Determine what chunk of the data we want to configure for transfer.
		if(returnValue == kHTTPStatusCodePartialContent)
		{
			mnFileReadBegin = rangeFirst;
			mnFileReadEnd   = rangeLast + 1; // +1 because HTTP range values are inclusive.
			mbContentRange  = true;

			// We'll need to set some custom headers like the following, but we do that in 
			// our GetCustomHeaderLines function, which the server will call next.
			// Content-Range: bytes 21010-47021/47022
			// Content-Length: 26012
		}
		else if(returnValue == kHTTPStatusCodeOK)
			mnFileReadEnd = mnFileSize;
    }

    return returnValue;
}


///////////////////////////////////////////////////////////////////////////////
// GetCustomHeaderLines
//
uint32_t HTTPDataSourceFakeValidationFile::GetCustomHeaderLines(HTTPServer* pHTTPServer, HTTPServerJob* pJob, const URL& url, StringArray& headerLineArray)
{
    uint32_t headerCount = 0;

	headerLineArray.push_back();
	headerLineArray[headerCount++] = "Content-type: application/octet-stream\r\n";

    if(mbContentRange)
    {
        // Write something like this:
        // Content-Range: bytes 21010-47021/47022
        // Content-Length: 26012

        headerLineArray.push_back();
        headerLineArray[headerCount++].sprintf("Content-Range: bytes %I64u-%I64u/%I64u\r\n", mnFileReadBegin, mnFileReadEnd - 1, mnFileSize);

        headerLineArray.push_back(); // The server should already do this for us if we don't do it here, but let's make sure it gets done.
        headerLineArray[headerCount++].sprintf("Content-Length: %I64u\r\n", (mnFileReadEnd - mnFileReadBegin));
    }

    headerCount += HTTPDataSourceBase::GetCustomHeaderLines(pHTTPServer, pJob, url, headerLineArray);

    return headerCount;
}


///////////////////////////////////////////////////////////////////////////////
// Read
//
IO::size_type HTTPDataSourceFakeValidationFile::Read(HTTPServer* /*pHTTPServer*/, HTTPServerJob* /*pHTTPJob*/,
                                                      const URL& /*url*/, void* pData, IO::size_type nSize)
{
    // To do: Use the FakeValidationFile class instead of this, now that we have that available.
    uint64_t nSize64 = eastl::min_alt(mnFileReadEnd - mnFileReadBegin, (uint64_t)nSize);
    uint8_t* pData8  = static_cast<uint8_t*>(pData);

    for(uint64_t i = 0, f = mnFileReadBegin; i < nSize64; f++, i++)
        pData8[i] = (uint8_t)(f % kValidationFilePrimeNumber);
    
    if(mnFileReadBegin < sizeof(uint64_t)) // The first 8 bytes of a ValidationFile is a uint64_t: the file size in little-endian.
    {
        union FileSizeBytes {
            uint64_t mFileSize;
            uint8_t  mFileSize8[8];
        } fsb = { EA::StdC::ToLittleEndian(mnFileSize) };

        for(uint64_t i = 0, f = mnFileReadBegin, fEnd = eastl::min_alt(mnFileReadEnd, (uint64_t)sizeof(uint64_t)); (f < fEnd) && (i < nSize64); f++, i++)
            pData8[i] = fsb.mFileSize8[f];
    }

    mnFileReadBegin += nSize64;

    return (IO::size_type)nSize64;
}


///////////////////////////////////////////////////////////////////////////////
// ReadEnd
//
void HTTPDataSourceFakeValidationFile::ReadEnd(HTTPServer* /*pHTTPServer*/, HTTPServerJob* /*pHTTPJob*/, const URL& /*url*/)
{
    // Nothing to do.
}

} // namespace Internet
} // namespace EA








