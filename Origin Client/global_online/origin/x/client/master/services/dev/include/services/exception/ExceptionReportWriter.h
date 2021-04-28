///////////////////////////////////////////////////////////////////////////////
// ExceptionReportWriter.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
//
// We implement a system here which is based on EACallstack/ExceptionHandler/BugSentry package. 
// EACallstack+ExceptionHandler provide us the callstack information to go along 
// with crash(Exception raised) and BugSentry reports it to a central database.
///////////////////////////////////////////////////////////////////////////////

#ifndef ORIGIN_EXCEPTIONREPORTWRITER_H
#define ORIGIN_EXCEPTIONREPORTWRITER_H

#include <ExceptionHandler/ExceptionHandler.h>

#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        namespace Exception
        {
            // ExceptionReportWriter
            //
            // The purpose of this class is to write exception reports to a string instead of 
            // to a file. This string can then be sent to a server for storage and metrics.
            //
            // This report writer is notified by the ExceptionHandler package with the relevant information. 
            // We format this information as per our need.
            class ORIGIN_PLUGIN_API ExceptionReportWriter : public EA::Debug::ReportWriter
            {
            public:
                ExceptionReportWriter(bool fileWriteEnabled);

                bool OpenReport(const char* pReportPath);
                bool CloseReport();
                bool BeginReport(const char* pReportName);
                bool EndReport();
                bool BeginSection(const char* pSectionName);
                bool EndSection(const char* pSectionName);
                bool Write(const char* pData, size_t count);
                virtual bool WriteSystemInfo();
                
                int WriteEACoreIniToCrashReport();
                int WriteBootstrapLogToCrashReport(int byteLimit);
                int WriteMiniDumpToCrashReport(const QString& minidumpPath);

                const char* GetReportText() const;

            private:
                
                /// \brief  Writes the contents of the given file to the crash report.
                /// \param  filePath        The path to the file that you want to add to the crash report.
                /// \param  startMarker     A string that demarcates the beginning of the file in the crash report.
                /// \param  endMarker       A string that demarcates the end of the file in the crash report.
                /// \param  fileByteLimit   Enforces a byte limit on how many bytes of the given file should be
                ///                         included in the report. If the file size is less than the byte limit,
                ///                         the entire file is included in the report. Otherwise, the end of the
                ///                         file contents are included (typically the most recent entries of log
                ///                         files are located towards the end of the file).
                /// \return The number of bytes written.
                int WriteFileToCrashReport(const QString& filePath, const QString& startMarker, const QString& endMarker, int fileByteLimit = -1);

                eastl::string mReportText;
                bool mFileWriteEnabled;
            };
        }
    }
}

#endif // Header include guard
