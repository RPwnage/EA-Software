/******************************************************************************/
/*!
     \class curlutil

     \brief  CurlUtil helper class.

     [Put your optional additional description here.]
 */
/******************************************************************************/

/*** Includes *****************************************************************/

// main include
#include "framework/blaze.h"
#include "./curlutil.h"

// curl includes
#include <stdio.h>
#include <curl/curl.h>
#include <curl/easy.h>


/*** Using ********************************************************************/
namespace Blaze
{
namespace Arson
{

/*** Macros *******************************************************************/

/*** Interface ****************************************************************/

/*** Variables ****************************************************************/

/*** Implementation ***********************************************************/


/******************************************************************************/
/*! CurlUtil::CurlUtil

     \brief The constructor.
 */
/******************************************************************************/
CurlUtil::CurlUtil(const Blaze::Arson::SftpConfig& config) :
    mConfigLoaded(false),
    mFtpIsVerbose(false),
    mFtpThrottlingEnabled(false),
    mFtpMaxFilesPerInterval(0),
    mFtpHostname(nullptr),
    mFtpPassword(nullptr),
    mFtpUsername(nullptr),
    mFtpRemotePath(nullptr),
    mFtpSshPrivateKeyfile(nullptr),
    mFtpSshPublicKeyfile(nullptr),
    mHandle(nullptr)
{
    mFtpIsVerbose = config.getFtpIsVerbose();
    mFtpThrottlingEnabled = config.getFtpThrottlingEnabled();
    mFtpMaxFilesPerInterval = config.getFtpMaxFilesPerInterval();
    mFtpHostname = config.getFtpHostname();
    mFtpPassword = config.getFtpPassword();
    mFtpUsername = config.getFtpUsername();
    mFtpRemotePath = config.getFtpRemotePath();
    mConfigLoaded = true;
}


/******************************************************************************/
/*! CurlUtil::~CurlUtil

     \brief The destructor.

 */
/******************************************************************************/
CurlUtil::~CurlUtil()
{
    FileDownloadDetailsList::iterator i = mFileDownloadDetailsList.begin();
    FileDownloadDetailsList::const_iterator e = mFileDownloadDetailsList.end();

    for(; i!=e; ++i)
    {
        FileDownloadDetails* downloadDetails = *i;
        delete downloadDetails;
        downloadDetails = nullptr;
    }

    mFileDownloadDetailsList.clear();
}

/******************************************************************************/
/*! CurlUtil::TrimString

     \brief      Trim an eastl string and remove the '\n\ character

     \param      trimMe    - The string to trim.
 */
/******************************************************************************/
void CurlUtil::TrimString(eastl::string& trimMe)
{
    // default eastl trim does not remove \n.
    const char array[] = { ' ', '\t', '\n', 0 }; // This is a pretty simplistic view of whitespace.
    trimMe.erase(0, trimMe.find_first_not_of(array));
    trimMe.erase(trimMe.find_last_not_of(array) + 1);
}

/******************************************************************************/
/*! CurlUtil::WriteFile

     \brief      Writes out a chunk of bytes for the current file being downloaded.

     \param      buff    - A pointer to a buffer with a set of data to write.
     \param      size    - The size of each element in the buffer (number of characters).
     \param      nmemb   - The number of elements in the buffer. (size * nmember = bytes).
     \param      data    - The user provided data we added for context.
     \return             - The number of bytes written out.
 */
/******************************************************************************/
size_t CurlUtil::WriteFile(char* buff, size_t size, size_t nmemb,
                           FtpCallbackData* data)
{
    size_t written = 0;
    if(data->output)
    {
        written = fwrite(buff, size, nmemb, data->output);
    }
    else
    {
        // listing output or response output
        // write into response list
        // note: I expect this to never be used.
        size_t offset = size * nmemb;
        eastl::string response(buff, buff + offset);
        TrimString(response);
        WARN("[CurlDebug]:WriteFile. This should not get called. %s", response.c_str());
        data->ftpResponseList->push_back(response.c_str());
        written =  offset;
    }
    return written;
}


/******************************************************************************/
/*! CurlUtil::WriteHeaders

     \brief      Writes out a chunk of bytes for a line of response from the FTP server.

     \param      buff    - A pointer to a buffer with a set of data to write.
     \param      size    - The size of each element in the buffer (number of characters).
     \param      nmember - The number of elements in the buffer. (size * nmember = bytes).
     \param      data    - The user provided data we added for context.
     \return             - The number of bytes written out.
 */
/******************************************************************************/
size_t CurlUtil::WriteHeaders(char* buff, size_t size, size_t nmemb,
                              FtpCallbackData* data)
{
    // listing output or response output
    // write into response list
    size_t offset = size * nmemb;
    eastl::string response(buff, buff + offset);
    TrimString(response);
    BLAZE_TRACE_LOG(Log::HTTP, "[CurlDebug]:WriteHeaders: " << response.c_str());
    data->ftpResponseList->push_back(response.c_str());
    return offset;
}

/******************************************************************************/
/*! CurlUtil::WriteHeaders

     \brief      Write nothing but report success.

     \param      buff    - A pointer to a buffer with a set of data to write.
     \param      size    - The size of each element in the buffer (number of characters).
     \param      nmember - The number of elements in the buffer. (size * nmember = bytes).
     \param      data    - The user provided data we added for context.
     \return             - The number of bytes written out.
 */
/******************************************************************************/
size_t CurlUtil::WriteNothing(char* buff, size_t size, size_t nmemb,
                              FtpCallbackData* data)
{
    return size * nmemb;
}


/******************************************************************************/
/*! CurlUtil::WriteDebug

     \brief      Writes out details about the FTP transfer to the Blaze debug log.

     \param      handle   - The active curl handle.
     \param      infoType - The origin of the message.
     \param      buff     - A pointer to a buffer with a set of data to write.
     \param      size     - The number of bytes of valid data in buff.
     \param      data     - The user provided data we added for context.
     \return              - The number of bytes written out.
 */
/******************************************************************************/
size_t  CurlUtil::WriteDebug(CURL* handle, int infoType,
                             char* buff, size_t size,
                             FtpCallbackData* data)
{
    switch(infoType)
    {
        case CURLINFO_TEXT:
            {
                eastl::string message(buff, buff + size);
                BLAZE_TRACE_LOG(Log::HTTP, "[CurlDebug]:Info: " <<  message.c_str());
                break;
            }
        case CURLINFO_HEADER_IN:
            {
                eastl::string message(buff, buff + size);
                BLAZE_TRACE_LOG(Log::HTTP, "[CurlDebug]:Header from peer: " << message.c_str());
                break;
            }
        case CURLINFO_HEADER_OUT:
            {
                eastl::string message(buff, buff + size);
                BLAZE_TRACE_LOG(Log::HTTP, "[CurlDebug]:Header to peer: " << message.c_str());
                break;
            }
            // actual upload/download payload. don't log this.
        case CURLINFO_DATA_IN:
        case CURLINFO_DATA_OUT:
        case CURLINFO_SSL_DATA_IN:
        case CURLINFO_SSL_DATA_OUT:
            break;
        default:
            {
                eastl::string message(buff, buff + size);
                BLAZE_TRACE_LOG(Log::HTTP, "[CurlDebug]:Unknown  : " << message.c_str());
                break;
            }
    }
    return 0;
}

/******************************************************************************/
/*! CurlUtil::DiscoverFilenames

     \brief      Discovers the files in the FTP directory and records them so we can pull
                 the files with a future method call.

     \param      buff    - A pointer to a buffer with a set of data to write.
     \param      size    - The size of each element in the buffer (number of characters).
     \param      nmember - The number of elements in the buffer. (size * nmember = bytes).
     \param      data    - The user provided data we added for context.
     \return             - The number of bytes written out.
 */
/******************************************************************************/
size_t CurlUtil::DiscoverFilenames(char* buff, size_t size, size_t nmemb,
                                   FtpCallbackData* data)
{
    // listing output or response output
    // write into response list
    size_t offset = size * nmemb;

    eastl::string filename(buff, buff + offset);

    // must trim whitespace or the fopen will fail!!!
    TrimString(filename);

    // Only download .xml files
    if (filename.right(4).compare(".xml") == 0)
    {
        data->curlUtil->RecordDownloadedFile(filename.c_str(), 0);
    }
    return offset;
}

/******************************************************************************/
/*! CurlUtil::DeleteFiles

     \brief      Deletes all files in the DownloadedFile list from the FTP directory.

     \param      ftpResponseList    - The data structure that contains curl/FTP messages.
     \return                        - The curl error code.
 */
/******************************************************************************/
int CurlUtil::DeleteFiles(Blaze::Arson::FtpResponseList& ftpResponseList)
{
    int result = CURLE_OK;

    // all files still in the downloaded file list need to be deleted
    // create a curl list of delete commands
    if (mConfigLoaded)
    {
        struct curl_slist* commands = nullptr;

        FtpCallbackData responseData = { 0 };
        responseData.curlUtil = this;
        responseData.ftpResponseList = &ftpResponseList;
        responseData.output = 0x0;

        // this callback will record the filenames
        curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, WriteNothing);
        curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &responseData);

