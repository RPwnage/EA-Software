///////////////////////////////////////////////////////////////////////////////
// DataCollector.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "DataCollector.h"
#include "ReportErrorView.h"
#include "ErrorHandler.h"
#include "Shlobj.h" // for SHGetSpecialFolderPath
#include "GzipCompress.h"
#include "atlbase.h"
#include "cstringt.h"
#include "atlstr.h"
#include "version/version.h"
#include <services/platform/PlatformService.h>
#include "TelemetryConfig.h"
#include "Utilities.h"
#include "EAStdC/EARandomDistribution.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <assert.h>

int const FILE_HEADER_VERSION = 0x1;
QString const ORIGIN_DATA_PATH = "\\Origin\\Logs\\";
QString const CORE_LOG_GLOB = "*.html";
QString const CLIENT_LOG_GLOB = "Client_Log*.txt";
QString const CLIENT_SERVICE_LOG_GLOB = "OriginClientService_*.txt";
QString const BOOTSTRAP_LOG_GLOB = "Bootstrapper_Log*.txt";
QString const HARDWARE_SURVEY = "Hardware_Survey.txt";
QString const IGO_LOGS = "IGO*.txt";
QString const DXDIAG = "DxDiag.txt";
QString const START_DELIMITER = "@@@START@@@";
QString const END_DELIMITER = "@@@END@@@";
QString const DXDIAG_SCRUB = "Machine Name";
LPCWSTR const DXDIAG_EXECUTABLE = L"\\dxdiag.exe";

/* 
Fixed size header block inspired by tar, contains:
    header block format identifier, 4 bytes, equal to 0x1
    file name, 32 wide characters padded with 0x00, at least one terminating 0x00 byte
    file size, 4 bytes
*/
QByteArray buildFileHeader( QString name, int size )
{
    assert(sizeof(int)==4);

    int const NAME_LENGTH = 32;
    int const BYTES_PER_CHAR = 2;
    int const NAME_SIZE = NAME_LENGTH * BYTES_PER_CHAR;

    int const VERSION_OFFSET = 0;
    int const NAME_OFFSET = VERSION_OFFSET + sizeof(int);
    int const FILESIZE_OFFSET = NAME_OFFSET + NAME_SIZE;
    int const BUFSIZE = FILESIZE_OFFSET + sizeof(int);

    char buffer[BUFSIZE];
    memset(buffer, 0, BUFSIZE);

    int* versionPtr = (int*) &buffer[VERSION_OFFSET];
    char* namePtr   =        &buffer[NAME_OFFSET];
    int* sizePtr    = (int*) &buffer[FILESIZE_OFFSET];

    *versionPtr = FILE_HEADER_VERSION;
    *sizePtr = size;
    name.truncate(NAME_LENGTH-1); // does nothing if the string is shorter
    memcpy(namePtr, name.constData(), BYTES_PER_CHAR * name.size());

    return QByteArray(buffer, BUFSIZE);
}

DataCollector* DataCollector::sInstance = NULL;

DataCollector* DataCollector::instance()
{
    if(!sInstance)
        init();

    return sInstance;
}

DataCollector::DataCollector()
    : m_reportView(0)
    , m_errorHandler(0)
{
}

DataCollector::~DataCollector()
{
}

void DataCollector::setErrorHandler(ErrorHandler* errorHandler)
{
    assert(errorHandler);
    m_errorHandler = errorHandler;
}

ErrorHandler* DataCollector::getErrorHandler()
{
    assert(m_errorHandler);
    return m_errorHandler;
}

void DataCollector::setReportView(ReportErrorView* dialog)
{
    assert(dialog);
    m_reportView = dialog;
}
ReportErrorView* DataCollector::getReportView()
{
    assert(m_reportView);
    return m_reportView;
}

void DUMP( QByteArray const& data, QString const& name )
{
    QFile f1(name);
    f1.open(QIODevice::WriteOnly);
    f1.write(data);
}

