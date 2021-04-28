/////////////////////////////////////////////////////////////////////////////
// ExceptionReportWriter.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ExceptionReportWriter.h"
#include "base64.h"
#include "services/log/LogService.h"
#include "services/compression/GzipCompress.h"
#include "TelemetryAPIDLL.h"
#include "services/platform/PlatformService.h"
#if defined (ORIGIN_PC)
#include "qt_windows.h"
#endif

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            /////////////////////////////////////////////////////////////////////////
            // ExceptionReportWriter
            /////////////////////////////////////////////////////////////////////////
            ExceptionReportWriter::ExceptionReportWriter(bool fileWriteEnabled) :
                EA::Debug::ReportWriter(),
                mReportText(),
                mFileWriteEnabled(fileWriteEnabled)
            {
            }

            // Override from EA::Debug::ReportWriter
            bool ExceptionReportWriter::OpenReport(const char* pReportPath)
            {
                if(mFileWriteEnabled)
                    return EA::Debug::ReportWriter::OpenReport(pReportPath);
                else
                    return true;
            }

            // Override from EA::Debug::ReportWriter
            bool ExceptionReportWriter::CloseReport()
            {
                if(mFileWriteEnabled)
                    return EA::Debug::ReportWriter::CloseReport();
                else
                    return true;
            }

            // Override from EA::Debug::ReportWriter
            bool ExceptionReportWriter::BeginReport(const char* pReportName)
            {
                mReportText.clear();
            
                if(mFileWriteEnabled)
                    return EA::Debug::ReportWriter::BeginReport(pReportName);
                else
                    return true;
            }

            // Override from EA::Debug::ReportWriter
            bool ExceptionReportWriter::EndReport()
            {
                if(mFileWriteEnabled)
                    return EA::Debug::ReportWriter::EndReport();
                else
                    return true;
            }

            // Override from EA::Debug::ReportWriter
            bool ExceptionReportWriter::BeginSection(const char* pSectionName)
            {
                if(mFileWriteEnabled)
                    return EA::Debug::ReportWriter::BeginSection(pSectionName);
                else
                    return true;
            }

            // Override from EA::Debug::ReportWriter
            bool ExceptionReportWriter::EndSection(const char* pSectionName)
            {
                if(mFileWriteEnabled)
                    return EA::Debug::ReportWriter::EndSection(pSectionName);
                else
                    return true;
            }

            // Override from EA::Debug::ReportWriter
            bool ExceptionReportWriter::Write(const char* pData, size_t count)
            {
                if(count == (size_t)-1)
                    count = strlen(pData);

                mReportText.append(pData,count);
            
                if(mFileWriteEnabled)
                    return EA::Debug::ReportWriter::Write(pData, count);
                else
                    return true;
            }

            const char* ExceptionReportWriter::GetReportText() const
            { 
                return mReportText.c_str(); 
            }

            bool ExceptionReportWriter::WriteSystemInfo()
            {
#ifdef ORIGIN_PC
                //  Only write out user and machine info if eacore.ini exists
                bool eaCoreIniExists = QFile::exists(Origin::Services::PlatformService::eacoreIniFilename());
                if (eaCoreIniExists == true)
                {
                    char  buffer[256];
                    DWORD dwCapacity;

                    // Write the computer name.
                    char machineName[32];
                    GetMachineName(machineName, sizeof(machineName));
                    WriteKeyValue("Computer name", machineName);

                    // Write the computer DNS name.
                    dwCapacity = (DWORD)sizeof(buffer);
                    if(GetComputerNameExA(ComputerNameDnsFullyQualified, buffer, &dwCapacity) > 0)
                        WriteKeyValue("Computer DNS name", buffer);

                    // Write the user name.
                    dwCapacity = (DWORD)sizeof(buffer);
                    if(GetUserNameA(buffer, &dwCapacity) > 0)           
                        WriteKeyValue("User name", buffer);
                }
#endif
                return EA::Debug::ReportWriter::WriteSystemInfo();
            }

            int ExceptionReportWriter::WriteFileToCrashReport(const QString& filePath, const QString& startMarker, const QString& endMarker, int fileByteLimit)
            {
                int previousReportSize = mReportText.length();

                //	Open the file if it exists
                FILE* inputFile = fopen(filePath.toUtf8().data(), "rb");
                if (inputFile != NULL)
                {
                    //	Use fseek and ftell to get the size of file
                    int result = fseek(inputFile, 0, SEEK_END);
                    if (result == 0) // success
                    {
                        int fileSize = ftell(inputFile);

                        // Check if we shuold limit the number of bytes written to the report. Also, if a byte limit has been
                        // specified, but the byte limit is greater than the size of the file itself, write out the entire file anyways.
                        if (fileByteLimit < 0 || fileSize < fileByteLimit)
                        {
                            // Write entire file to report
                            fseek(inputFile, 0, SEEK_SET);
                        }

                        else
                        {
                            fileSize = fileByteLimit;

                            // Write only a portion to the report by seeking back the byte limit amount
                            fseek(inputFile, -fileSize, SEEK_END);
                        }

                        // Allocate one extra byte for the null-terminating char
                        char* fileBuffer = new char[fileSize + 1];
                        if (fileBuffer != NULL)
                        {
                            //	Read "fileSize" bytes of inputFile
                            if (fread(fileBuffer, fileSize, 1, inputFile) != 0)
                            {
                                //	NULL terminate the string
                                fileBuffer[fileSize] = 0;
                                
                                //	Write file to crash report
                                WriteText(startMarker.toLocal8Bit().constData(), true);
                                WriteText(fileBuffer, true);
                                WriteText(endMarker.toLocal8Bit().constData(), true);
                            }
                        }
                        delete [] fileBuffer;
                    }
                    fclose(inputFile);
                }

                return (mReportText.length() - previousReportSize);
            }

            int ExceptionReportWriter::WriteEACoreIniToCrashReport()
            {
                QString ebisuIniName = Origin::Services::PlatformService::eacoreIniFilename();
            
                // Write out the full EACore.ini to the report
                return WriteFileToCrashReport(ebisuIniName, "\n[EACoreIniStart]\n", "\n[EACoreIniEnd]\n");
            }

            int ExceptionReportWriter::WriteBootstrapLogToCrashReport(int byteLimit)
            {
                QString bootstrapFilename = Origin::Services::PlatformService::bootstrapperLogFilename();

                // Write out only the given number of bytes of the bootstrap log file to the crash report.
                return WriteFileToCrashReport(bootstrapFilename, "\n[BootstrapStart]\n", "\n[BootstrapEnd]\n", byteLimit);
            }

            int ExceptionReportWriter::WriteMiniDumpToCrashReport(const QString& minidumpPath)
            {
                int previousReportSize = mReportText.length();

#if defined(ORIGIN_PC)
                WriteText("[MinidumpStart]", true);
            
                //	Open the minidump file
                FILE* miniDumpFile = fopen(minidumpPath.toUtf8().constData(), "rb");
                if (miniDumpFile != NULL)
                {
                    //	Use fseek and ftell to get the size of file
                    int result = fseek(miniDumpFile, 0, SEEK_END);
                    if (result == 0) // success
                    {
                        int miniDumpFileSize = ftell(miniDumpFile);
                        fseek(miniDumpFile, 0, SEEK_SET);
                        char* miniDumpBuffer = new char[miniDumpFileSize];
                        if (miniDumpBuffer != NULL)
                        {
                            //	Read the whole minidump file in
                            if (fread(miniDumpBuffer, miniDumpFileSize, 1, miniDumpFile) != 0)
                            {
                                //	Zip the mini-dump
                                QByteArray zipMiniDumpData;
                                zipMiniDumpData = gzipData(miniDumpBuffer, miniDumpFileSize);
                            
                                //	Encode 64 the zip mini-dump
                                size_t encodedByteCount = Base64EncodedSize(zipMiniDumpData.length());
                                char*  pEncodedBuffer   = new char[encodedByteCount + 1];
                                Base64Encode((int32_t)zipMiniDumpData.length(), zipMiniDumpData.data(), pEncodedBuffer);
                                WriteText(pEncodedBuffer, true);
                                delete [] pEncodedBuffer;
                            }
                        }
                        delete [] miniDumpBuffer;
                    }
                    fclose(miniDumpFile);
                }
                WriteText("[MinidumpEnd]", true);
#elif defined(ORIGIN_MAC)
                Q_UNUSED(minidumpPath);

                // For Apple platforms, probably the best approximation of a minidump is a core dump.
                // For example code to manually generate a core dump from user space, see:
                // http://osxbook.com/book/bonus/chapter8/core/download/gcore.c
#endif

                return (mReportText.length() - previousReportSize);
            }
        }
    }
}