        // add callbacks and user data to write FTP header information
        curl_easy_setopt(mHandle, CURLOPT_HEADERFUNCTION, WriteHeaders);
        curl_easy_setopt(mHandle, CURLOPT_WRITEHEADER, &responseData);

        FileDownloadDetailsList::iterator i = mFileDownloadDetailsList.begin();
        FileDownloadDetailsList::const_iterator e = mFileDownloadDetailsList.end();
        eastl::string::CtorSprintf temp;

        for(; i!=e; ++i)
        {
            // create the command to delete a filename on the sftp server using the 'rm' command
            // give path from the server's default directory
            FileDownloadDetails* downloadDetails = *i;
            const char* filename = downloadDetails->mFilename.c_str();
            eastl::string command(temp, "rm \"/%s/%s\"", mFtpRemotePath, filename);

            commands = curl_slist_append(commands, command.c_str());
        }
        curl_easy_setopt(mHandle, CURLOPT_POSTQUOTE, commands);

        // set the url
        eastl::string url(temp, "sftp://%s/", mFtpHostname);
        curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());

        // now execute that list of commands
        result = curl_easy_perform(mHandle);

        // clean up the list
        curl_slist_free_all(commands);
    }
    else
    {
        BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] NflFantasyConfig object not found. Curl based FTP disabled.");
    }
    return result;
}

