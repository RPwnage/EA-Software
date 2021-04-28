///////////////////////////////////////////////////////////////////////////////
// DataCollector.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DataCollector_h
#define DataCollector_h
#include <QObject>

// global_online\ebisu\client\main\TelemetryAPI\TelemetryAPIDLL\SystemInformation.h
#include "SystemInformation.h"
#include <QString.h>

class ErrorHandler;
class ReportErrorView;

int const MAX_UPLOAD_SIZE = 10*1024*1024; // 10 MB

enum DataCollectedType{
    START_OR_END,           // Starting and ending
    CLIENT_LOG,             // Client_Log.txt
    BOOTSTRAPPER_LOG,       // Bootstrapper_Log.txt
    HARDWARE,               // Hardware Survey dump
    DXDIAG_DUMP,            // Call dxdiag.exe and dump to a file
    READ_DXDIAG,            // Read the dumped DxDiag.txt
    NETWORK,                // Preparing to upload. This is the final step
    TOTAL                   // Total number of processes to enumerate over. Leave this as the last entry
};

class DataCollector : public QObject
{
    Q_OBJECT
public:
	DataCollector();
	~DataCollector();

    static DataCollector* instance();

	void setErrorHandler(ErrorHandler* errorHandler);
	ErrorHandler* getErrorHandler();

	void setReportView(ReportErrorView* dialog);
	ReportErrorView* getReportView();

	QByteArray const& getDiagnosticDataCompressed();
	QString const& getMachineHash();
	QString const& getMachineLogData();
	QString const& getOriginVersion();
    QString const& getLocale();
    void setLocale(QString const& locale);
    void const appendDelimiters(QByteArray& data, QString const& fileName);
    QString getCompatibilityMode();
    QString getDataFolder();
    QByteArray const getDiagnosticData();

    // These functions are called on different threads. See DiagnosticApplication::runDiagnosticThread
    DataCollectedType dumpClientLogs();
    DataCollectedType dumpBootStrapperLogs();
    DataCollectedType dumpDxDiagLogs();
    DataCollectedType dumpHardwareLogs();
    DataCollectedType readDxDiagLogs();
    void readAndDumpMinidumps();
    void collectTelemetryErrorLog();

    QString m_OSVersionString;
    QString mLocale;

private:
    static DataCollector* sInstance;
    static void init() { sInstance = new DataCollector(); }

    NonQTOriginServices::SystemInformation* getSystemInformation();
    void appendFileContents(QString const& name, QByteArray& dest, QString const& fileName);
    void removePersonalIdentifiers(QByteArray& data);

    void dumpDxDiagToTxt();
    QString readDxDiagTxt();

    /// reads one line from obfuscated stream
    /// \param src - source to read from - contains obfuscated string
    /// \param dst - destination where to store the read line
    /// \param startIdx - index into src where to commence the reading of the next line
    /// \return the number of bytes read
    int DataCollector::readObfuscatedLine(QByteArray &src, QByteArray &dst, uint32_t &startIdx);

    ReportErrorView* m_reportView;
    ErrorHandler* m_errorHandler;
    QString m_DataFolder;
    QString m_MachineHash;
    QString m_MachineLogData;
    QString m_originVersion;

    QByteArray m_diagnosticData;	
    QByteArray m_diagnosticDataCompressed;

signals:
    void backendProgress(DataCollectedType,DataCollectedType);
};

#endif