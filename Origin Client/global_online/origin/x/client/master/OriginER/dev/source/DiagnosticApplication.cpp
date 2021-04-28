///////////////////////////////////////////////////////////////////////////////
// DiagnosticApplication.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "DiagnosticApplication.h"
#include "ReportErrorView.h"
#include "AppRunDetector.h"
#include "Network.h"
#include "ErrorHandler.h"
#include "EAIniFile.h"
#include "Services/debug/DebugService.h"
#include <windows.h>
#include <string>
#include <QDateTime>
#include <QTextStream>
#include <QStringList>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include "TelemetryAPIDLL.h"
#include "OriginCommonUI.h"
#include "originlabel.h"
#include "Qt/originwindow.h"
#include "Qt/originscrollablemsgbox.h"
#include "Qt/originmsgbox.h"
#include "Services/qt/QtUtil.h"

QString const OER_REPORT_ID = "OER.txt";

DiagnosticApplication::DiagnosticApplication(int argc, char** argv)
: QApplication(argc, argv)
, m_iniFileReader(0)
, m_errorHandler(0)
, m_mainView(0)
, m_mainViewWindow(0)
, m_collector(0)
, m_network(0)
, m_watcher(0)
, mTranslator(0)
, mLocale("en_US")
, mDataFolder("")
{
    setQuitOnLastWindowClosed(false);
	QApplication::setOrganizationName("ea.com");
	QApplication::setApplicationName ("OriginER");
	QApplication::setApplicationVersion(getVersion());
    QApplication::setWindowIcon(QIcon(":/Resources/icon.png"));
    m_cmdline_originId = parseCommandLineForValue(QApplication::arguments(),"-originId");
    m_cmdline_source = parseCommandLineForValue(QApplication::arguments(),"-source");
    QString cmdline_category = parseCommandLineForValue(QApplication::arguments(),"-category");
    m_cmdline_sub = parseCommandLineForValue(QApplication::arguments(), "-sub");
    if(cmdline_category == "")
        cmdline_category = "0";
    if(m_cmdline_source == "")
        m_cmdline_source = "executable";
    const QString cmdline_description = parseCommandLineForValue(QApplication::arguments(),"-description");

    // Check if this was a download error. QString 5 is the integer value of a download error, given by const QList<QPair<QString,int> > DiagnosticApplication::getCategoryOrder()
    // If more type of errors are introduced, then the downloader call to OER must update it's number
    if(cmdline_category == QString("5"))
    {
        // Ensure that the description has 3 elements to parse out, delimited by &
        if(cmdline_description.count("&") >= int(OFFERID))
        {
            // Parse out ejob, esys, offer ID. Expected input of cmdline_description on a download error is "ejob:esys:offerid"
            // Delimiter set to & to avoid collision with offer IDs
            QStringList downloadErrorComponents = cmdline_description.split("&");
            m_download_ejob = downloadErrorComponents[EJOB];
            m_download_esys = downloadErrorComponents[ESYS];
            m_download_offerid = downloadErrorComponents[OFFERID];
        }
        else
        {
            m_download_ejob = "";
            m_download_esys = "";
            m_download_offerid = "";
        }

    }

    bool launchedFromClient;
    if(m_cmdline_originId.isEmpty())
        launchedFromClient = 0;
    else
        launchedFromClient = 1;

    // create the main window
    QString currentLocale = determineLocale();
    Q_INIT_RESOURCE(resources);
    OriginCommonUI::initCommonControls();
    OriginCommonUI::initFonts();
    OriginCommonUI::changeCurrentLang(currentLocale);
    SetAppLanguage(currentLocale);

    m_mainView = new ReportErrorView(currentLocale, m_cmdline_originId);
    using namespace Origin::UIToolkit;
    m_mainViewWindow = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
                                        NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok);
    m_mainViewWindow->setTitleBarText(tr("diag_tool_form_title"));
    m_mainViewWindow->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, tr("diag_tool_report_title"), "");
    m_mainViewWindow->setButtonText(QDialogButtonBox::Ok, tr("diag_tool_form_button_submit"));
    OriginLabel* verLbl = new OriginLabel();

    // EBISU_CHANGELIST_STR is a string environment var that is defined in the build system
    // for display beside the build version. This is the only place it is used.
    QString version = getVersion().append(" - " EBISU_CHANGELIST_STR);

    // the label always contains the necessary %1 elements after the retranslate call above
    verLbl->setText(QString(tr("ebisu_client_version")).arg(version));
    verLbl->setLabelType(OriginLabel::DialogSmall);
    m_mainViewWindow->setCornerContent(verLbl);
    m_mainViewWindow->setContent(m_mainView);
    ORIGIN_VERIFY_CONNECT(m_mainViewWindow, SIGNAL(rejected()), this, SLOT(quit()));
    ORIGIN_VERIFY_CONNECT_EX(m_mainView, SIGNAL(submitReport()), this, SLOT(onSubmitReport()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(m_mainView, SIGNAL(cancelSubmit()), this, SLOT(onCancelSubmit()), Qt::QueuedConnection);
    OriginPushButton *submitButton = m_mainViewWindow->button(QDialogButtonBox::Ok);

    // TODO: Review code structure. Check Katherine's CL 246011 
    m_mainView->setSubmitButton(submitButton);
    submitButton->setEnabled(false);
    ORIGIN_VERIFY_CONNECT(submitButton, SIGNAL(clicked()), m_mainView, SLOT(onSubmitClicked()));

    getDataCollector()->setLocale(mLocale);
    ORIGIN_VERIFY_CONNECT( getNetwork(), SIGNAL(reportResult()), this, SLOT(onReportResult()));

    m_mainView->setProblemAreas(getProblemAreasList(), getCategoryOrder());
    m_mainView->setCurrentCategory(cmdline_category.toInt());

    if(cmdline_description.count())
    {
        m_mainView->setDescription(tr("diag_tool_internal_reference_code_dont_delete").arg(cmdline_description) + "<br>");
    }
    m_mainViewWindow->adjustSize();

    OriginTelemetry::init("oer_");
    //No Dyncamic config overrides for OER. Just start sending.
    GetTelemetryInterface()->startSendingTelemetry();

    //  TELEMETRY:  Initialize with client version string
    GetTelemetryInterface()->Metric_OER_LAUNCHED(launchedFromClient);

}

DiagnosticApplication::~DiagnosticApplication()
{
    if(m_mainViewWindow)
    {
        m_mainViewWindow->deleteLater();
        m_mainViewWindow = NULL;
    }
	delete mTranslator;
	delete m_iniFileReader;
	delete m_errorHandler;
	delete m_collector;
	delete m_network;
}

QString DiagnosticApplication::determineLocale()
{
    // Set up our locale right away so we have access to our string db
    QString sBestLocale;
    //Get the available translations
    QDir dir(":/Resources/lang/", "OriginERStrings*.xml");

    QStringList files = dir.entryList(QDir::Files|QDir::Readable);

    QStringList langList;

    //Create supported language array - it is stripping out the "EbisuStrings_"
    // part so if you ever change the file prefix, this has to change as well
    foreach(QString sFile, files)
    {
        langList.append(sFile.mid(16, 5));
    }

    const QString sSystemLocale = QLocale::system().name();

    if ( sSystemLocale == "en_CA" || sSystemLocale == "")
    {
        // if sSystemLocale == "en_CA" Qt will map this to en_GB but we want the NA store and don't mind en_US text
        // if sSystemLocale == "" we need to fallback on a universal locale
        sBestLocale = "en_US";
    }
    else if (sSystemLocale.startsWith("zh") && (sSystemLocale != "zh_CN"))
    {
        // zh_TW is the locale we use for all Chinese that isn't mainland (Simplified) Chinese.
        sBestLocale = "zh_TW";
    }
    else
    {
        sBestLocale = Origin::Services::getNearestLocale(langList);
    }

    return sBestLocale;
}

QString const& DiagnosticApplication::getLocale()
{
	return mLocale;
}

EbisuTranslator* DiagnosticApplication::getTranslator()
{
	if (!mTranslator)
	{
		mTranslator = new EbisuTranslator;
	}
	return mTranslator;
}

bool DiagnosticApplication::SetAppLanguage(const QString &locale, bool updateQSS /* = true */)
{
	// Try and load the language
	if (getTranslator()->loadHAL(":/Resources/lang/OriginERStrings_", locale))
		mLocale = locale;

	installTranslator(getTranslator());

	if (updateQSS)
		applyStylesheet();

	return true;
}

void DiagnosticApplication::applyStylesheet()
{
	QString sStyleSheet;
	bool bStyleSheetFound = GetStyleSheetFromFile(":/Resources/styles.qss", sStyleSheet);

	// Open language specific file, if it exists
	QString strStyleLangFile = ":/Resources/styles_";
	strStyleLangFile.append(mLocale);
	strStyleLangFile.append(".qss");
	QString  sStyleSheetLang;

	if (GetStyleSheetFromFile(strStyleLangFile, sStyleSheetLang))
		sStyleSheet.append(sStyleSheetLang);

	if(bStyleSheetFound)
		setStyleSheet(sStyleSheet);
}

bool DiagnosticApplication::GetStyleSheetFromFile(const QString& styleSheetPath, QString& styleSheet)
{
	bool bStyleSheetFound = false;

	QFile file(styleSheetPath);

	if(file.open(QFile::ReadOnly))
	{
		styleSheet = QString(file.readAll());
		file.close();
		bStyleSheetFound = true;
	}
	return bStyleSheetFound;
}

// returns a list of items, properly sorted, with the default entry first and the 'other' entry last
QStringList const& DiagnosticApplication::getProblemAreasList()
{
	if ( mProblemList.empty() )
		createCategoryOrderMapping();

	return mProblemList;
}

void DiagnosticApplication::addProblemArea(const QString& localized)
{
    mProblemList << localized;
}

QStringList const& DiagnosticApplication::createCategoryOrderMapping()
{
	if ( mProblemList.empty() ) 
	{
		// add entries to the list. Entry order is determined by order of mCategoryOrder
        QList<QPair<QString,int> > categoryOrder = getCategoryOrder();
        QList<QPair<QString,int> >::iterator i;
        for(i = categoryOrder.begin(); i != categoryOrder.end(); ++i)
        {
            QPair<QString,int> categoryValue = *i;
            addProblemArea(categoryValue.first.toStdString().c_str());
        }
	}
	return mProblemList;
}

const QList<QPair<QString,int> > DiagnosticApplication::getCategoryOrder()
{
    if(mCategoryOrder.isEmpty())
    {
        // New categories should be added here
        // Second value in the QPair is for backend reporting. OER submits the category string as an integer, so
        // each localized string maps to an integer and remapped back to string on Rawbox backend.
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_choose_one")),(int)CHOOSE_CATEGORY));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_achievements")),(int)ACHIEVEMENTS));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_social")),(int)CHAT_FRIENDS));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_cloud")),(int)CLOUD_STORAGE));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_crash")),(int)CRASH_HANG));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_downloading_games")),(int)DOWNLOADING_GAMES));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_game_updates")),(int)GAME_UPDATES));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_install_disc")),(int)INSTALLING_GAMES_FROM_DISCS));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_launching_games")),(int)LAUNCHING_GAMES));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_launching_origin")),(int)LAUNCHING_ORIGIN));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_login")),(int)LOGIN));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_profile")),(int)MY_ACCOUNT_PROFILE));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_my_games")),(int)MY_GAMES_LIBRARY));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_offline")),(int)OFFLINE_MODE));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_app_updates")),(int)ORIGIN_CLIENT_UPDATES));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_cs")),(int)ORIGIN_HELP));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_igo")),(int)ORIGIN_IN_GAME));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_store")),(int)ORIGIN_STORE));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_code")),(int)REDEEM_PRODUCT_CODE));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_registration")),(int)REGISTRATION));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_repair")),(int)REPAIRING_GAMES));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_uninstalling")),(int)UNINSTALLING_GAMES));
        mCategoryOrder.append(qMakePair(QString(tr("diag_tool_form_error_dropdown_other")),(int)OTHER));
    }
    return mCategoryOrder;
}