/******************************************************************************/
/*! CurlUtil::CreateCurlFtpHandle

     \brief      Creates a curl FTP handle to resuse for all FTP calls

     \return     - The curl error code.
 */
/******************************************************************************/
int  CurlUtil::CreateCurlFtpHandle()
{
    int result = CURLE_OK;

    // global initialization
    result = curl_global_init(CURL_GLOBAL_ALL);
    if(result)
    {
        return result;
    }

    // initialization of easy mHandle
    mHandle = curl_easy_init();
    if(!mHandle)
    {
        curl_global_cleanup();
        return CURLE_OUT_OF_MEMORY;
    }

    if (mConfigLoaded)
    {
        // set login credentials
        curl_easy_setopt(mHandle, CURLOPT_USERNAME, mFtpUsername);
        curl_easy_setopt(mHandle, CURLOPT_PASSWORD, mFtpPassword);

        // turn on debug information in the config file
        if (mFtpIsVerbose)
        {
            curl_easy_setopt(mHandle, CURLOPT_DEBUGFUNCTION, WriteDebug);
            curl_easy_setopt(mHandle, CURLOPT_VERBOSE, 1L);
        }

        // if this is not set, curl will block on 404 or like errors
        curl_easy_setopt(mHandle, CURLOPT_FAILONERROR, 1L);
    }
    else
    {
        BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] NflFantasyConfig object not found. Curl based FTP disabled.");
    }

    return result;
}

