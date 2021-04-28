/*************************************************************************************************/
/*!
\file logfileinfo.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "framework/logger/logfileinfo.h"

#include "EAIO/EAFileBase.h"
#include "EAIO/EAFileUtil.h"

namespace Blaze
{

    const int64_t LOGGING_OPEN_FILE_RETRY_DELAY = 10; // number of seconds before retrying

    LogFileInfo::LogFileInfo()
        : mInitialized(false)
        , mEnabled(false)
        , mIsBinary(false)
        , mFp(nullptr)
        , mWrittenSize(0)
        , mFlushedSize(0)
        , mSequenceCounter(0)
    {
        mpNext = mpPrev = nullptr;
    }

    void LogFileInfo::initialize(const char8_t* subName, const char8_t* subFolderName)
    {
        EA_ASSERT_MSG(!mInitialized, "Cannot initialize a LogFileInfo after it has already been initialized.");
        mInitialized = true;

        if (Logger::mName[0] == '\0')
        {
            // The '--logname' command-line arg was not provided on the command line. This effectively
            // means logging to file is disabled across the board.
            return;
        }

        // Set the base name of the file. Typically something like "blaze_coreSlave2"
        // Then add the subName, which would make it like "blaze_coreSlave2_usersessions".
        mFileName = Logger::mName;
        if (subName && *subName != '\0')
            mFileName.append_sprintf("_%s", subName);

        // Set the log folder, which initially comes from the '--logdir' command-line arg.
        // Then, if one was actually provided, tack on a path separator (/).
        mFileDirectory = Logger::mLogDir;
        if (!mFileDirectory.empty())
            mFileDirectory.append(EA_FILE_PATH_SEPARATOR_STRING_8);

        // Append the "binary_events/" (or other) folder name if this is actually a special type of log file.
        if (subFolderName && *subFolderName != '\0')
        {
            // Logs that are put into a sub-folder are considered 'binary logs'.  Being a binary log file just
            // means we won't try to massage or inject any content into the file because it screw up a reader
            // of the file. The binary event and PIN event log files are examples of this.
            mIsBinary = true;
            mFileDirectory.append_sprintf("%s" EA_FILE_PATH_SEPARATOR_STRING_8, subFolderName);
        }
    }

    void LogFileInfo::outputData(const char8_t* buffer, uint32_t count, eastl::intrusive_list<LogFileInfo>& flushList, bool allowRotate)
    {
        if (!isEnabled())
            return;

        if (mFp == nullptr)
            open();

        if (mFp != nullptr)
        {
            uint32_t written = fwrite(buffer, 1, count, mFp);

            mWrittenSize += written;

            // Rotate the log file if necessary
            if (allowRotate && (mWrittenSize >= Logger::mRotationFileSize))
            {
                mFlushedSize = mWrittenSize;
                fflush(mFp);
                rotate();
            }
            else if ((mpNext == nullptr) && (mpPrev == nullptr))
            {
                flushList.push_back(*this);
            }
        }
    }


    void LogFileInfo::rotate(TimeValue now)
    {
        mWrittenSize = 0;
        mFlushedSize = mWrittenSize;
        mOpenFailureTime = 0;

        // Not logging to a file so don't do anything
        if (mFileName.empty())
            return;

        if (mFp != nullptr)
        {
            fclose(mFp);
            mFp = nullptr;
        }

        eastl::string timestampStr;
        uint32_t year, month, day, hour, min, sec, msec;
        TimeValue::getLocalTimeComponents(now, &year, &month, &day, &hour, &min, &sec, &msec);
        timestampStr.sprintf("%04d%02d%02d_%02d%02d%02d-%03d", year, month, day, hour, min, sec, msec);

        StringBuilder currentPath, renamedPath;
        currentPath << mFileDirectory << mFileName << Logger::BLAZE_LOG_FILE_EXTENSION;
        renamedPath << mFileDirectory << mFileName << "_" << timestampStr << Logger::BLAZE_LOG_FILE_EXTENSION;

        // Rename the current log file to a dated version
        rename(currentPath.c_str(), renamedPath.c_str());

        mLastRotation = now;
    }

    void LogFileInfo::open()
    {
        if (mFileName.empty())
            return;

        // Wait at least X seconds before a retry to open log file for write.
        // If first time to open file, mOpenFailureTime will be zero, and should not exit/return.
        if ((TimeValue::getTimeOfDay() - mOpenFailureTime).getSec() < LOGGING_OPEN_FILE_RETRY_DELAY)
        {
            return;
        }

        //check the binary log directory
        EA::IO::Directory::EnsureExists(mFileDirectory.c_str());

        StringBuilder fullFilePath;
        fullFilePath << mFileDirectory << mFileName << Logger::BLAZE_LOG_FILE_EXTENSION;

        mFp = fopen(fullFilePath.c_str(), "a+");
        if (mFp == nullptr)
        {
            if (mOpenFailureTime == 0)
            {
                // Avoid spamming the logs, so only logging the "first" attempt
                if (fullFilePath.length() > EA::IO::kMaxPathLength)
                {
                    BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].open: Cannot open log file with path length " << fullFilePath.length() << " greater than " << EA::IO::kMaxPathLength << ".");
                }
                BLAZE_ERR_LOG(Log::SYSTEM, "[Logger].open: Cannot open log file '" << fullFilePath << "' Error: " << strerror(errno));
            }
            mOpenFailureTime = TimeValue::getTimeOfDay();
            return;
        }

        if (mOpenFailureTime != 0) // failed previously.
        {
            mOpenFailureTime = 0;
            if (!mIsBinary)
            {
                const char8_t* warningLine = "\nWARNING: logging is now resuming after a previous failure to create this log file, some earlier log statements may have been lost.\n\n";
                auto len = strlen(warningLine);
                auto written = fwrite(warningLine, 1, len, mFp);
                fflush(mFp);
                mWrittenSize += written;
                mFlushedSize = mWrittenSize;
            }
        }
    }

    void LogFileInfo::close()
    {
        if (mFp != nullptr)
        {
            flush();
            fclose(mFp);
            mFp = nullptr;
        }
    }

    void LogFileInfo::flush()
    {
        if (mFp != nullptr)
        {
            if (mFlushedSize < mWrittenSize)
            {
                fflush(mFp);
                mFlushedSize = mWrittenSize;
            }
        }
        mpNext = mpPrev = nullptr;
    }

}