void DataCollector::removePersonalIdentifiers(QByteArray& haystack)
{
    QStringList needles;
    needles << 
        "Underage" <<
        "Dob" <<
        "FirstName" <<
        "LastName" <<
        "EAId" <<
        "EA_Id" <<
        "Mail" <<
        "PersonaId" <<
        "UserId" 
        //"Status" <<
        //"Email" <<
        //"Country" <<
        //"Language" <<
        //"step" <<
        //"AuthToken" <<
        //"ActivationToken" <<
        //"GenericAuthToken" <<
        ;
    QStringList tmp;
    for ( QStringList::const_iterator i = needles.begin(); i != needles.end(); ++i ) 
        tmp << i->toUpper() << i->toLower();
    needles.append(tmp);

    for ( QStringList::const_iterator i = needles.begin(); i != needles.end(); ++i ) 
    {
        QByteArray needle = i->toUtf8();
        // DUMP( needle, "needle.txt" );
        // DUMP( haystack, "before.html" );
        int index = haystack.indexOf(needle);
        while (index != -1)
        {
            // DUMP( haystack, "before.html" );
            int from = haystack.lastIndexOf("\r\n", index);
            int to = haystack.indexOf("\r\n", index);

            if ( from == -1 ) 
                from = 0;

            if ( to == -1 ) 
                to = haystack.size();

            haystack.replace(from, to - from, QString("\r\n").toUtf8());
            index = haystack.indexOf(needle, from);
            // DUMP( haystack, "after.html" );
        }
    }
}

void DataCollector::appendFileContents(QString const& glob, QByteArray& dest, QString const& /*fileName*/)
{
    QFileInfoList files = QDir(getDataFolder()).entryInfoList(QStringList() << glob);
    for ( QFileInfoList::const_iterator i = files.begin(); i != files.end(); ++i ) 
    {
        QByteArray data;
        if ( !i->exists() )
            data.append(glob + ": This file does not exist");

        else if ( !i->isFile() )
            data.append(glob + ": This is not a file");

        else if ( !i->isReadable() )
            data.append(glob + ": Could not read the file");

        // reusing the max upload size as limit on the size of each log file
        else if ( i->size() > MAX_UPLOAD_SIZE )
            data.append(glob + ": File size too big");

        else
        {
            QFile file(i->filePath());
            file.open(QIODevice::ReadOnly);
            data = file.readAll();
            appendDelimiters(data, i->fileName());
        }
        dest.append(data); 
    }
}

DataCollectedType DataCollector::dumpClientLogs()
{
    appendFileContents(CLIENT_LOG_GLOB, m_diagnosticData, CLIENT_LOG_GLOB);
    readAndDumpMinidumps();
    collectTelemetryErrorLog();
    // Append OriginClientService_*.txt to the glob
    appendFileContents(CLIENT_SERVICE_LOG_GLOB, m_diagnosticData, CLIENT_SERVICE_LOG_GLOB);
    appendFileContents(CORE_LOG_GLOB, m_diagnosticData, CORE_LOG_GLOB);
    appendFileContents(IGO_LOGS, m_diagnosticData, IGO_LOGS);
    return CLIENT_LOG;
}

void DataCollector::collectTelemetryErrorLog()
{
    EA::StdC::RandomLinearCongruential rand(0x3C4D);

    eastl::wstring location;
    NonQTOriginServices::Utilities::getIOLocation(OriginTelemetry::TelemetryConfig::originLogPath(), location);
    // construct full path to errors.dat. Location should be C:\ProgramData\Origin\errors.dat
    QString path = QString::fromWCharArray(location.c_str())+QString::fromWCharArray(OriginTelemetry::TelemetryConfig::telemetryErrorDefaultFileName());
    QFile file(path);
    // Attempt to open and read the contents of the file
    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray contents = file.readAll();
        QByteArray line;
        QByteArray final;
        uint32_t index = 0;
        // decyphre the DAT file content
        while(1)
        {
            uint32_t  length = readObfuscatedLine(contents, line, index);
            if(length == 0)
                break;

            if(length == -1)
            {
                // reset the random generator
                rand.SetSeed(0x3C4D);
                continue;
            }

            for(uint32_t i=0; i<length; i++)
                line[i] = line[i] ^ (int8_t) EA::StdC::Random256(rand);

            line.append(line[0]);
            for(uint32_t i=0; i<length; i++)
            {
                final += ((line[i] << 4) & 0xF0) | ((line[i+1] >> 4) & 0x0F);
            }
            final.append("\n");
        }
        appendDelimiters(final.append("\n"), QString::fromWCharArray(OriginTelemetry::TelemetryConfig::telemetryErrorDefaultFileName()));
        m_diagnosticData.append(QString(final));

        file.close();
    }
}