/******************************************************************************/
/*! CurlUtil::CloseCurlFtpHandle

     \brief      Closes the opened curl FTP handle and frees the curl memory

     \return     - The curl error code.
 */
/******************************************************************************/
void CurlUtil::CloseCurlFtpHandle()
{
    curl_easy_cleanup(mHandle);
    curl_global_cleanup();
}

/******************************************************************************/
/*! CurlUtil::DiscoverRemoteFiles

     \brief      Discovers all of the files in the FTP directory.

     \param      ftpResponseList    - The data structure that contains curl/FTP messages.
     \return                        - The curl error code.
 */
/******************************************************************************/
int CurlUtil::DiscoverRemoteFiles(Blaze::Arson::FtpResponseList& ftpResponseList)
{
    int result = CURLE_OK;

    FtpCallbackData callbackData = { 0 };
    callbackData.curlUtil = this;
    callbackData.ftpResponseList = &ftpResponseList;

    if (mConfigLoaded)
    {
        // we only want the remote path filenames
        curl_easy_setopt(mHandle, CURLOPT_DIRLISTONLY, 1L);

        // this callback will record the filenames
        curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, DiscoverFilenames);
        curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &callbackData);

        // add callbacks and user data to write FTP header information
        curl_easy_setopt(mHandle, CURLOPT_HEADERFUNCTION, WriteHeaders);
        curl_easy_setopt(mHandle, CURLOPT_WRITEHEADER, &callbackData);

        // set the url
        eastl::string::CtorSprintf temp;
        eastl::string url(temp, "sftp://%s/%s/", mFtpHostname, mFtpRemotePath);
        curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());

        // execute queued curl commands, starting the file transfer
        result = curl_easy_perform(mHandle);

        // turn this option off here, so we don't cause side effects
        curl_easy_setopt(mHandle, CURLOPT_DIRLISTONLY, 0L);
    }
    else
    {
        BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] NflFantasyConfig object not found. Curl based FTP disabled.");
    }

    return result;
}

/******************************************************************************/
/*! CurlUtil::PullFiles

     \brief      Pulls all files in a given FTP directory by using a wildcard.

     \param      destDir            - Directory to save the FTP files to.
     \param      ftpResponseList    - The data structure that contains curl/FTP messages.
     \return                        - The curl error code.
 */
/******************************************************************************/
int CurlUtil::PullFiles(const char* destDir, Blaze::Arson::FtpResponseList& ftpResponseList)
{
    int result = CURLE_OK;

    FtpCallbackData callbackData = { 0 };
    callbackData.curlUtil = this;
    callbackData.ftpResponseList = &ftpResponseList;

    FtpCallbackData responseData = { 0 };
    responseData.curlUtil = this;
    responseData.ftpResponseList = &ftpResponseList;
    responseData.output = 0x0;


    if (mConfigLoaded)
    {
        // this callback will write downloaded buffer contents into files
        curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, WriteFile);
        curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, &callbackData);

        // add callbacks and user data to write FTP header information
        curl_easy_setopt(mHandle, CURLOPT_HEADERFUNCTION, WriteHeaders);
        curl_easy_setopt(mHandle, CURLOPT_WRITEHEADER, &responseData);


        FileDownloadDetailsList::iterator i = mFileDownloadDetailsList.begin();
        FileDownloadDetailsList::const_iterator e = mFileDownloadDetailsList.end();
        eastl::string::CtorSprintf temp;

        for(; i!=e; ++i)
        {
            FileDownloadDetails* downloadDetails = *i;
            const char* filename = downloadDetails->mFilename.c_str();
            eastl::string filePath(temp, "%s/%s", destDir, filename);
            callbackData.output = fopen(filePath.c_str(), "wb"); // binary required for windows

            // set the url
            eastl::string url(temp, "sftp://%s/%s/%s", mFtpHostname, mFtpRemotePath, filename);
            curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());

            // execute queued curl commands, starting the file transfer
            result = curl_easy_perform(mHandle);

            if (result == CURLE_OK)
            {
                double filesize = 0.0;
                result = curl_easy_getinfo(mHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);

                if (result == CURLE_OK && filesize > 0)
                {
                    downloadDetails->mFilesize = static_cast<size_t>(filesize);

                    eastl::string message(temp, "Downloaded: %s \t %20lu bytes", downloadDetails->mFilename.c_str(), downloadDetails->mFilesize);
                    if (mFtpIsVerbose)
                    {
                        BLAZE_TRACE_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] " << message.c_str());
                    }
                    ftpResponseList.push_back(message.c_str());
                }
            }

            fclose(callbackData.output);

            if (result != CURLE_OK)
            {
                BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] Curl Failed to download file: " << url.c_str() << ".");
                break;
            }
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] NflFantasyConfig object not found. Curl based FTP disabled.");
    }


    return result;
}

