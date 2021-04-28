/******************************************************************************/
/*!
    curlutil
    Copyright 2011 Electronic Arts Inc.

    \file       curlutil.h
    \ingroup    nflfantasy
    \brief      Helper class to pull files from SFTP server
 */
/******************************************************************************/

/*** Include guard ************************************************************/

#ifndef BLAZE_ARSON_CURLUTIL_H
#define BLAZE_ARSON_CURLUTIL_H

/*** Includes *****************************************************************/

#include "arson/tdf/arson.h"

/*** Macros *******************************************************************/

/*** Interface ****************************************************************/

// --- Constants

// --- Forward declarations
typedef void CURL;

namespace Blaze
{

namespace Arson
{

// --- More Constants

/*** Implementation ***********************************************************/

class CurlUtil
{
public:
    CurlUtil(const Blaze::Arson::SftpConfig& config);
    ~CurlUtil();

    bool PullXmlFiles(const char* destDir, bool deletePulledFiles, Blaze::Arson::FtpResponseList& ftpResponseList);

private:

    // This structure is used to detect if the FTP-callback is for
    // downloading a file or non-file. We ignore non-file types (like directories)
    // and only fetch the files.
    struct FtpCallbackData
    {
      FILE*            output;
      CurlUtil*        curlUtil;
      Blaze::Arson::FtpResponseList* ftpResponseList;
    };

    struct FileDownloadDetails
    {
        eastl::string mFilename;
        size_t mFilesize;
    };
    typedef eastl::list<FileDownloadDetails*> FileDownloadDetailsList;

    // methods for getting notifications via FTP
    void RecordDownloadedFile(const char* filename, size_t size);
    int PullFiles(const char* destDir, Blaze::Arson::FtpResponseList& ftpResponseList);
    int DeleteFiles(Blaze::Arson::FtpResponseList& ftpResponseList);
    int DiscoverRemoteFiles(Blaze::Arson::FtpResponseList& ftpResponseList);

    int  CreateCurlFtpHandle();
    void CloseCurlFtpHandle();


    // these are all curl callback methods
    static size_t WriteFile(char* buff, size_t size, size_t nmemb,
                            FtpCallbackData* data);
    static size_t WriteHeaders(char* buff, size_t size, size_t nmemb,
                               FtpCallbackData* data);
    static size_t WriteNothing(char* buff, size_t size, size_t nmemb,
                               FtpCallbackData* data);
    static size_t WriteDebug(CURL* handle, int infoType,
                             char* buff, size_t size,
                             FtpCallbackData* data);
    static size_t DiscoverFilenames(char* buff, size_t size, size_t nmemb,
                                    FtpCallbackData* data);

    // HACK: EASTL does not trim strings correctly!
    static void TrimString(eastl::string& trimMe);

    bool mConfigLoaded;
    bool mFtpIsVerbose;
    bool mFtpThrottlingEnabled;
    uint32_t mFtpMaxFilesPerInterval;
    const char8_t* mFtpHostname;
    const char8_t* mFtpPassword;
    const char8_t* mFtpUsername;
    const char8_t* mFtpRemotePath;
    const char8_t* mFtpSshPrivateKeyfile;
    const char8_t* mFtpSshPublicKeyfile;

    FileDownloadDetailsList mFileDownloadDetailsList;
    CURL* mHandle;
};

} // Arson
} // Blaze
#endif // BLAZE_ARSON_CURLUTIL_H