void DiagnosticApplication::onSubmitReport()
{
    mCancelSubmit = false;
    runDiagnosticThread(CLIENT_LOG);
}
void DiagnosticApplication::runDiagnosticThread(DataCollectedType type)
{
    m_watcher = new QFutureWatcher<DataCollectedType>();
    DataCollector *dCollector = getDataCollector();

    ORIGIN_VERIFY_CONNECT(m_watcher, SIGNAL(finished()), this, SLOT(onProcessingDiagnosticInformation()));
    ORIGIN_VERIFY_CONNECT(m_watcher, SIGNAL(canceled()), this, SLOT(onProcessingDiagnosticInformation()));

    if(dCollector)
    {
        switch(type)
        {
        case CLIENT_LOG:
            {
                QFuture<DataCollectedType> future = QtConcurrent::run(dCollector, &DataCollector::dumpClientLogs);
                m_watcher->setFuture(future);
                break;
            }
        case BOOTSTRAPPER_LOG:
            {
                QFuture<DataCollectedType> future = QtConcurrent::run(dCollector, &DataCollector::dumpBootStrapperLogs);
                m_watcher->setFuture(future);
                break;
            }
        case HARDWARE:
            {
                QFuture<DataCollectedType> future = QtConcurrent::run(dCollector, &DataCollector::dumpHardwareLogs);
                m_watcher->setFuture(future);
                break;
            }
        case DXDIAG_DUMP:
            {
                QFuture<DataCollectedType> future = QtConcurrent::run(dCollector, &DataCollector::dumpDxDiagLogs);
                m_watcher->setFuture(future);
                break;
            }
        case READ_DXDIAG:
            {
                QFuture<DataCollectedType> future = QtConcurrent::run(dCollector, &DataCollector::readDxDiagLogs);
                m_watcher->setFuture(future);
                break;
            }
		case NETWORK:
			{
				m_watcher->deleteLater();
				m_watcher=NULL;
				// Create networking QThread
				Network* network = getNetwork();
				QThread *newThread = new QThread;
				network->moveToThread(newThread);
				ORIGIN_VERIFY_CONNECT(newThread, SIGNAL(started()), network, SLOT(requestSignedURL()));
				ORIGIN_VERIFY_CONNECT(network, SIGNAL(finished()), newThread, SLOT(quit()));
				ORIGIN_VERIFY_CONNECT(newThread, SIGNAL(finished()), newThread, SLOT(deleteLater()));
				newThread->start();
				break;
			}
        }
    }
}

