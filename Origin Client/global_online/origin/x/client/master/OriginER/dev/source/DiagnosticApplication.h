///////////////////////////////////////////////////////////////////////////////
// DiagnosticApplication.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef DiagnosticApplication_h
#define DiagnosticApplication_h

#include "DataCollector.h"
#include "version/version.h"
#include <QString>
#include <QStringList>
#include <QApplication>
#include <QMap>
#include <QPair>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include "EbisuTranslator.h"

QString const PROD_URL    = "https://er.dm.origin.com/er/errorreport.xml";

class ErrorHandler;
class DataCollector;
class Network;
class ReportErrorView;

namespace EA { namespace IO { class IniFile; } }

namespace Origin {
    namespace UIToolkit {
        class OriginWindow;
    };
};
enum categoryOrder {
    CHOOSE_CATEGORY,
    ACHIEVEMENTS,
    CHAT_FRIENDS,
    CLOUD_STORAGE,
    DOWNLOADING_GAMES,
    GAME_UPDATES,
    INSTALLING_GAMES_FROM_DISCS,
    LAUNCHING_GAMES,
    LAUNCHING_ORIGIN,
    LOGIN,
    MY_ACCOUNT_PROFILE,
    MY_GAMES_LIBRARY,
    OFFLINE_MODE,
    ORIGIN_CLIENT_UPDATES,
    ORIGIN_HELP,
    ORIGIN_IN_GAME,
    ORIGIN_STORE,
    REDEEM_PRODUCT_CODE,
    REGISTRATION,
    REPAIRING_GAMES,
    UNINSTALLING_GAMES,
    OTHER,
    CRASH_HANG
};

enum downloadErrorComponents {
    EJOB,
    ESYS,
    OFFERID
};

class DiagnosticApplication : public QApplication
{
	Q_OBJECT

public:
	DiagnosticApplication( int argc, char** argv);
	~DiagnosticApplication();

	QString determineLocale();
	QString const& getLocale();

	bool hasIniValue(QString const& key, QString const& section = "OriginER");
	QString getIniValue(QString const& key, QString const& section = "OriginER");

	int exec();

    QString m_cmdline_source;
    QString m_download_ejob;
    QString m_download_esys;
    QString m_download_offerid;
    QString m_cmdline_sub;

public slots:
	void onSubmitReport();
    void onReportResult();
    void onCancelSubmit();

private slots:
    void onProcessingDiagnosticInformation();

private:
	/**
	OER reads the OriginER section from EACore.ini and respects the following settings:
		URL: the URL used for the initial request for a cloud save URL
			default: PROD_URL as defined above
			dev environment: https://dev.er.dm.origin.com/er/errorreport.xml
			qa environment: https://qa.er.dm.origin.com/er/errorreport.xml
			fc.qa environment: https://fc.qa.er.dm.origin.com/er/errorreport.xml

		ValidatePeer: enable verification of the server certificate, see 
			QSslConfiguration::setPeerVerifyMode(). Some of the environments above
			lack properly signed certs, allowing for testing of how the tool responds
			to that.
			default: true

		FailRequest: simulate failure of initial request for an S3 URL
			default: false

		FailUpload: simulate failure of the upload to S3
			default: false

		DumpData: enable writing of S3 upload data to local file named <tracking#>.gzip
			default: false

		PopUp: enable a popup message containing the current settings pulled from the ini file, 
			also get popup message with detailed error information
			default: false

	Example
	[OriginER]
	URL=https://qa.er.dm.origin.com/er/errorreport.xml
	PopUp=true
	*/
	EA::IO::IniFile* getIniFileReader();
	EbisuTranslator* getTranslator();
	ErrorHandler* getErrorHandler();
	ReportErrorView* getMainView();
	DataCollector* getDataCollector();
	Network* getNetwork();
	// returns a map from localized to unlocalized problem identifiers
	QStringList const& createCategoryOrderMapping();
	QStringList const& getProblemAreasList();
    const QList<QPair<QString,int> > getCategoryOrder();

	void addProblemArea(const QString& localized);
    bool SetAppLanguage(const QString &locale, bool updateQSS = true);
    void applyStylesheet();
    bool GetStyleSheetFromFile(const QString& styleSheetPath, QString& styleSheet);
    void writeReportIdToFile(QString& reportId);
    bool isOerFileExist();
    void writeIdToFile(QString& reportId);
    void createIdFile();
    const QString parseCommandLineForValue(const QStringList& cmdLineList, const QString& key);
    void runDiagnosticThread(DataCollectedType type);
	QString getVersion() const { return QString(EBISU_VERSION_STR).replace(",", "."); }

	EA::IO::IniFile* m_iniFileReader;
	ErrorHandler* m_errorHandler;
	ReportErrorView* m_mainView;
    Origin::UIToolkit::OriginWindow* m_mainViewWindow;
	DataCollector* m_collector;
	Network* m_network;
    QFutureWatcher<DataCollectedType>* m_watcher;
    QString m_cmdline_originId;

	EbisuTranslator*      mTranslator;
	QString 		      mLocale;
	QStringList			  mProblemList;
    QString               mDataFolder;
    bool                  mCancelSubmit;
    QList<QPair<QString,int> > mCategoryOrder;

};

#endif