void DataCollector::readAndDumpMinidumps()
{
    QStringList extensionFilter("*.mdmp");
    QDir directory(m_DataFolder);
    QStringList files = directory.entryList(extensionFilter);
    for(int i = 0; i < files.size(); ++i)
    {
        QString minidump = files.at(i);
        QFile minidumpFile(m_DataFolder+minidump);
        if(minidumpFile.open(QIODevice::ReadOnly))
        {
            QByteArray mdmpContents = minidumpFile.readAll();
            mdmpContents = mdmpContents.toBase64();
            appendDelimiters(mdmpContents.append("\n"), minidump);
            m_diagnosticData.append(QString(mdmpContents));
            minidumpFile.close();
        }
    }
}

DataCollectedType DataCollector::dumpBootStrapperLogs()
{
    emit(backendProgress(CLIENT_LOG,BOOTSTRAPPER_LOG));
    appendFileContents(BOOTSTRAP_LOG_GLOB, m_diagnosticData, "Bootstrapper_Log.txt");
    return BOOTSTRAPPER_LOG;
}

DataCollectedType DataCollector::dumpHardwareLogs()
{
    emit(backendProgress(BOOTSTRAPPER_LOG,HARDWARE));
    QByteArray machineLogData = getMachineLogData().toUtf8();
    appendDelimiters(machineLogData, HARDWARE_SURVEY);
    m_diagnosticData.append(QString(machineLogData)); // TODO handle wide chars here
    return HARDWARE;
}

DataCollectedType DataCollector::dumpDxDiagLogs()
{
    emit(backendProgress(HARDWARE,DXDIAG_DUMP));
    dumpDxDiagToTxt();
    return DXDIAG_DUMP;
}

DataCollectedType DataCollector::readDxDiagLogs()
{
    emit(backendProgress(DXDIAG_DUMP,READ_DXDIAG));
    QByteArray dxdiagInfo = readDxDiagTxt().toUtf8();
    appendDelimiters(dxdiagInfo, DXDIAG);
    m_diagnosticData.append(QString(dxdiagInfo)); // TODO handle wide chars here
    return READ_DXDIAG;
}

QByteArray const DataCollector::getDiagnosticData()
{
    return m_diagnosticData;
}

QByteArray const& DataCollector::getDiagnosticDataCompressed()
{
    if ( m_diagnosticDataCompressed.isEmpty() )
        m_diagnosticDataCompressed = Origin::Services::gzipData(getDiagnosticData());

    return m_diagnosticDataCompressed;
}

QString const& DataCollector::getOriginVersion()
{
    if ( m_originVersion.isEmpty() )
    {
        m_originVersion = QString(EBISU_VERSION_STR).replace(",", ".").append(" - " EBISU_CHANGELIST_STR);
    }
    return m_originVersion;
}

QString const& DataCollector::getLocale()
{
    return mLocale;
}
void DataCollector::setLocale(QString const& locale)
{
    mLocale = locale;
}

void const DataCollector::appendDelimiters(QByteArray& data, QString const& fileName)
{
    data.prepend(START_DELIMITER.toUtf8() + fileName.toUtf8() + "\n");
    data.append(END_DELIMITER.toUtf8() + fileName.toUtf8() + "\n");
}