void DiagnosticApplication::onCancelSubmit()
{
    mCancelSubmit = true;
}

void DiagnosticApplication::onProcessingDiagnosticInformation()
{
    DataCollectedType result = TOTAL;
    if(m_watcher)
    {
        result = m_watcher->future().result();
        ORIGIN_VERIFY_DISCONNECT(m_watcher,SIGNAL(finished()),0,0);
        ORIGIN_VERIFY_DISCONNECT(m_watcher,SIGNAL(canceled()),0,0);
        m_watcher->deleteLater();
        m_watcher=NULL;
    }

    if (mCancelSubmit || result == TOTAL) 
    {
        m_mainView->onHideProgress();
        m_mainView->setSending(false);
    }
    else 
    {
        switch(result)
        {
            case CLIENT_LOG:
                {
                    runDiagnosticThread(BOOTSTRAPPER_LOG);
                    break;
                }
            case BOOTSTRAPPER_LOG:
                {
                    runDiagnosticThread(HARDWARE);
                    break;
                }
            case HARDWARE:
                {
                    runDiagnosticThread(DXDIAG_DUMP);
                    break;
                }
            case DXDIAG_DUMP:
                {
                    runDiagnosticThread(READ_DXDIAG);
                    break;
                }
            case READ_DXDIAG:
                {
					m_mainView->setProgressCancelButton(false);
                    m_mainView->updateProgressWindow(READ_DXDIAG,NETWORK);
                    getErrorHandler()->clear();
					runDiagnosticThread(NETWORK);
                    break;
                }
        }
    }
}