/******************************************************************************/
/*! CurlUtil::RecordDownloadedFile

     \brief      Records the names of all files we download and their size for later comparisons.

     \param      filename   - The name of the file downloaded.
     \param      size       - the size of the file downloaded (in bytes).
 */
/******************************************************************************/
void CurlUtil::RecordDownloadedFile(const char* filename, size_t size)
{
    if (!mFtpThrottlingEnabled || (mFileDownloadDetailsList.size() < mFtpMaxFilesPerInterval))
    {
        FileDownloadDetails* downloadDetails = BLAZE_NEW FileDownloadDetails();
        downloadDetails->mFilename.assign(filename);
        downloadDetails->mFilesize = size;

        mFileDownloadDetailsList.push_back(downloadDetails);
    }
}


/******************************************************************************/
/*! CurlUtil::PullXmlFiles

     \brief  Pulls notifications from the NFL FTP site

     \param      destDir            - Directory to save the FTP files to.
     \param      deletePulledFiles  - Should the downloaded files be deleted from the server?
     \param      ftpResponseList    - The list of curl/FTP messages.
     \return                        - true if success.
*/
/******************************************************************************/
bool CurlUtil::PullXmlFiles(const char* destDir, bool deletePulledFiles, Blaze::Arson::FtpResponseList& ftpResponseList)
{
    int result = CURLE_OK;
    if ( (result = CreateCurlFtpHandle()) != CURLE_OK)
    {
        eastl::string error(curl_easy_strerror(static_cast<CURLcode>(result)));
        TrimString(error);
        ftpResponseList.push_back(error.c_str());
        BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] Curl Error: " << error.c_str());
        return false;
    }

    // discover the list of files in the download directory
    if ( (result = DiscoverRemoteFiles(ftpResponseList)) != CURLE_OK)
    {
        eastl::string error(curl_easy_strerror(static_cast<CURLcode>(result)));
        TrimString(error);
        ftpResponseList.push_back(error.c_str());
        BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] Curl Error: " << error.c_str());

        CloseCurlFtpHandle();
        return false;
    }

    // pull the files found in the FTP directory to the local directory
    if ( (result = PullFiles(destDir, ftpResponseList)) != CURLE_OK)
    {
        eastl::string error(curl_easy_strerror(static_cast<CURLcode>(result)));
        TrimString(error);
        ftpResponseList.push_back(error.c_str());
        BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] Curl Error: " << error.c_str());

        CloseCurlFtpHandle();
        return false;
    }


    // iterate over each file on the server directory and get its files size
    // remove any file where we downloaded a size that is not the same as the
    // server size

    if (deletePulledFiles)
    {
        // delete all remaining files on the FTP server
        if ( (result = DeleteFiles(ftpResponseList)) != CURLE_OK)
        {
            eastl::string error(curl_easy_strerror(static_cast<CURLcode>(result)));
            TrimString(error);
            ftpResponseList.push_back(error.c_str());
            BLAZE_ERR_LOG(Log::HTTP, "[" << EA_CURRENT_FUNCTION << "] Curl Error: " << error.c_str());

            CloseCurlFtpHandle();
            return false;
        }
    }

    CloseCurlFtpHandle();
    return true;
}

} // namespace Arson
} // namespace Blaze