QString DataCollector::getDataFolder()
{
    if ( m_DataFolder.isEmpty() )
    {
        wchar_t path[MAX_PATH];
        SHGetSpecialFolderPath(NULL, path, CSIDL_COMMON_APPDATA, false);
        m_DataFolder = QString::fromWCharArray(path) + ORIGIN_DATA_PATH; 
    }
    return m_DataFolder;
}

NonQTOriginServices::SystemInformation* DataCollector::getSystemInformation()
{
        //TODO make the SystemINformation Object a service and get the constructed object instead of constructing a new one.
        // note that this call causes the current hash to be
        // written to disk, this functionality is used by the 
        // origin client, but not by OriginER.
    return &NonQTOriginServices::SystemInformation::instance();
}

QString const& DataCollector::getMachineHash()
{
    if ( m_MachineHash.isEmpty() )
    {
        assert(sizeof(uint64_t) == sizeof(qulonglong));
        // The system information class computes two hashes, one
        // which is based on the MAC address only, returned by 
        // SystemInformation::GetMachineHash(), and one which is based
        // on that value plus a lot of hardware and software settings,
        // computed by SystemInformation::CreateChecksum() but not accessible
        // through any public function.
        uint64_t checksum = getSystemInformation()->GetMachineHash();
        // format as 16 digit upper case hexadecimal with leading zeroes
        m_MachineHash = QString("%1").arg(static_cast<qulonglong>(checksum), 16, 16, QChar('0')).toUpper();
    }
    return m_MachineHash;
}

QString DataCollector::getCompatibilityMode()
{
    QString mode = Origin::Services::PlatformService::getCompatibilityMode();
    if (mode.isEmpty())
        return QString("Not using compatibility mode");

    return mode;
}