void DiagnosticApplication::onReportResult()
{
    QPoint windowPos = m_mainViewWindow->pos();
    windowPos.setX(windowPos.x() + 10);
    windowPos.setY(windowPos.y() + 100);
    if (getErrorHandler()->isOk())
    {
        m_mainView->onHideProgress();
        m_mainViewWindow->accept(); // close the main dialog since the request succeeded
        m_mainViewWindow = NULL;

        QString reportId = getNetwork()->getTrackingNumber();
        writeReportIdToFile(reportId);

        GetTelemetryInterface()->Metric_OER_REPORT_SENT(reportId.toUtf8());

        const QString text = QString("<p>%0</p><p>%1</p><p>%2</p>")
                             .arg(tr("diag_tool_message"))
                             .arg(tr("diag_tool_confirm_report_id").arg("<b>"+reportId+"</b>"))
                             .arg(tr("diag_tool_confirm_records"));
        using namespace Origin::UIToolkit;
        OriginWindow* window = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
            NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Close);
        window->scrollableMsgBox()->setup(OriginScrollableMsgBox::NoIcon, tr("diag_tool_success_title"), text);
        ORIGIN_VERIFY_DISCONNECT(window, SIGNAL(closeWindow()), window, SIGNAL(rejected()));
        ORIGIN_VERIFY_CONNECT(window->button(QDialogButtonBox::Close), SIGNAL(clicked()), window, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(closeWindow()), window, SLOT(reject()));

        if(window && window->scrollableMsgBox())
        {
            window->setTitleBarText(tr("diag_tool_form_title"));
            window->scrollableMsgBox()->getTextLabel()->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse|Qt::TextBrowserInteraction|Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);
            ORIGIN_VERIFY_CONNECT(window, SIGNAL(finished(int)), this, SLOT(quit()));
            window->showWindow(windowPos);
            window->adjustSize();
            QApplication::alert(window);
        }
    }
    else
    {
        if ( getIniValue("PopUp").toUpper() == "TRUE" )
        {
            QMessageBox::information(0, "OriginER error", getErrorHandler()->getAllErrors().join("\n") + "\nThis box is shown because the file EACore.ini contains the key PopUp");
        }
        // Keep the main dialog open so they can close the error dialog and retry
        m_mainView->setSending(false);
        m_mainView->show();
        m_mainView->activateWindow();
        m_mainView->raise();
        using namespace Origin::UIToolkit;
        OriginWindow::alert(OriginMsgBox::Error, tr("diag_tool_error_title"), tr("diag_tool_error_text"), tr("diag_tool_form_button_close"),
            tr("diag_tool_form_title"), windowPos);

        GetTelemetryInterface()->Metric_OER_ERROR();

    }
    OriginTelemetry::release();
}

void DiagnosticApplication::writeReportIdToFile(QString& reportId)
{
    mDataFolder = getDataCollector()->getDataFolder();
    if( !isOerFileExist() )
        createIdFile();
    writeIdToFile(reportId);
}

bool DiagnosticApplication::isOerFileExist()
{
    QFileInfoList files = QDir(mDataFolder).entryInfoList();
    foreach(QFileInfo file, files)
    {
        QString filename = file.fileName();
        if(file.fileName().contains(OER_REPORT_ID,Qt::CaseInsensitive))
            return true;
    }
    return false;
}

void DiagnosticApplication::writeIdToFile(QString& reportId)
{
    QFile file(mDataFolder + OER_REPORT_ID);
    file.open(QIODevice::Append);
    QTextStream stream(&file);
    QDateTime timestamp;
    stream << "TS: " << timestamp.currentDateTime().toString("MM-dd-yyyy hh:mm:ss ap") << "   ID: " << reportId << "\n";
    file.close();
}

void DiagnosticApplication::createIdFile()
{
    QFile file(mDataFolder + OER_REPORT_ID);
    file.open(QIODevice::WriteOnly);
    file.close();
}

EA::IO::IniFile* DiagnosticApplication::getIniFileReader()
{
	if ( !m_iniFileReader ) 
	{
		QString path = QCoreApplication::applicationDirPath() + QDir::separator() + "EACore.ini";
		m_iniFileReader = new EA::IO::IniFile((const char16_t*)path.data());
	}
	return m_iniFileReader;
}