QString const& DataCollector::getMachineLogData()
{
    if ( m_MachineLogData.isEmpty() )
    {
        NonQTOriginServices::SystemInformation* si = getSystemInformation();
        m_OSVersionString = si->GetOSVersion();
        QTextStream stream(&m_MachineLogData, QIODevice::WriteOnly);
        stream 
        //<< "\t" << si->GetClientVersion() << "\r\n"

        //<< "Browser\t" << si->GetBrowserName() << "\r\n"
        << "OSLocale\t\t" << si->GetOSLocale() << "\r\n"
        << "UserLocale\t\t" << si->GetUserLocale() << "\r\n"

        << "CPUName\t\t\t" << si->GetCPUName() << "\r\n"
        << "CPUCount\t\t" << si->GetNumCPUs() << "\r\n"
        << "CoreCount\t\t" << si->GetNumCores() << "\r\n"
        << "CPUSpeed\t\t" << si->GetCPUSpeed() << "\r\n"
        << "InstructionSet\t\t" << si->GetInstructionSet() << "\r\n"

        << "InstalledMemory\t\t" << si->GetInstalledMemory_MB() << "\r\n"

        //  OS
#ifdef ORIGIN_PC
        << "Compatibility Mode\t" << getCompatibilityMode() << "\r\n"
#endif
        << "OSBuild\t\t\t" << si->GetOSBuildString() << "\r\n"
        << "OSVersion\t\t" << m_OSVersionString << "\r\n"
        << "OSServicePack\t\t" << si->GetServicePackName() << "\r\n"
        << "OSArchitecture\t\t" << si->GetOSArchitecture() << "\r\n"

        //  Locale setting from the Nucleus account
        << "LocaleSettings\t\t" << getLocale() << "\r\n"
        //<< "MACAddress\t" << si->GetMacAddr() << "\r\n"
    
        << "HDDCount\t\t" << si->GetNumHDDs() << "\r\n";
        uint32_t i = 0;
        for ( i = 0; i < si->GetNumHDDs(); ++i )
            stream 
            << "HDDSpaceGB\t\t" << si->GetHDDSpace_GB(i) << "\r\n";

        stream 
        << "DisplayCount\t\t" << si->GetNumDisplays() << "\r\n";
        for ( i = 0; i < si->GetNumDisplays(); ++i ) 
            stream 
            << "Resolution\t\t" << si->GetDisplayResX(i) 
            << "x" << si->GetDisplayResY(i) << "\r\n";

        stream 
        << "VideoControllerCount\t" << si->GetNumVideoControllers() << "\r\n";
        for ( i = 0; i < si->GetNumVideoControllers(); ++i )
        {
            stream 
            << "VideoAdaptorName\t" << si->GetVideoAdapterName(i) << "\r\n"
            << "VideoAdaptorBitsDepth\t" << si->GetVideoAdapterBitsDepth(i) << "\r\n"
            << "VideoAdaptorVRAM\t" << si->GetVideoAdapterVRAM(i) << "\r\n"
            << "VideoAdaptorDeviceID\t" << si->GetVideoAdapterDeviceID(i) << "\r\n"
            << "VideoAdaptorVendorID\t" << si->GetVideoAdapterVendorID(i) << "\r\n";
        }

        //  Extended resolution
        stream 
        << "VirtualResolution\t" << si->GetVirtualResX() << "x"
        << si->GetVirtualResY() << "\r\n"
        << "Microphone\t\t" << si->GetMicrophone() << "\r\n";

    }
    return m_MachineLogData;
}
void DataCollector::dumpDxDiagToTxt()
{
    wchar_t pathToDxDiag[MAX_PATH];
    LPTSTR sDestFolder = (LPTSTR)getDataFolder().utf16();
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    SHGetSpecialFolderPath(NULL, pathToDxDiag, CSIDL_SYSTEM, false);
    wcscat(pathToDxDiag,DXDIAG_EXECUTABLE);

    // Start the child process. 
    if( !CreateProcess( pathToDxDiag,   // No module name (use command line)
        _T("dxdiag.exe /tDxDiag.txt"),        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        sDestFolder,           // Use parent's starting directory 
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) 
    {
        return;
    }

    // Wait until child process exits. Wait 1 minute.
    WaitForSingleObject( pi.hProcess, 60000 );

    // Close process and thread handles. 
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}
QString DataCollector::readDxDiagTxt()
{
    QFile dxdiag(m_DataFolder+DXDIAG);
    QString content = "";
    if(dxdiag.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&dxdiag);
        while(!stream.atEnd())
        {
            QString line = stream.readLine();
            if(!line.toLower().contains(DXDIAG_SCRUB.toLower()))
                content.append(line+"\n");
        }
    }
    return content;
}

int DataCollector::readObfuscatedLine(QByteArray &src, QByteArray &dst, uint32_t &startIdx)
{
    if(startIdx >= static_cast<uint32_t>(src.length()))
        return 0;

    // just in case
    dst.clear();

    while(1)
    {
        int8_t c = src[startIdx++];
        if((c == OriginTelemetry::END_OF_FILE) || (c == OriginTelemetry::END_OF_LINE))
            return dst.length();
        if(c == OriginTelemetry::RESET_GENERATOR_ESC)
            return -1;

        if(c == OriginTelemetry::ESCAPE_CHAR)
        {
            // see what the next character is
            int8_t n = src[startIdx++];
            switch(n)
            {
                case OriginTelemetry::ESCAPE_CHAR:
                    // 'D''D' maps to 'D'
                    // do not do anything
                break;

                case OriginTelemetry::END_OF_LINE_ESC:
                    // end of line
                    c = OriginTelemetry::END_OF_LINE;
                break;

                case OriginTelemetry::END_OF_STRING_ESC:
                    // end of line
                    c = OriginTelemetry::END_OF_STRING;
                break;

                case OriginTelemetry::END_OF_FILE_ESC:
                    // end of file
                    c = OriginTelemetry::END_OF_FILE;
                break;

                case OriginTelemetry::RESET_GENERATOR_ESC:
                    // reset random generator
                    c = OriginTelemetry::RESET_GENERATOR_ESC;
                break;

                default:
                    // we have a problem
                    return 0;
                break;
            }
        }

        dst.append(c);

    }

    return dst.length();
}