bool DiagnosticApplication::hasIniValue(QString const& key, QString const& section)
{
	EA::IO::String16 value;
	int status = getIniFileReader()->ReadEntry( 
		(const char16_t*)section.data(), 
		(const char16_t*)key.data(), 
		value 
	);
	return status >= 0;
}

QString DiagnosticApplication::getIniValue(QString const& key, QString const& section)
{
	EA::IO::String16 value;
	int status = getIniFileReader()->ReadEntry( 
		(const char16_t*)section.data(), 
		(const char16_t*)key.data(), 
		value 
	);
	if (status > 0)
		return QString::fromWCharArray( value.c_str() );

	return "";
}

ErrorHandler* DiagnosticApplication::getErrorHandler()
{
	if ( !m_errorHandler )
	{
		m_errorHandler = new ErrorHandler();
	}
	return m_errorHandler;
}

DataCollector* DiagnosticApplication::getDataCollector()
{
	if ( !m_collector )
	{
		m_collector = DataCollector::instance();
		m_collector->setReportView(m_mainView);
		m_collector->setErrorHandler(getErrorHandler());
	}
	return m_collector;
}

Network* DiagnosticApplication::getNetwork()
{
	if ( !m_network )
	{
		m_network = new Network(this);
		m_network->setErrorHandler(getErrorHandler());
		m_network->setDataCollector(getDataCollector());
        Q_ASSERT(m_mainView);
		m_network->setReportView(m_mainView);
	}
	return m_network;
}

int DiagnosticApplication::exec()
{
    if(m_mainViewWindow)
        m_mainViewWindow->showWindow();
	return QApplication::exec();
}

const QString DiagnosticApplication::parseCommandLineForValue(const QStringList& cmdLineList, const QString& key)
{
    // Iterate through the string list
    // Example command line: -originId ceejay
    for (int i = 0; i < cmdLineList.size(); i++)
    {
        QString keyValueString = cmdLineList[i];
        // Check to see if current parameter contains key. If it does, then return the next element which is value.
        if(keyValueString.contains(key) && cmdLineList.size() > i+1)
            return cmdLineList[i+1];
    }
    // Did not make a relevant match for key
    return QString("");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow)
{
    int argCount;
    LPWSTR *argList = CommandLineToArgvW(GetCommandLine(), &argCount);
    LPSTR *args = new LPSTR[argCount];
    // Convert arguments to LPSTR
    for (int iter = 0; iter < argCount; ++iter)
    {
        size_t qStrLen = wcslen(argList[iter]), qConverted = 0;
        args[iter] = new CHAR [qStrLen+1];
        wcstombs_s(&qConverted,args[iter],qStrLen+1, argList [iter],qStrLen+1);
    }

    DiagnosticApplication app(argCount, args);

    for (int i = 0; i < argCount; i++)
        delete[] args[i];
    delete[] args;
    LocalFree(argList);

    EA::AppRunDetector appRunDetector(L"OriginER", L"Qt5QWindowIcon", L"Origin Error Reporter", L"Qt5QWindowIcon", hInstance);
    if(appRunDetector.IsAnotherInstanceRunning())
    {
        appRunDetector.ActivateRunningInstance(std::wstring(GetCommandLine()), true);
        return true;
    }
    return app.exec();
}
