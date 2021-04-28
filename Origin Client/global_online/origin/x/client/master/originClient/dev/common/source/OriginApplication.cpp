///////////////////////////////////////////////////////////////////////////////
// OriginApplication.cpp
//
// Copyright (c) 2010-2012, Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
#include <shlobj.h>
#include <shobjidl.h>
#endif

#include "common/source/OriginApplication.h"
#if defined(ORIGIN_MAC)
#include "common/source/OriginApplicationDelegateOSX.h"
#include "Qt/origintitlebarosx.h"
#endif

// USING_XP
#if defined(ORIGIN_PC)
#include "Dbt.h"
#include "services/voice/VoiceDeviceService.h"
#endif

#include "services/Services.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/network/GlobalConnectionStateMonitor.h"
#include "services/connection/ConnectionStatesService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/network/ClientNetworkConfigReader.h"
#include "services/platform/PlatformService.h"
#include "services/crypto/CryptoService.h"
#include "services/config/OriginConfigService.h"
#include "services/rest/GeoCountryClient.h"
#include "services/session/SessionService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformJumplist.h"
#include "services/escalation/IEscalationClient.h"
#include "services/platform/EnvUtils.h"
#include "services/translator/OriginTranslator.h"
#include "services/heartbeat/Heartbeat.h"
#include "services/qt/QtUtil.h"
#include "services/voice/VoiceService.h"

#include "engine/Engine.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/igo/IGOController.h"
#include "engine/login/LoginController.h"

#include "LocalHost/localHost.h"
#include "LocalHost/LocalHostOriginConfig.h"
#include "LSX/LSX.h"
#include "LSX/LsxServer.h"
#include "Service/SDKService/SDK_ServiceArea.h"
#include "version/version.h"
#include "Qt/OriginCommonUI.h"
#include "EbisuSystemTray.h"
#include "TelemetryAPIDLL.h"
#include "TelemetryManager.h"
#include "WebWidgetController.h"
#include "DialogController.h"

#include "views/source/PlayView.h"
#include "views/igo/source/IGOUIFactory.h"
#include "widgets/client/source/ConfigIngestViewController.h"
#include "engine/config/ConfigIngestController.h"

#include <QCryptographicHash>
#include <QDomDocument>
#include <QFileInfo>
#include <QRegExp>
#include <QDir>
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QWindow>
#include <QLocale>
#include <QMetaObject>
#include <QWidget>
#include <QDebug>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QStringBuilder> 
#include <QFileOpenEvent>

#include "flows/PendingActionFlow.h"

#include "flows/CommandFlow.h"
#include "utilities/CommandLine.h"

#if defined(ORIGIN_PC)
#include <windows.h>
#include <ShellAPI.h>
#endif

#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAIniFile.h>
#include <EATrace/EATrace.h>
#include <EATrace/EALog_imp.h>
#include <EATrace/EALogConfig.h>
#include <EAStdC/EAProcess.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAStdC/EATextUtil.h>
#include <EASTL/string.h>

#if defined(ORIGIN_PC)
#include "src/harfbuzz.h" // This is from $(QTDIR)\src\3rdparty\harfbuzz
#include "IGOSharedStructs.h"
#include <richedit.h>
#endif

#include "cryptssc2.h"

#include "Qt/originwindow.h"
#include "Qt/originmsgbox.h"
#include "Qt/originscrollablemsgbox.h"
#include "Qt/origincheckbutton.h"
#include "Qt/originpushbutton.h"

#ifdef ORIGIN_MAC // for re-launching(restart) the client on Mac
#include <libproc.h>
#include <objc/objc.h>
#include <objc/message.h>
#include <execinfo.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#include "ClientFlow.h"
#include "SocialViewController.h"
#include "ChatWindowManager.h"

using namespace Origin::Engine;

#define WM_COMMANDLINE_ACCEPTED WM_USER + 100

#if defined(EATEXT_PRESENT)
namespace EA
{
    namespace Text
    {
        // Custom Thai dictionary characters
        Q_DECL_IMPORT void InitWordBreakDictionary();
        Q_DECL_IMPORT void ShutdownWordBreakDictionary();
        Q_DECL_IMPORT void AddWordBreakDictionaryEntry(const char16_t* pWord);
    }
}
#endif
extern bool g_bStartNotAllowed;     // if this variable is true, try to close the app as soon as the message boxes are up and running
                                    // is will also prevent wineventfilter from running, to prevent crashes of uninitialized classes
extern uint64_t g_startupTime;


const int OriginApplication::SENDHOOKTHRESHOLD = 3;

#if defined(ORIGIN_PC)
// there's double click close message that is not QtWndProc SC_CLOSE
#define SC_CLOSE2 0xF063
#endif

// Global Accessor to parameter
bool ForceLockboxURL(){ return OriginApplication::instance().forceLockboxURL(); }

/* make environment name globally visible */
const QString& GetEnvironmentName()
{
    return OriginApplication::instance().GetEnvironmentName();
}


#ifdef ORIGIN_MAC
// Create a handler for the dock Icon clicked Event
bool dockClickHandler(id self, SEL _cmd)
{
    Q_UNUSED(self);
    Q_UNUSED(_cmd);
    ((OriginApplication*)qApp)->onClickDock();
    return true;
}

bool dockQuitHandler(id self, SEL _cmd)
{
    Q_UNUSED(self);
    Q_UNUSED(_cmd);
    ((OriginApplication*)qApp)->onDockQuit();
    return false;
}
#endif


// make supported locales available
QMap <QString, QString> OriginApplication::mCustomerSupportLocales;

//#define TEST_ESCALATION_CLIENT
bool testEscalationClient()
{
#ifdef TEST_ESCALATION_CLIENT
    using namespace Origin::Escalation;

    QString escalationReasonStr = "testEscalationClient";
    int escalationError = 0;
    QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient;
    if (!Origin::Escalation::IEscalationClient::quickEscalate(escalationReasonStr, escalationError, escalationClient))
        return false;

    escalationClient->setRegistryStringValue(IEscalationClient::RegSz, HKEY_LOCAL_MACHINE, "Software\\Origin","IsBeta", "KAKALOKOTA");
    if(Origin::Escalation::kCommandErrorNone != escalationClient->getError())
        return false;

     escalationClient->setRegistryStringValue(IEscalationClient::RegMultiSz, HKEY_LOCAL_MACHINE, "Software\\PERRO_DEL_INFIERNO","Multilinea", "KAKALOKOTA\\0KAKALOKOTA\\0KAKALOKOTA\\0");
     if(Origin::Escalation::kCommandErrorNone != escalationClient->getError())
         return false;
 
     escalationClient->setRegistryStringValue(IEscalationClient::RegExpandSz, HKEY_LOCAL_MACHINE, "Software\\PERRO_DEL_INFIERNO","Path", "%PATH%");
     if(Origin::Escalation::kCommandErrorNone != escalationClient->getError())
         return false;
 
     escalationClient->setRegistryIntValue(IEscalationClient::RegDWord, HKEY_LOCAL_MACHINE, "Software\\PERRO_DEL_INFIERNO","THIS IS A DWORD", 25255);
     if(Origin::Escalation::kCommandErrorNone != escalationClient->getError())
         return false;
 
     escalationClient->setRegistryIntValue(IEscalationClient::RegQWord, HKEY_LOCAL_MACHINE, "Software\\PERRO_DEL_INFIERNO","THIS IS A QWORD", 72057594037927935);
     if(Origin::Escalation::kCommandErrorNone != escalationClient->getError())
         return false;
 
     escalationClient->setRegistryIntValue(IEscalationClient::RegDWord, HKEY_LOCAL_MACHINE, "Software\\PERRO_DEL_INFIERNO","THIS IS A DWORD", -123);
     if(Origin::Escalation::kCommandErrorNone != escalationClient->getError())
         return false;
 
     escalationClient->deleteRegistrySubKey(HKEY_LOCAL_MACHINE, "Software\\PERRO_DEL_INFIERNO");
     if(Origin::Escalation::kCommandErrorNone != escalationClient->getError())
         return false;
#endif

    return true;
}

// Show error dialog
void popupConfigErrorDialog(const QString& environmentStr)
{
    Origin::UIToolkit::OriginWindow* errorMessage = new Origin::UIToolkit::OriginWindow(
        (Origin::UIToolkit::OriginWindow::TitlebarItems)(Origin::UIToolkit::OriginWindow::Icon | Origin::UIToolkit::OriginWindow::Close),
        NULL, Origin::UIToolkit::OriginWindow::MsgBox, QDialogButtonBox::Close);
    // This dialog is not localized since it can only appear on non-production environments...
    errorMessage->setTitleBarText(QString("Config Service Error - %1").arg(environmentStr));
    errorMessage->setButtonText(QDialogButtonBox::Close, "Close");
    errorMessage->msgBox()->setup(Origin::UIToolkit::OriginMsgBox::NoIcon, "Failed to load or parse Origin Config Payload", 
        QString("This pop-up is visible because Origin is configured with a non-production environment and the config service failed to initialize.\n\nCheck the following:\n\n * %1 is a recognized environment (qa, fc.qa, integration, production, etc)\n * Origin is being run from a directory with the correct permissions\n * Autopatch service functioning correctly on %1 environment.").arg(environmentStr));

    ORIGIN_VERIFY_CONNECT(errorMessage->button(QDialogButtonBox::Close), SIGNAL(clicked()), errorMessage, SLOT(reject()));
    errorMessage->exec();
}
    
#ifdef ORIGIN_PC
LRESULT WINAPI OriginApplication::OriginDetectionWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    QWindowList windows = qApp->topLevelWindows();
    if (!windows.isEmpty())
        SendMessage((HWND) windows.first()->winId(), message, wParam, lParam);  // forward WM messages to the Origin app

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// Helper to check/reset some of the registry entries setup during the initial install.
class InstallChecker
{
public:
    InstallChecker()
        : mEscalationAttempt(false)
    {
    }

    ~InstallChecker()
    {
    }

    // Repair the protocol registry keys if necessary.
    void checkProtocols(const QString& exeFullPath)
    {
    #ifdef DEBUG
        return; // Don't override your local install while debugging/testing
    #else
        // Same set of protocols defined in the Bootstrap/Registration.cpp
        QString protocolNameEadm(QString::fromLatin1("URL:EADM Protocol"));
        QString protocolNameOrigin(QString::fromLatin1("URL:ORIGIN Protocol"));

        if (!registerUrlProtocol(exeFullPath, protocolNameEadm, QString::fromLatin1("eadm")))
        {
            ORIGIN_LOG_ERROR << "Unable to cleanup Url Protocol eadm";
        }

        if (!registerUrlProtocol(exeFullPath, protocolNameOrigin, QString::fromLatin1("origin")))
        {
            ORIGIN_LOG_ERROR << "Unable to cleanup Url Protocol origin";
        }

        if (!registerUrlProtocol(exeFullPath, protocolNameOrigin, QString::fromLatin1("origin2")))
        {
            ORIGIN_LOG_ERROR << "Unable to cleanup Url Protocol origin2";
        }
    #endif
    }

private:
    QSharedPointer<Origin::Escalation::IEscalationClient> mEscalationClient;
    bool mEscalationAttempt;


    QSharedPointer<Origin::Escalation::IEscalationClient> escalationClient()
    {
        if (!mEscalationAttempt)
        {
            mEscalationAttempt = true;

            int errorCode;
            QString reason = QString::fromLatin1("Repair protocol registry keys");
            if (!Origin::Escalation::IEscalationClient::quickEscalate(reason, errorCode, mEscalationClient))
            {
                ORIGIN_LOG_ERROR << "Escalation failed when trying to restore Origin protocols";
                mEscalationClient.reset(NULL);
            }
        }

        return mEscalationClient;
    }

    bool setRegistryStringValue(HKEY registryKey, const QString& registrySubKey, const QString& valueName, const QString& val)
    {
        QSharedPointer<Origin::Escalation::IEscalationClient> client = escalationClient();
        if (!client)
            return false;

        client->setRegistryStringValue(Origin::Escalation::IEscalationClient::RegSz, registryKey, registrySubKey, valueName, val);
        return client->getError() == Origin::Escalation::kCommandErrorNone;
    }

    
    bool registerUrlProtocol(const QString& exeFullPath, const QString& protocolName, const QString& keyName)
    {
        bool b64BitPath = false;
    
        QString value;
        QString rootClassKey = QString::fromLatin1("HKEY_CLASSES_ROOT\\%1\\").arg(keyName);
        bool valid = Origin::Services::PlatformService::registryQueryValue(rootClassKey, value, b64BitPath);
        valid &= value.compare(protocolName, Qt::CaseSensitive) == 0;

        if (!valid)
        {
            if (!setRegistryStringValue(HKEY_CLASSES_ROOT, keyName, "", protocolName))
            {
                ORIGIN_LOG_ERROR << "Failed to reset root default for protocol: " << keyName;
                return false;
            }
    
            else
                ORIGIN_LOG_EVENT << "Root default reset for protocol: " << keyName;
        }


        QString protocolValueName = QString::fromLatin1("URL Protocol");
        value.clear();
        valid = Origin::Services::PlatformService::registryQueryValue(rootClassKey + protocolValueName, value, b64BitPath, true);
        valid &= value.isEmpty();
        
        if (!valid)
        {
            if (!setRegistryStringValue(HKEY_CLASSES_ROOT, keyName, protocolValueName, ""))
            {
                ORIGIN_LOG_ERROR << "Failed to reset root URL protocol for: " << keyName;
                return false;
            }
    
            else
                ORIGIN_LOG_EVENT << "Root URL protocol reset for: " << keyName;
        }


        QString shellSubKey = QString::fromLatin1("shell\\open\\command\\");
        QString shellCommand =QString::fromLatin1("\"%1\" \"%2\"").arg(exeFullPath).arg("%1");
        valid &= Origin::Services::PlatformService::registryQueryValue(rootClassKey + shellSubKey, value, b64BitPath);
        valid &= value.compare(shellCommand, Qt::CaseInsensitive) == 0;

        if (!valid)
        {
            if (!setRegistryStringValue(HKEY_CLASSES_ROOT, QString::fromLatin1("%1\\%2").arg(keyName).arg(shellSubKey), "", shellCommand))
            {
                ORIGIN_LOG_ERROR << "Failed to reset root shell command for protocol: " << keyName;
                return false;
            }
    
            else
                ORIGIN_LOG_EVENT << "Root shell command reset for protocol: " << keyName;
        }

        return true;
    }

}; // InstallChecker

#endif // ORIGIN_PC

OriginApplication::OriginApplication(int &argc, char **argv)
    : QApplication(argc, argv),
#ifdef ORIGIN_PC
    mOriginDetectionWindow(NULL),
#endif
    mArgc(argc),
    mArgv(argv),
    mLocale(),
    mIniFilePath(),
    mLogConfigFilePath(),
    mEnvironmentName("Uninitialized"),
    mExternalLaunchStartupTab(TabNone),
    mServerStartupTab(TabNone),
    mExceptionSystem(),
    mNetworkConfigReadState(ConfigReadPending),
    mLSXThread(NULL),
    mTelemetryManager(new Origin::Client::TelemetryManager),
    mBlockingBehaviorEnabled(false),
    mQADebugLang(false),
    mSingleLoginEnabled(false),
    mForceLockboxURL(false),
    mCrashOnStartup(false),
    mITEActive(false),
    mbLimitedUser(false),
    mWasStartedFromAutoRun(false),
    mHasQuit(false),
    mReceivedQueryEndSession(false),
    mStartupState(StartNormal),
    mbForceException(false),
    mbEnableException(true),
    mTranslateStrings(true),
    mIsFreeToPlay(false)
{
#ifdef ORIGIN_PC    // create a dummy window Origin with class QWidget to maintain compatibility with Qt4 Origin clients

    WNDCLASSEX wc;

    // clear out the window class for use
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    // fill in the struct with the needed information
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = OriginApplication::OriginDetectionWindowProc;
    wc.hInstance = GetModuleHandle(NULL);

    wc.hCursor = 0;
    wc.hbrBackground = 0;
    wc.lpszClassName = EBISU_WINDOW_CLASS;

    RegisterClassEx(&wc);
    
    mOriginDetectionWindow = CreateWindowEx(NULL,
                                                    EBISU_WINDOW_CLASS,    // name of the window class
                                                    EBISU_INTERNAL_WINDOW_TITLE,    // title of the window
                                                    0,    // window style
                                                    0,    // x-position of the window
                                                    0,    // y-position of the window
                                                    0,    // width of the window
                                                    0,    // height of the window
                                                    HWND_MESSAGE,    // message only - otherwise we see heavy slowdowns
                                                    NULL,    // we aren't using menus, NULL
                                                    GetModuleHandle(NULL),    // application handle
                                                    NULL);    // used with multiple windows, NULL

    if (mOriginDetectionWindow)
    {
        disableUIPI(mOriginDetectionWindow, WM_COPYDATA);
        disableUIPI(mOriginDetectionWindow, WM_SYSCOMMAND);
    }
#endif

    QAbstractEventDispatcher::instance()->installNativeEventFilter(this);

    processCommandLineSwitches();

#if !defined (ORIGIN_MAC)
    // don't set this on Mac since it make for an unsual Application Support path
    QCoreApplication::setOrganizationName(QString("Origin"));
#endif
    // Do this very early before we do any HTTP requests as it's used to build our User-Agent
    QCoreApplication::setApplicationName ("Origin");
    QCoreApplication::setApplicationVersion(QString(EBISU_VERSION_STR).replace(",", "."));
    
#ifdef ORIGIN_PC
    // Only Windows needs this to give the app a taskbar icon (because of the bootstrapper)
    setWindowIcon(QIcon(":/origin.png"));
#endif

    setQuitOnLastWindowClosed(false);   // otherwise application will shutdown, if we hit "cancel" in the offline mode dialog
    
    srand( (unsigned)time( NULL ) );

    // This enables assertions to trigger
    EA::Trace::ITracer* const pSavedTracer = EA::Trace::GetTracer();
    Q_UNUSED(pSavedTracer);


#ifdef ORIGIN_PC
    // Set the current bootstrap version in use
    GetTelemetryInterface()->SetBootstrapVersion(qPrintable(EnvUtils::GetOriginBootstrapVersionString()));
#endif
    bool doOriginEarlyExit = !Origin::Services::init(argc, argv);

    GetTelemetryInterface()->Metric_APP_START();

    if (Origin::Services::OriginConfigService::instance()->isConfigOk())
    {
        // Update the telemetry server address and port
        QString telemetryServer = Origin::Services::OriginConfigService::instance()->telemetryConfig().TelemetrySDK14.endPoint;
        GetTelemetryInterface()->setTelemetryServer(telemetryServer.toStdString().c_str());

        // update telemetry api even if the hooks blacklist is not configured
        // in case we pass an empty string, the API will set the default values
        QString telemetryHookBlacklist = Origin::Services::OriginConfigService::instance()->telemetryConfig().HookBlacklist.hooks;
        GetTelemetryInterface()->setHooksBlacklist(telemetryHookBlacklist.toStdString().c_str());

        // update telemetry api even if the countries blacklist is not configured
        // in case we pass an empty string, the API will set the default values
        QString telemetryCountryBlacklist = Origin::Services::OriginConfigService::instance()->telemetryConfig().CountryBlacklist.countries;
        GetTelemetryInterface()->setCountriesBlacklist(telemetryCountryBlacklist.toStdString().c_str());
    }

    GetTelemetryInterface()->setIsBOI(Origin::Services::PlatformService::isBeta());

    //We don't really care if it succeeded or not. Just that it's done. If it failed the telemetry system will use the defaults.
    GetTelemetryInterface()->startSendingTelemetry();

    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::Bootstrap, g_startupTime);
    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_STOP(EbisuTelemetryAPI::Bootstrap);
    GetTelemetryInterface()->RUNNING_TIMER_START(g_startupTime);
    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::ShowLoginPage);
    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::RTPGameLaunch_launching);

    if (doOriginEarlyExit)
    {
        QString environmentStr = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName);
        popupConfigErrorDialog(environmentStr);

        switch (Origin::Services::OriginConfigService::instance()->configurationError())
        {
        default:
        case Origin::Services::NoError:
            break;
        case Origin::Services::ConfigFileIOError:
            {
                QString error = QString("Failed to open: %1").arg(Origin::Services::OriginConfigService::instance()->configFilePath());
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("configServiceFailedOpenFile", error.toLocal8Bit().constData());
                ORIGIN_LOG_ERROR << "Failed to open configuration file [" << Origin::Services::OriginConfigService::instance()->configFilePath() << "], falling back on hard-coded values if on production, otherwise exiting...";
            }
            break;
        case Origin::Services::ParsingError:
            {
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("configServiceFailedParsed", Origin::Services::OriginConfigService::instance()->parsingError().toLocal8Bit().constData());
            }
            break;
        case Origin::Services::DecryptionFailedRijndaelError:
            {
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("configServiceFailedDecryption", Origin::Services::OriginConfigService::instance()->parsingError().toLocal8Bit().constData());
            }
            break;
        case Origin::Services::DecryptionFailedAnyDataError:
            {
                GetTelemetryInterface()->Metric_ERROR_NOTIFY("configServiceFailedDecryption", Origin::Services::OriginConfigService::instance()->parsingError().toLocal8Bit().constData());
            }
            break;
        }
    }

    // Make sure we don't try to ingest a desktop eacore.ini if we're
    // uninstalling!
    bool okToIngest = true;

    // TELEMETRY: Did we just install/uninstall?
    bool justUpdatedOrInstalled = false;
    for (int i = 0; i < mArgc; ++i)
    {
        QString arg(mArgv[i]);
        if (arg.contains("/Installed:"))
        {
            QString version = arg.split(":")[1];
            GetTelemetryInterface()->Metric_APP_INSTALL(version.toLatin1().data());
            justUpdatedOrInstalled = true;
            break;
        }
        if (arg.contains("/Uninstall"))
        {
            QString version = QString(EALS_VERSION_P_DELIMITED);
            GetTelemetryInterface()->Metric_APP_UNINSTALL(version.toLatin1().data());
            doOriginEarlyExit = true;
            okToIngest = false;
            // Services don't get shutdown properly on early exit so shutdown the telemetry service now so that it will send everything.
            OriginTelemetry::release();
    }
    }

    // Make sure to censor appropriate info
    Origin::Services::Logger::Instance()->AddStringToCensorList("achievements");
    Origin::Services::Logger::Instance()->AddStringToCensorList("achievement");
    Origin::Services::Logger::Instance()->AddStringToCensorList("achieve");
    
    if (!Origin::Services::readSetting(Origin::Services::SETTING_LogConsoleDevice).toString().isEmpty())
    {
        Origin::Services::DebugService::createConsole();
    }

    // Get the country information based on IP.
    Origin::Services::GeoCountryResponse *geoResponse = Origin::Services::GeoCountryClient::getGeoAndCommerceCountry();
    ORIGIN_VERIFY_CONNECT(geoResponse, SIGNAL(success()), this, SLOT(onGeoCountryResponse()));
    ORIGIN_VERIFY_CONNECT(geoResponse, SIGNAL(error(Origin::Services::restError)), this, SLOT(onGeoCountryError(Origin::Services::restError)));
    ORIGIN_VERIFY_CONNECT(geoResponse, SIGNAL(finished()), geoResponse, SLOT(deleteLater()));

#ifdef ORIGIN_PC
    // Validate/Repair the registry if necessary
    InstallChecker v;
    v.checkProtocols(QCoreApplication::applicationFilePath().replace("/", "\\"));
#endif

    Origin::Engine::init();
    Origin::Engine::IGOController::init(new Origin::Client::IGOUIFactory());

    // Start watching our settings and logout now that services are initialized.
    mTelemetryManager->setupWatchers();

#ifdef ORIGIN_PC
    testEscalationClient();
#endif

    initLanguageSettings(argc, argv);

    //create the pendingActionFlow -- it handles all the origin2:// calls
    Origin::Client::PendingActionFlow::create();

    //create the commandFlow -- it will exist in parallel with MainFlow and handle
    //all command line processing
    Origin::Client::CommandFlow::create();
    Origin::Client::CommandFlow::instance()->setCommandLine (arguments(), argc, argv);

#if defined(ORIGIN_MAC)
    // strip the psn command-line parameter so that we don't re-use it on a restart
    Origin::Client::CommandFlow::instance()->removeParameter ("-psn", QRegularExpression("\\-psn[0-9_]+"));
#endif
    
    mCommandLine = Origin::Client::CommandFlow::instance()->commandLine();

    // Init our library of Ebisu UI components
    ORIGIN_LOG_EVENT << "Common controls setup";
    OriginCommonUI::initCommonControls();
    OriginCommonUI::initFonts();

    // Check for a new eacore.ini on the desktop, giving the user to update
    // and restart if there is one
    bool ingestionRestart = false;

    if (okToIngest)
    {
        ingestionRestart = ingestDesktopConfigFile();
    }
    if (doOriginEarlyExit || ingestionRestart)
    {
        quit(ReasonEarlyRestart);
#ifdef ORIGIN_PC
        TerminateProcess(GetCurrentProcess(), 0xffffffffL);
#else
        _exit(0xffffffffL);
#endif
        return;
    }

#ifdef ORIGIN_MAC
    // Verify that the user agrees to install the escalation service helper tool, if the user has not already
    // done so.  The user should only have to authorize this once - not every time they run the client.  A system prompt
    // will appear, requesting the user's password.  If the user declines, then an error message box will appear that
    // informs the user that the escalation service helper tool is required in order to proceed with Origin.
    // The user will be given the option to re-try authorization, or to exit the client.
    
    QScopedPointer<Origin::Escalation::IEscalationClient> escalationClient;
    escalationClient.reset(Origin::Escalation::IEscalationClient::createEscalationClient());
    
    while (escalationClient->checkIsRunningElevatedAndUACOK() == Origin::Escalation::kCommandErrorNotElevatedUser)
    {
        ORIGIN_LOG_EVENT << "Request user to allow escalation client install...";
        
        using namespace Origin::UIToolkit;
        OriginWindow* escalationMsgBox  = new OriginWindow(
                                                           (OriginWindow::TitlebarItems)(OriginWindow::Close),
                                                           NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
        escalationMsgBox->msgBox()->setup(OriginMsgBox::NoIcon,
                                          tr("ebisu_client_mac_install_helper_warning_title"),
                                          tr("ebisu_client_install_helper_tool").arg(tr("application_name")));
        OriginPushButton* okBtn = escalationMsgBox->addButton(QDialogButtonBox::Ok);
        okBtn->setText(tr("ebisu_client_mac_install_helper_warning_button"));
        OriginPushButton* quitBtn = escalationMsgBox->addButton(QDialogButtonBox::Close);
        quitBtn->setText(tr("ebisu_client_quit"));
        escalationMsgBox->setDefaultButton(okBtn);
        escalationMsgBox->manager()->setupButtonFocus();
        ORIGIN_VERIFY_CONNECT(escalationMsgBox, SIGNAL(rejected()), escalationMsgBox, SLOT(close()));
        ORIGIN_VERIFY_CONNECT(quitBtn, SIGNAL(clicked()), escalationMsgBox, SLOT(reject()));
        ORIGIN_VERIFY_CONNECT(okBtn, SIGNAL(clicked()), escalationMsgBox, SLOT(accept()));
        if (escalationMsgBox->exec() == QDialog::Rejected)
        {
            quit(ReasonEscalationHelperDeclined);
            _exit(0xffffffffL);
            return;
        }
    }
#endif //ORIGIN_MAC

    // Read EACore.ini file
    readConfiguration();
    
    ORIGIN_LOG_EVENT << "Environment: " << mEnvironmentName;
    
    // Detect Developer Mode
    detectDeveloperMode();

    // Flush the web cache on startup if necessary, so designers
    // don't have to manually delete directories.
    if (Origin::Services::readSetting(Origin::Services::SETTING_FlushWebCache))
    {
        Origin::Services::NetworkAccessManagerFactory::instance()->clearCache();
    }
    
    //  HEARTBEAT:  Initialize with data from INI file
    Origin::Services::Heartbeat::instance()->initialize();
    
    dumpEACoreToLog();

#ifdef ORIGIN_PC 
    //We don't want the SystemTry Icon appearing on a Mac
    // Load icon - different colour for non-production environment
    if(Origin::Services::readSetting(Origin::Services::SETTING_OverridesEnabled).toQVariant().toBool())
    {
        QString envStr = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName, Origin::Services::Session::SessionService::currentSession()).toString();

        // Current design only needs to set the systray icon. We may turn on
        // the taskbar icon as well.
        if (envStr.compare("production", Qt::CaseInsensitive) != 0)
        {
            setWindowIcon(QIcon(":/origin_np.png"));
            Origin::Client::EbisuSystemTray::instance()->showDeveloperSystrayIcon();
        }
    }
#endif

    QObject::installEventFilter(this);
    
#if defined (ORIGIN_MAC)
    Origin::Client::registerOriginProtocolSchemes();
    Origin::Client::interceptSleepEvents();
#endif

    // We're about to start our connection controller, did we request to start origin offline?
    // also check whether we should start minimized
    bool startOffline = false;
    for (int i = 0; i < mArgc; i++)
    {
        if (QString::compare(mArgv[i], "/StartOffline") == 0)
        {
            startOffline = true;
        }
        else if (QString::compare(mArgv[i], "/StartClientMinimized") == 0 || QString::compare(mArgv[i], "-StartClientMinimized") == 0)
        {
            mStartupState = StartInTaskbar;
        }
        else if (QString::compare(mArgv[i], "/StartClientMaximized") == 0 || QString::compare(mArgv[i], "-StartClientMaximized") == 0)
        {
            mStartupState = StartMaximized;
        }
        else if(QString(mArgv[i]).startsWith("/LsxPort:", Qt::CaseInsensitive) || QString(mArgv[i]).startsWith("-LsxPort:", Qt::CaseInsensitive))
        {
            QString val = QString(argv[i]).mid(9);
            Origin::Services::writeSetting(Origin::Services::SETTING_LSX_PORT, val);
        }
    }

    if (startOffline)
        Origin::Services::Network::GlobalConnectionStateMonitor::setUpdatePending (true);

    // Start in system tray if the user has auto start, auto update, and remember me enabled.
    // https://developer.origin.com/documentation/display/EBI/Origin+Quiet+Mode+PRD#OriginQuietModePRD-2.1.AutostartAutoupdateUpdating%5CMandatory%5C
    if(justUpdatedOrInstalled
       && static_cast<bool>(Origin::Services::readSetting(Origin::Services::SETTING_AUTOUPDATE))
       && static_cast<bool>(Origin::Services::readSetting(Origin::Services::SETTING_RUNONSYSTEMSTART))
       && (!Origin::Services::isDefault(Origin::Services::SETTING_REMEMBER_ME_PROD) || !Origin::Services::isDefault(Origin::Services::SETTING_REMEMBER_ME_INT)))
    {
        mStartupState = StartInSystemTray;
    }

    // Start LSX in the background so we can startup in parallel
    ORIGIN_LOG_EVENT << "LSX thread starting";
    startLSXThread();

    bool localHostEnabled = Origin::Services::readSetting(Origin::Services::SETTING_LOCALHOSTENABLED);

    if(localHostEnabled)
    {
        Origin::Sdk::LocalHost::start(new Origin::Sdk::LocalHost::LocalHostOriginConfig());    
    }
    else
    {
        // The responder needs to be disabled also (since it defaults to enabled)
        Origin::Services::writeSetting(Origin::Services::SETTING_LOCALHOSTRESPONDERENABLED, false);

        GetTelemetryInterface()->Metric_LOCALHOST_SERVER_DISABLED();
    }

    initEAText();

    // do any critical checks here, so we can show a UI to the user and do not have to connect to the middleware
    // compared to the old way

    if(g_bStartNotAllowed)
    {
        ORIGIN_LOG_ERROR << "Was not allowed to start, terminating process";
        showAppNotAllowedError();
#ifdef ORIGIN_PC
        TerminateProcess(GetCurrentProcess(), 0xffffffffL);
#else
        exit(0xffffffffL);
#endif
        return;
    }

    if(abortDueToGuestUser())
    {
        ORIGIN_LOG_ERROR << "Aborting due to guest error, terminating process";
        g_bStartNotAllowed=true;
#ifdef ORIGIN_PC
        TerminateProcess(GetCurrentProcess(), 0xffffffffL);
#else
        exit(0xffffffffL);
#endif
        return;
    }
  
    // We discover this from the state of the registry
    Origin::Services::writeSetting(Origin::Services::SETTING_RUNONSYSTEMSTART, Origin::Services::PlatformService::isAutoStart());

    // Set up a global login component
    // XXX: If we set ourselves as our parent we crash during shutdown in the LSX code
    ORIGIN_LOG_EVENT << "Login setup";
    ORIGIN_VERIFY_CONNECT(LoginController::instance(), SIGNAL(userLoggedIn(Origin::Engine::UserRef)), this, SLOT(onUserLoggedIn(Origin::Engine::UserRef)));

    // Hook up quit detection
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));
    
    // See if we have an authtoken/download content ID
    CheckArguments(argc, argv);

    // Sets middleware locale
    //Origin::Services::writeStringSetting(Origin::Services::SETTING_LOCALE, mLocale, Origin::Services::Session::SessionService::currentSession());
    GetTelemetryInterface()->SetLocale(mLocale.toLatin1().data());

    // announce the change of presence
    ORIGIN_LOG_EVENT << "Announce presence";
    // TBD: setting is not available since the user isn't logged in yet

    ORIGIN_LOG_EVENT << "Read settings file";
    ORIGIN_VERIFY_CONNECT(Origin::Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)), this, SLOT(onSettingChanged(const QString&, const Origin::Services::Variant&)));
    ORIGIN_VERIFY_CONNECT(Origin::Services::SettingsManager::instance(), SIGNAL(settingsSaved()), this, SLOT(onSettingsSaved()));

    // Once we read our settings do our forced update check. This will then show the UI once complete
    ORIGIN_LOG_EVENT << "Social slot setup";

    storeCommandLine();

    ORIGIN_VERIFY_CONNECT(this, SIGNAL(gameDownloadError(const QString&, const QString&)), this, SLOT(showDownloadingError(const QString&, const QString&)));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(updateDownloadError()), this, SLOT(showUpdateDownloadingError()));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(jitUrlRetrievalError (const QString &)), this, SLOT (showJITUrlRetrievalError (const QString &)));

    // Initialize our web widget support
    Origin::Client::WebWidgetController::create();
    Origin::Client::DialogController::create();

#ifdef ORIGIN_PC
    // Mac is not impacted by running under non-administrative accounts
    ORIGIN_LOG_EVENT << "Show limited user warning";
    mbLimitedUser = !Origin::Services::PlatformService::IsUserAdmin();
    if (mbLimitedUser)
        showLimitedUserWarning();
#endif

    bool isElevatedUser = EnvUtils::GetIsProcessElevated();

    // Telemetry Boot Settings.  We use a separate hook to make sure the real boot sess strt is as soon as possible.
    GetTelemetryInterface()->Metric_APP_SETTINGS( Origin::Services::PlatformService::isBeta(), isElevatedUser, mWasStartedFromAutoRun, mIsFreeToPlay );

    // this is a special qa/dev thing. If the ini file has set this up to crash on startup, crash
    if (mCrashOnStartup) 
    {
        ORIGIN_LOG_EVENT << "Crash on startup requested";
        QString* nullStrPtr = 0;
        nullStrPtr->clear();
    }

    // Set up our tray icon
#if !defined(ORIGIN_MAC)
    Origin::Client::EbisuSystemTray::instance()->show();
#endif
    
    QString sCommandLine(mCommandLine);

    //!!!! make sure this check for whether we're launched from browser or not occurs AFTER the call to Origin::Client::CommandFlow::instance()->setCommandLine above, 
    //otherwise CommandFlow::mLaunchedFromBrowser will get initialized to false
    //if an existence of origin is already running, bootstrapper automatically appends -Origin_LaunchedFromBrowser when passing along the command
    //but for the case when origin is actually getting launched, it doesn't work to append it to the commandline because we use Qt's arguments() to retrieve
    //the arguments from the commandLine and in the Qt code for arguments(), we seem to specifically ignore argv, argc that is artificially passed in via bootstrapper and use the
    //the one that's passed via the system.

    // If this instance of Origin was launched from a browser but no other instances were running, then flagging that it was 
    // posted from a browser must be done here.
    // Also note that we can't persist that Origin was launched from a browser because a new instance can be started from a browser,
    // but all further RTP commands could come from outside the browser.  (In that case Origin would be a child of a browser but still need to pass commands along.)
    if (EnvUtils::GetParentProcessIsBrowser())
    {
        Origin::Client::CommandFlow::instance()->appendToCommandLine ("-Origin_LaunchedFromBrowser");
    }

    // Code to force-trigger an exception for testing purposes.
    if (mbForceException)
    {
        ORIGIN_LOG_EVENT << "Crash with -ForceCrash";
        QString* nullStrPtr = 0;
        nullStrPtr->clear();
    }

#ifdef ORIGIN_MAC
    // Set up the Handlers for tracking the dock icon actions
    Class cls = reinterpret_cast<Class>(objc_getClass("NSApplication"));
    SEL sharedApplication = sel_registerName("sharedApplication");
    objc_object* appInst = objc_msgSend((id)cls,sharedApplication);
    
    if (appInst != NULL)
    {
        objc_object* delegate = objc_msgSend(appInst, sel_registerName("delegate"));
        Class delClass = (Class)objc_msgSend(delegate, sel_registerName("class"));
        //register a handler for the dock icon clicked event
        bool test = class_addMethod(delClass, sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:"), (IMP)dockClickHandler, "B@:");
        if (test)
        {
            qDebug("Registration of dockClickHandler was successfull");
        }
        // register handler for the 'Quit dock menu option, if one already exists we want to use our implemetaion not theirs.        
        test = class_replaceMethod(delClass, sel_registerName("applicationShouldTerminate:"), (IMP)dockQuitHandler, "B@");
        if (test)
        {
            qDebug("Registration of dockQuitHandler was successfull");
        }
    }
    
#endif
    // Log some telemetry about what config options are in place
    bool hasOverrides = Origin::Services::readSetting(Origin::Services::SETTING_OverridesEnabled);

    // Does user have an EACore.ini?
    GetTelemetryInterface()->Metric_APP_CONFIG(Origin::Services::SETTING_OverridesEnabled.name().toLocal8Bit().constData(),
        (hasOverrides ? "true" : "false"), hasOverrides);

    Origin::Engine::Content::CatalogDefinitionController::init();

    Origin::Services::OriginConfigService::instance()->reportDynamicConfigLoadTelemetry();

    if (showCompatibilityModeWarning())
    {
        ORIGIN_LOG_ERROR << "Origin is running in compatibility mode.";
    }

    ORIGIN_LOG_EVENT << "Done"; 
}

void OriginApplication::performPostConstructionInitialization()
{
    // create the MainFlow before triggering the hack to prevent a crash during RTP when the URL
    // event handler tries to invoke MainFlow::instance() before MainFlow has been created
    Origin::Client::MainFlow::create();
    
#ifdef ORIGIN_MAC

    // Cause GetURL events to be handled by our own handler.
    // We must call this *after* Qt installs their own handler, so ours will override it.
    Origin::Client::interceptGetUrlEvents();
    
    // Cause system shutdown events to be handled by our own handler.
    // We must call this *after* Qt installs their own app delegate.
    Origin::Client::interceptShutdownEvents();
    
#endif

    Origin::Client::MainFlow::instance()->start();

    bool proceedWithLogin = true;
    Origin::Client::CommandFlow *commandFlow = Origin::Client::CommandFlow::instance();
    if(commandFlow)
    {
        if (commandFlow->isActionPending())
        {
            QUrl actionUrl = commandFlow->getActionUrl();   //it could return an empty Url if parsing failed
            if (!actionUrl.isEmpty())
            {
                // stop the RTP game launch timer if we're not performing RTP
                if (!(actionUrl.toString().contains ("game/launch", Qt::CaseInsensitive)))
                {
                    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_CLEAR(EbisuTelemetryAPI::RTPGameLaunch_launching);
                }

                bool clearCommandParams = (commandFlow->isLaunchedFromBrowser() || commandFlow->isOpenAutomateParam());
                Origin::Client::PendingActionFlow::instance()->startAction(actionUrl, clearCommandParams);

                //if we're starting a sso flow, hold off on starting the login flow, that will get triggered
                //in onSSOFlowFinished in MainFlow.cpp
                proceedWithLogin = !(Origin::Client::MainFlow::instance()->ssoFlowActive());
            }
        }
    }
    if (proceedWithLogin)
    {
        ORIGIN_LOG_EVENT << "Proceed with login flow";
        Origin::Client::MainFlow::instance()->startLoginFlow();
    }

    if (mCustomerSupportLocales.empty())
    {
        mCustomerSupportLocales.insert("en_US", "en");
        mCustomerSupportLocales.insert("cs_CZ", "cz");
        mCustomerSupportLocales.insert("da_DK", "dk");
        mCustomerSupportLocales.insert("de_DE", "de");
        mCustomerSupportLocales.insert("el_GR", "en");
        mCustomerSupportLocales.insert("en_GB", "uk");
        mCustomerSupportLocales.insert("es_ES", "es");
        mCustomerSupportLocales.insert("es_MX", "mx");
        mCustomerSupportLocales.insert("fi_FI", "fi");
        mCustomerSupportLocales.insert("fr_FR", "fr");
        mCustomerSupportLocales.insert("hu_HU", "hu");
        mCustomerSupportLocales.insert("it_IT", "it");
        mCustomerSupportLocales.insert("ja_JP", "jp");
        mCustomerSupportLocales.insert("ko_KR", "kr");
        mCustomerSupportLocales.insert("nl_NL", "nl");
        mCustomerSupportLocales.insert("no_NO", "no");
        mCustomerSupportLocales.insert("pl_PL", "pl");
        mCustomerSupportLocales.insert("pt_BR", "br");
        mCustomerSupportLocales.insert("pt_PT", "pt");
        mCustomerSupportLocales.insert("ru_RU", "ru");
        mCustomerSupportLocales.insert("sv_SE", "se");
        mCustomerSupportLocales.insert("th_TH", "en");
        mCustomerSupportLocales.insert("zh_CN", "en");
        mCustomerSupportLocales.insert("zh_TW", "en");
    }
}

int OriginApplication::exec()
{
    // Setup the post-construction initialization to take place.
    QTimer::singleShot(0, this, SLOT(performPostConstructionInitialization()));

    bool ret = QApplication::exec();

#if defined(_DEBUG)
    Origin::Services::HttpServiceResponse::checkForMemoryLeak();
#endif

    return ret;
}

void OriginApplication::configCEURL(QString& settingValue, const QString& authCode)
{
    QStringList languageSetListForExternalBrowser;
    languageSetListForExternalBrowser << "zh_CN" << "zh_TW" << "th_TH" << "el_GR";
    QMap<QString,QString>::const_iterator it = mCustomerSupportLocales.find(locale());
    if(it != mCustomerSupportLocales.constEnd())
    {
        settingValue.replace("{loc2letter}", it.value());
        settingValue.replace("{code}", authCode);

        if (languageSetListForExternalBrowser.contains (locale()))  //one of the languages that gets mapped to "en", assume locale would be "en_US"
            settingValue.replace("{locale}", "en_US");
        else
            settingValue.replace("{locale}", it.key());
    }
}

bool OriginApplication::GetEbisuCustomerSupportInBrowser(QString & pCustomerSupport, const QString& authCode) const 
{   
    bool ShowInDefaultResult = true;
    QStringList languageSetListForExternalBrowser;
    languageSetListForExternalBrowser << "zh_CN" << "zh_TW" << "th_TH" << "el_GR";
    
    QMap<QString,QString>::const_iterator it = mCustomerSupportLocales.find(locale());
    if(it != mCustomerSupportLocales.constEnd())
    {
        // If we don't have an authCode to pass to the CE portal, send the user to the CE home page.
        if(authCode.isEmpty())
            pCustomerSupport = Origin::Services::readSetting(Origin::Services::SETTING_CustomerSupportHomeURL).toString();
        else
            pCustomerSupport = Origin::Services::readSetting(Origin::Services::SETTING_CustomerSupportURL).toString();

        pCustomerSupport.replace("{loc2letter}", it.value());
        pCustomerSupport.replace("{code}", authCode);

        if (languageSetListForExternalBrowser.contains (locale()))  //one of the languages that gets mapped to "en", assume locale would be "en_US"
            pCustomerSupport.replace("{locale}", "en_US");
        else
            pCustomerSupport.replace("{locale}", it.key());
    }
    else
    {
        pCustomerSupport = "";
    }
    
    // This code was disabled as part of OFM-499 due to high customer support traffic
/*
    //if we don't find it in a list of sites that use an external browser 
    if(Origin::Engine::LoginController::currentUser()->isUnderAge() == false
        && (languageSetListForExternalBrowser.indexOf(this->locale()) == -1))
    {
        ShowInDefaultResult = false;
    }
    else 
    {
        ShowInDefaultResult = true;
    }*/

    // Always show help in-IGO if it's active
    if (IGOController::instance()->isActive())
    {
        ShowInDefaultResult = false;
    }

    return ShowInDefaultResult; 
}

// returns false on failure
bool OriginApplication::SetAppLanguage(const QString &locale, bool updateQSS /* = true */)
{
    OriginTranslator* target = 0;
    bool loaded = false;

    if (!mTranslateStrings)
    {
        applyStylesheet();
        return true;
    }

    if (locale.isEmpty())
    {
        return false;
    }

#if defined(ORIGIN_MAC)
    if (locale != "en_US")
    {
        bool wasLoaded = mQtDefaultTranslator.load(QLocale(locale), "qt", "_", Origin::Services::PlatformService::translationsPath());
        ORIGIN_ASSERT(wasLoaded);
        if (wasLoaded)
            installTranslator(&mQtDefaultTranslator);
        else
            ORIGIN_LOG_EVENT << "SetAppLanguage: unable to load locale - " << locale;
    }
#endif

    // No work to do unless we have a new locale.
    if (locale.compare(mLocale, Qt::CaseInsensitive) == 0)
    {
        return true;
    }

    target = mTranslators.value(locale);

    // If we don't already have a translator for this locale we
    // have to try to load one.
    if (!target)
    {
        target = new OriginTranslator;
        loaded = target->loadHAL(":/lang/EbisuStrings_", locale);

        if (loaded)
        {
            mTranslators.insert(locale, target);
        }
        else
        {
            // We have no strings for the given locale.
            delete target;
        return false;
        }
    }

    installTranslator(target);
    mLocale      = locale;

    // If QA debug language is turned on, tweak our strings.
    if (mQADebugLang)
    {
        target->setQADebugLang(1.3f);
    }

    // Inform Qt of our locale choice
    QLocale::setDefault(QLocale(mLocale));

    if (Origin::Services::SettingsManager::instance() && updateQSS)
        Origin::Services::writeSetting(Origin::Services::SETTING_LOCALE, mLocale);

    if(updateQSS)
    {
        applyStylesheet();
    }

    // Tell the UI toolkit to update styles per language
    OriginCommonUI::changeCurrentLang(locale);

    return true;
}

void OriginApplication::readConfiguration()
{
    mQaForcedUpdateCheckInterval = Origin::Services::readSetting(Origin::Services::SETTING_qaForcedUpdateCheckInterval, Origin::Services::Session::SessionRef());
    mForceLockboxURL = Origin::Services::readSetting(Origin::Services::SETTING_ForceLockboxURL, Origin::Services::Session::SessionRef());
    mCrashOnStartup = Origin::Services::readSetting(Origin::Services::SETTING_CrashOnStartup, Origin::Services::Session::SessionRef());
    mEnvironmentName = Origin::Services::readSetting(Origin::Services::SETTING_EnvironmentName, Origin::Services::Session::SessionRef()).toString();

    if (Origin::Services::readSetting(Origin::Services::SETTING_HeartbeatLog, Origin::Services::Session::SessionRef()))
        Origin::Services::Heartbeat::instance()->logOpen();
    
    mQADebugLang = Origin::Services::readSetting(Origin::Services::SETTING_DebugLanguage, Origin::Services::Session::SessionRef());
    mBlockingBehaviorEnabled = Origin::Services::readSetting(Origin::Services::SETTING_BlockingBehavior, Origin::Services::Session::SessionRef());
    mSingleLoginEnabled = Origin::Services::readSetting(Origin::Services::SETTING_SingleLogin, Origin::Services::Session::SessionRef());
    
    Origin::Services::Heartbeat::instance()->setConfigServerName(Origin::Services::readSetting(Origin::Services::SETTING_HeartbeatURL, Origin::Services::Session::SessionRef()));
    
    const size_t kPathCapacity = 512;
    wchar_t appDirectory[kPathCapacity];

    // Setup mLogConfigFilePath
    EA::IO::GetSpecialDirectory(EA::IO::kSpecialDirectoryCurrentApplication, appDirectory, false, EAArrayCount(appDirectory));
    EA::StdC::Strlcat(appDirectory, "EALogConfig.ini", EAArrayCount(appDirectory));

    if(!EA::IO::File::Exists(appDirectory)) // If not found with the executable, try the working directory, which might be a different location.
    {
        EA::IO::Directory::GetCurrentWorkingDirectory(appDirectory);
        EA::StdC::Strlcat(appDirectory, "EALogConfig.ini", EAArrayCount(appDirectory));
    }

    mLogConfigFilePath.append(QString::fromWCharArray(appDirectory, wcslen(appDirectory)));
}

#if defined(EA_DEBUG)
void OriginApplication::ShowLocalizationKey(const bool showLocalizationKey)
{
    OriginTranslator* translator = mTranslators.value(mLocale);

    if (translator)
    {
        translator->debugShowKey(showLocalizationKey);

    // This is done just to force a language change event.
        removeTranslator(translator);
        installTranslator(translator);
}
}
#endif

bool OriginApplication::abortDueToGuestUser()
{
    if (!Origin::Services::PlatformService::isGuestUser())  // TODO add standard users, because they cannot launch installers !?
    {
        return false;
    }

    // TODO: UI TEAM. Implement Dialog here:

    // flag to prevent the rest of the UI to show up
    using namespace Origin::UIToolkit;
    OriginWindow* guestUserMsgBox  = new OriginWindow(
        (OriginWindow::TitlebarItems)(OriginWindow::Close),
        NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
    guestUserMsgBox->msgBox()->setup(OriginMsgBox::Notice,
        tr("ebisu_client_guest_user_warning_title"),
        tr("ebisu_client_guest_user_warning_message").arg(tr("application_name")));
    OriginPushButton* exitBtn = guestUserMsgBox->addButton(QDialogButtonBox::Yes);
    exitBtn->setText(tr("ebisu_client_exit"));
    OriginPushButton* continueBtn = guestUserMsgBox->addButton(QDialogButtonBox::No);
    continueBtn->setText(tr("ebisu_client_continue"));
    guestUserMsgBox->setDefaultButton(continueBtn);
    guestUserMsgBox->manager()->setupButtonFocus();
    guestUserMsgBox->setAttribute(Qt::WA_DeleteOnClose, false);
    ORIGIN_VERIFY_CONNECT(guestUserMsgBox, SIGNAL(rejected()), guestUserMsgBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(exitBtn, SIGNAL(clicked()), guestUserMsgBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(continueBtn, SIGNAL(clicked()), guestUserMsgBox, SLOT(close()));
    guestUserMsgBox->exec();
    bool exitClicked = (guestUserMsgBox->getClickedButton() == exitBtn);
    guestUserMsgBox->deleteLater();
    return exitClicked;
}

bool OriginApplication::showCompatibilityModeWarning()
{
#ifdef ORIGIN_PC
    if (Origin::Services::PlatformService::getCompatibilityMode().isEmpty())
    {
        return false;
    }

    Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::Error,
        tr("ebisu_client_os_compatibilitymode_error_title").arg(tr("application_name")),
        tr("ebisu_client_os_compatibilitymode_error_message").arg(tr("application_name")),
        tr("ebisu_client_continue"));

    return true;
#else
    return false;   // no compatibility mode on OSX
#endif
}

bool OriginApplication::showAppNotAllowedError()
{
    Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::Error,
        tr("ebisu_client_one_instance_warning_title").arg(tr("application_name")),
        tr("ebisu_client_one_instance_warning_message").arg(tr("application_name")).arg(tr("application_name")),
        tr("ebisu_client_close"));
    return true;
}

void OriginApplication::showLimitedUserWarning(bool fromITE_Flow)
{
    if (Origin::Services::PlatformService::isGuestUser())   // make sure if guest acct warning was already shown, don't show this warning
        return;
    
    if (!fromITE_Flow && isITE())   // if this is launched from Install Thru Ebisu, don't show this at startup since there will be another message for ITE
        return; 

    // flag to prevent the rest of the UI to show up
    using namespace Origin::UIToolkit;
    OriginWindow* limitedUserMsgBox = new OriginWindow(
        (OriginWindow::TitlebarItems)(OriginWindow::Close),
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);


    if (fromITE_Flow)   // if from the ITE flow, show a slightly different message and title
    {
        limitedUserMsgBox->msgBox()->setup(OriginMsgBox::Notice, tr("ebisu_client_dual_installation_error_title"), 
            tr("ebisu_client_ite_limited_user_warning_text"));
    }
    else
    {
        limitedUserMsgBox->msgBox()->setup(OriginMsgBox::Notice, 
            tr("ebisu_client_limited_user_warning_title"), 
            tr("ebisu_client_limited_user_warning_text").arg(tr("application_name")));
    }

    limitedUserMsgBox->setModal(true);
    ORIGIN_VERIFY_CONNECT(limitedUserMsgBox, SIGNAL(rejected()), limitedUserMsgBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(limitedUserMsgBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), limitedUserMsgBox, SLOT(close()));

    limitedUserMsgBox->manager()->setupButtonFocus();
    limitedUserMsgBox->exec();

    
}

OriginApplication::~OriginApplication()
{
    Origin::Services::PlatformService::setOriginState(Origin::Services::PlatformService::ShutDown);
    Origin::Sdk::LocalHost::stop();
    
    QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);

	ORIGIN_LOG_EVENT << "Destroy LSX Thread";
    emit stopLSXThread();

    Origin::Client::MainFlow::destroy();

    //  Shutdown the Heartbeat Monitoring System
    ORIGIN_LOG_EVENT << "Shutdown heartbeat";
    Origin::Services::Heartbeat::destroy();  

    // We do most of our cleanup in the quit() function, which is safer since it's not a destructor.
    shutdownEAText();

    ORIGIN_LOG_EVENT << "Destroy tray";
    Origin::Client::EbisuSystemTray::deleteInstance();
    
    OriginApplication& app = OriginApplication::instance();
    ORIGIN_LOG_EVENT << "Locale: " << app.locale();

    //since translate can be called from a different thread, deleting the translator could cause a crash (edge case that's been seen)
    //so since we're exiting anyways, and since neither ~OriginTranslator nor ~EATranslator do anything in the desturctor, just don't delete it
    //fyi - looked at calling removeTranslator before the delete but removeTranslator emits QEvent::LanguageChange event which may end up 
    //causing other knock-ons so nixed that idea.

    //ORIGIN_LOG_EVENT << "Destroy translators";
    //QMap<QString,OriginTranslator*>::iterator iter;

    //for (iter = mTranslators.begin(); iter != mTranslators.end(); iter++)
    //{
    //    delete iter.value();
    //}
    //mTranslators.clear();

    Origin::Client::CommandFlow::destroy();
    Origin::Client::PendingActionFlow::destroy();

    Origin::Engine::release();
    Origin::Services::release();
    
    ORIGIN_LOG_EVENT << "Done";
    Origin::Services::Logger::Instance()->Destroy();

#ifdef ORIGIN_PC
    if (mOriginDetectionWindow)
    {
        DestroyWindow(mOriginDetectionWindow);
        UnregisterClass(EBISU_WINDOW_CLASS, GetModuleHandle(NULL));
    }
    mOriginDetectionWindow = NULL;
#endif

}

void OriginApplication::dumpEACoreToLog()
{
    //  Write out the eacore.ini into log file
    QFile dumpFile(Origin::Services::PlatformService::eacoreIniFilename());
    if (dumpFile.open(QIODevice::ReadOnly))
    {
        QTextStream in(&dumpFile);
        in.setCodec(QTextCodec::codecForName("UTF-8"));
        in.setAutoDetectUnicode(true);
        QString line = in.readLine();
        while (!line.isNull())
        {
            if (line.length())
                ORIGIN_LOG_EVENT << "EACORE: " << line;
            line = in.readLine();
        }
    }
}

bool OriginApplication::EACoreIniExists()
{
    return QFile::exists(Origin::Services::PlatformService::eacoreIniFilename());
}

void OriginApplication::detectDeveloperMode()
{
    // Check for the file C:\ProgramData\Origin Developer Tool\odt.
    // Note that this path will change on XP/Mac.
    QString odtPath = Origin::Services::PlatformService::odtDirectory();
    odtPath.append(QDir::separator());
    odtPath.append("odt");

    if(QFile::exists(odtPath))
    {
        QFile odtFile(odtPath);
        if(odtFile.open(QIODevice::ReadOnly))
        {
            QByteArray encryptedData = odtFile.readAll();
            QByteArray machineHash = Origin::Services::PlatformService::machineHashAsString().toLatin1();
            QByteArray decryptedData;
            if(Origin::Services::CryptoService::decryptSymmetric(decryptedData, encryptedData, machineHash))
            {
                // Check that our decrypted data contains the machine hash.
                const bool machineHashCheck = decryptedData.endsWith(machineHash);

                // Check the decrypted value's validity (at least 1 character, all characters are alphanumeric).
                QRegExp alphanumeric("^[a-zA-Z0-9]+$");
                const bool validityCheck = alphanumeric.exactMatch(decryptedData);

                if(machineHashCheck && validityCheck)
                {
                    Origin::Services::writeSetting(Origin::Services::SETTING_OriginDeveloperToolEnabled, true);
                }
            }
        }
    }
}

void OriginApplication::startLSXThread()
{
    ORIGIN_LOG_EVENT << "Start";
    mLSXThread = new Origin::SDK::Lsx::LSXThread();
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(stopLSXThread()), mLSXThread, SLOT(quit()));
    mLSXThread->start();
    ORIGIN_LOG_EVENT << "Done";
}

void OriginApplication::initEAText()
{
#if defined(EATEXT_PRESENT)
        // Setup custom Thai dictionary entries. We have code in the Qt auxiliary library called "harfbuzz" which 
        // knows how to do line breaking on Thai sentences. But that code is based on a fixed dictionary of words
        // which is unchanging and typically ships with the operating system. However, new words are invented 
        // over time which we need to support. So we export a function called AddWordBreakDictionaryEntry which
        // allows us to dynamically add words to the Thai word break dictionary. The localization team identifies
        // these new words and adds them to the HAL string database in a string called THAI_LINE_BREAK_WORDS.
        // We read that string here and add the custom dictionary entries.

        EA::Text::InitWordBreakDictionary();

        // We have something like this in the .xml file:
        //  <locString Key="THAI_LINE_BREAK_WORDS">str1 str2 str3</locString>
        // We want to read each word from this and add it to the Thai dictionary.

        QResource resource(":/lang/EbisuStrings_th_TH.xml");

        if(resource.size())
        {
            eastl::string16 str((const char16_t*)resource.data(), (eastl_size_t)resource.size());
            eastl::string16 word;
            eastl_size_t    i = str.find(L"THAI_LINE_BREAK_WORDS");

            if(i < str.length())
            {
                i += 23;
                str.erase(0, i);

                i = str.find(L"</locString>");
                ORIGIN_ASSERT(i < str.length());
                str.erase(i);

                // Now we have a string of thai words separated by spaces.
                while((i = str.find(L" ")) < str.length())
                {
                    word.assign(str.data(), i);  // Copy up to but not including the space char.
                    str.erase(0, i + 1); // Erase up to and including the space char.

                    if(!word.empty())
                        EA::Text::AddWordBreakDictionaryEntry(word.c_str());
                }

                if(!str.empty()) // If the last word didn't have a space after it, this will execute as true.
                    EA::Text::AddWordBreakDictionaryEntry(str.c_str());
            }
        }
#endif // EATEXT_PRESENT
}

void OriginApplication::shutdownEAText()
{
#if defined(EATEXT_PRESENT)
        EA::Text::ShutdownWordBreakDictionary();
#endif
}

void OriginApplication::applyStylesheet()
{
    std::wstring openFrom(mIniFilePath.toStdWString());
    EA::IO::IniFile iniFile(openFrom.c_str());
    
    char8_t         styleSheetPath[512];
    int             count = iniFile.ReadEntryFormatted("Origin", "StyleSheet", "%511s", styleSheetPath);
    bool            bStyleSheetFound = false;
    QString         sStyleSheet;

    if(count)
    {
        bStyleSheetFound = GetStyleSheetFromFile(styleSheetPath, sStyleSheet);
    }

    if(!bStyleSheetFound) // If not applied via the text file...
    {
        bStyleSheetFound = GetStyleSheetFromFile(":/styles/origin.qss", sStyleSheet);
    }

    // Open language specific file, if it exists
    //mLocale = "ko_KR";
    QString strStyleLangFile = ":/styles/origin_";
    strStyleLangFile.append(mLocale);
    strStyleLangFile.append(".qss");
    QString  sStyleSheetLang;

    if (GetStyleSheetFromFile(strStyleLangFile, sStyleSheetLang))
    {
        sStyleSheet.append(sStyleSheetLang);
    }

    if (mLocale.compare("ko_KR", Qt::CaseInsensitive) == 0)
    {
        sStyleSheet.replace("bold;", "normal;", Qt::CaseInsensitive);
    }

    ORIGIN_ASSERT(bStyleSheetFound);
    if(bStyleSheetFound)
    {
        setStyleSheet(sStyleSheet);
    }
}

bool OriginApplication::GetStyleSheetFromFile(const QString& styleSheetPath, QString& styleSheet)
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

bool OriginApplication::isAppShuttingDown() const 
{ 
    return mHasQuit;
}

void OriginApplication::quit(ExitReason reason)
{
    if (!mHasQuit)
    {
        Origin::Services::PlatformService::setOriginState(Origin::Services::PlatformService::ShutDown);

        // We only need to do this for login because once we get past login
        // ITO parses and wipes the command line, so quitting after that will
        // not cause us to re-init ITO.
        if (reason == ReasonLoginExit)
        {
            Origin::Services::writeSetting(Origin::Services::SETTING_COMMANDLINE, "");
        }

        mHasQuit = true;
        ORIGIN_LOG_EVENT << "App Quit: " << reason;

        Origin::Services::PlatformJumplist::create_jumplist(true);   // set it to the base set of tasks

        // If the ini file has set this up to crash on exit, force a crash
        bool crashOnExit = Origin::Services::readSetting(Origin::Services::SETTING_CrashOnExit, Origin::Services::Session::SessionRef());
        if (crashOnExit) 
        {
            ORIGIN_LOG_EVENT << "Crash on exit requested";
            QString* nullStrPtr = 0;
            nullStrPtr->clear();
        }

        // KW: If IGO is currently visible, make sure the SDK gets the appropriate IGO closing event
        // and it propagates through middleware event before shutting down middleware.  
        // Fixes EBIBUGS-14095
        // Added the little 'instantiated' method because there's no need to create an instance of IGOController at this point. But understand it's
        // always safe to directly call the 'instance' method
        if(IGOController::instantiated() && IGOController::instance()->igowm()->visible())
        {
            const int igoCloseTimeout = 2000;
            ORIGIN_LOG_EVENT << "IGO is still visible, delaying shutdown until IGO closes, or forcing shutdown in " << igoCloseTimeout / 1000 << " seconds.";
            ORIGIN_VERIFY_CONNECT(IGOController::instance(), SIGNAL(stateChanged(bool)), this, SLOT(finishQuitOnIgoStateChange(bool)));
            IGOController::instance()->EbisuShowIGO(false);
            //TB: this is tricky, if the game process is shuting down/crashing or hanging while Origin quits, we may never receive
            //this signal, so I added a timeout!!!
            QTimer::singleShot(igoCloseTimeout, this, SLOT(forceQuitOnIgoStateChangeTimeout()));
        }
        else
        {
            QApplication::exit(0);
        }
            
    }
}

void OriginApplication::forceQuitOnIgoStateChangeTimeout()
{
    ORIGIN_LOG_EVENT << "IGO still active, forcing quit.";
    finishQuitOnIgoStateChange(false);
}

void OriginApplication::finishQuitOnIgoStateChange(bool igoState)
{
    ORIGIN_ASSERT(igoState == false);
    if(igoState == false)
    {
        ORIGIN_LOG_EVENT << "Quitting Origin now that IGO has closed or been forcibly closed.";
        QApplication::quit();
    }
}

void OriginApplication::restart(RestartMethod method)
{
    ORIGIN_LOG_EVENT << "App Restart";

    //unfortunately, we can't just take the commandline and prepend arguments to it
    //because in the event that origin:// or origin2:// is not percent encoded, we need to make sure
    //that the argument itself has quotes around it (quotes appear to be stripped when read in via arguments())
    QString sCommandLine;

    if (method == RestartWithCurrentCommandLine)
    {

#ifdef ORIGIN_PC
        //need to preserve that we were launched from browser
        if (EnvUtils::GetParentProcessIsBrowser())
            sCommandLine.append ("-Origin_LaunchedFromBrowser ");
#endif

        sCommandLine.append ("-Origin_Restart ");

        QStringList cmdArgs = Origin::Client::CommandFlow::instance()->commandLineArgs();

        for (int i = 0; i < cmdArgs.size(); i++)
        {
            QString arg = cmdArgs[i];
            if ((arg.contains ("origin://", Qt::CaseInsensitive)) || (arg.contains ("origin2://", Qt::CaseInsensitive)))
            {
#ifdef ORIGIN_PC
                if (arg.indexOf ("\"") != 0) //not surrounded
                {
                    arg.prepend ("\"");
                    arg.append ("\"");
                }
#elif defined(ORIGIN_MAC)   //for Mac check for single quote
                if (arg.indexOf ("'") != 0) //not surrounded
                {
                    arg.prepend ("'");
                    arg.append ("'");
                }
#endif
            }
            sCommandLine.append (arg);
            if (i < cmdArgs.size() - 1)
                sCommandLine.append (" ");
        }
    }


#ifdef ORIGIN_PC
    using namespace EA::StdC;

    wchar_t processPath[MAX_PATH];
    wchar_t processDir[MAX_PATH];
    wchar_t arguments[MAX_PATH];
    GetCurrentProcessPath(processPath);
    GetCurrentProcessDirectory(processDir);


    if (method == RestartWithCurrentCommandLine)
    {
        sCommandLine.toWCharArray (arguments);
        int len = sCommandLine.length();
        arguments [len] = L'\0';
    }
    else
    {
    swprintf_s(arguments, L"-Origin_Restart");
    }

    Origin::Engine::IGOController::instance()->unloadIGODll();
    quit(ReasonRestart);
    ExecNoWait(processPath, arguments, processDir);

#elif defined(ORIGIN_MAC)
    char tmpPath[PROC_PIDPATHINFO_MAXSIZE];
    char executablePath[PROC_PIDPATHINFO_MAXSIZE];
    proc_pidpath (getpid(), tmpPath, sizeof(tmpPath));
    
    // We need to point to the bootstrap, not the current exe
    size_t len = strlen(tmpPath);
    char* exeName = strrchr(tmpPath, '/');
    if (exeName)
    {
        if (exeName != tmpPath && exeName == (tmpPath + len - 1))
            exeName = strrchr(tmpPath - 1, '/');
        
        if (exeName)
        {
            exeName[0] = '\0';
            snprintf(executablePath, sizeof(executablePath), "%s/Origin", tmpPath);
        }
    }
    
    if (!exeName)
        strncpy(executablePath, tmpPath, sizeof(executablePath));
    
#ifdef ORIGIN_PC
    Origin::Engine::UnloadIGODll();
#endif
    
    // Construct the command to execute.  Wait for the old origin client process to die (by checking for
    // its pid), before re-launching.
    QString command;
    //
    // To properly execute Origin as an app, we'll execute
    // "/usr/bin/open -a <path-to-origin> --args <origin-args>
    //
    // This allows the system to prevent multiple instances of Origin from running.
    //
    if (method == RestartClean)
        //just mCommandLine as is
        command = QString("while [[ $? == 0 ]]; do sleep 1; ps -p %1; done ; exec /usr/bin/open -a '%2' --args -Origin_Restart").arg(getpid()).arg(executablePath).arg(mCommandLine);
    else
        //use sCommandLine that was processed with quotes around origin:// (or origin2://)
        command = QString("while [[ $? == 0 ]]; do sleep 1; ps -p %1; done ; exec /usr/bin/open -a '%2' --args %3").arg(getpid()).arg(executablePath).arg(sCommandLine);

    QByteArray command8Bit(command.toLocal8Bit());
    ORIGIN_LOG_EVENT << "Running command: " << command;
    char* args[4];
    args[0] = (char *)"/bin/sh";
    args[1] = (char *)"-c";
    args[2] = command8Bit.data();
    args[3] = 0;

    if (! fork())
    {
        // close all file descriptors beyond stdin, stdout, and stderrr.
        struct rlimit lim;
        getrlimit( RLIMIT_NOFILE, &lim );
        for ( rlim_t i = 3; i != lim.rlim_cur; ++i ) close( i );
        
        execv("/bin/sh", args);
    }
    
    quit(ReasonRestart);
#endif
}

void OriginApplication::ExecNoWait(wchar_t* executablePath, wchar_t* args, wchar_t* workingDir)
{
#ifdef ORIGIN_PC
    wchar_t commandLine[MAX_PATH*5];
    STARTUPINFOW startupInfo;
    memset(&startupInfo,0,sizeof(startupInfo));
    PROCESS_INFORMATION procInfo;
    memset(&procInfo,0,sizeof(procInfo));

    int size = swprintf_s(commandLine, L"%s %s", executablePath, args ? args : L"");
    if( size > 0 )
    {
        startupInfo.cb = sizeof(startupInfo);

        BOOL result = ::CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 0, NULL, workingDir, &startupInfo, &procInfo);
        if( result == FALSE )
        {
            if (procInfo.hProcess)
            {
                ::CloseHandle(procInfo.hProcess);
            }

            if (procInfo.hThread)
            {
                ::CloseHandle(procInfo.hThread);
            }
        }
    }
#endif
}

void OriginApplication::cancelTaskAndExit(ExitReason reason)
{
    quit(reason);
}

void OriginApplication::onAboutToQuit()
{
    quit(ReasonUnknown);
}

bool OriginApplication::nativeEventFilter(const QByteArray & eventType, void* message, long* result)
{
#ifdef ORIGIN_PC
    //On Windows, eventType is set to "windows_generic_MSG" "windows_dispatcher_MSG" 
    Q_UNUSED(eventType);
    return winEventFilter((MSG*)message, result);
#else
    Q_UNUSED(message);
    Q_UNUSED(eventType);
    Q_UNUSED(result);
    
    return false;
#endif
}

#if defined(ORIGIN_PC)
bool OriginApplication::disableUIPI(HWND hwnd, unsigned int msg)
{
    // win7+ only
    typedef struct tagCHANGEFILTERSTRUCT {
        DWORD cbSize;
        DWORD ExtStatus;
    } CHANGEFILTERSTRUCT, *PCHANGEFILTERSTRUCT;
        
    typedef BOOL (WINAPI *tChangeWindowMessageFilterEx)(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
    tChangeWindowMessageFilterEx pChangeWindowMessageFilterEx = (tChangeWindowMessageFilterEx)GetProcAddress(GetModuleHandle(L"user32.dll"), "ChangeWindowMessageFilterEx");
    BOOL result = FALSE;

    if (pChangeWindowMessageFilterEx)
    {
        result = pChangeWindowMessageFilterEx(hwnd, msg, 1 /*MSGFLT_ALLOW*/, NULL); // window wide
    }
    else
    {
        // win vista only
        typedef BOOL (WINAPI *tChangeWindowMessageFilter)(UINT message, DWORD dwFlag);
        tChangeWindowMessageFilter pChangeWindowMessageFilter = (tChangeWindowMessageFilter)GetProcAddress(GetModuleHandle(L"user32.dll"), "ChangeWindowMessageFilter");
        if (pChangeWindowMessageFilter)
        {
            result = pChangeWindowMessageFilter(msg, 1 /*MSGFLT_ADD*/); // process wide
        }
    }

    return result;
}

void OriginApplication::ProcessGameTelemetryMsg(PCOPYDATASTRUCT msg)
{
    // Be safe: check version and sizes
    if (msg->cbData != sizeof(OriginIGO::TelemetryMsg))
    {
        ORIGIN_LOG_ERROR << "Invalid IGO Telemetry msg size (" << msg->cbData << "/" << sizeof(OriginIGO::TelemetryMsg) << ")";
        return;
    }

    OriginIGO::TelemetryMsg* tMsg = reinterpret_cast<OriginIGO::TelemetryMsg*>(msg->lpData);
    if (tMsg)
    {
        if (tMsg->version != OriginIGO::TelemetryVersion)
        {
            ORIGIN_LOG_ERROR << "Invalid IGO Telemetry msg version (" << tMsg->version << "/" << OriginIGO::TelemetryVersion << ")";
            return;
        }

        QString productId = QString::fromUtf16(reinterpret_cast<const ushort*>(tMsg->productId));
        switch (tMsg->fcn)
        {
            case OriginIGO::TelemetryFcn_IGO_HOOKING_BEGIN:
                {
                    Origin::Services::Session::SessionRef session = Origin::Services::Session::SessionService::currentSession();
                    bool showHardwareSpecs = !Origin::Services::readSetting(Origin::Services::SETTING_HW_SURVEY_OPTOUT, session);
                    GetTelemetryInterface()->Metric_IGO_HOOKING_BEGIN(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid, showHardwareSpecs);
                }
                break;

            case OriginIGO::TelemetryFcn_IGO_HOOKING_FAIL:
                GetTelemetryInterface()->Metric_IGO_HOOKING_FAIL(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid, tMsg->context, tMsg->info.msg.renderer, tMsg->info.msg.message);
                break;

            case OriginIGO::TelemetryFcn_IGO_HOOKING_INFO:
                GetTelemetryInterface()->Metric_IGO_HOOKING_INFO(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid, tMsg->context, tMsg->info.msg.renderer, tMsg->info.msg.message);
                // keep old telemetry event for now as well
                if (tMsg->context == OriginIGO::TelemetryContext_THIRD_PARTY)
                GetTelemetryInterface()->Metric_IGO_OVERLAY_3RDPARTY_DLL(false, true, tMsg->info.msg.message);
                break;

            case OriginIGO::TelemetryFcn_IGO_HOOKING_END:
                GetTelemetryInterface()->Metric_IGO_HOOKING_END(productId.toLatin1().constData(), tMsg->timestamp, tMsg->pid);
                break;

            default:
                ORIGIN_LOG_ERROR << "Invalid IGO Telemetry msg fcn = " << tMsg->fcn;
                break;
        }
    }
}

bool OriginApplication::winEventFilter(MSG* message, long* result)
{
    // USING_XP
    static HDEVNOTIFY hDeviceNotify = NULL;

    if(g_bStartNotAllowed)
        return false;

    // disable User Interface Privilege Isolation (UIPI) for messages from AppRunDetector to enable mime type support if Origin runs elevated!
    if (message->message == WM_CREATE)
    {
        disableUIPI(message->hwnd, WM_COPYDATA);
        disableUIPI(message->hwnd, WM_SYSCOMMAND);

        if ((Origin::Services::PlatformService::OSMajorVersion() < 6)/* Windows XP */)
        {
            if( !hDeviceNotify )
            {
#if ENABLE_VOICE_CHAT
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( isVoiceEnabled )
                {
                    bool ok = Origin::Services::Voice::RegisterDeviceNotification(message->hwnd, &hDeviceNotify);
                    ORIGIN_ASSERT(ok);
                }
#endif
            }
        }
    }
    else
    if (message->message == WM_CLOSE)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            if ((Origin::Services::PlatformService::OSMajorVersion() < 6)/* Windows XP */)
            {
                if( hDeviceNotify )
                    Origin::Services::Voice::UnregisterDeviceNotification(hDeviceNotify);
            }
        }
#endif
    }
    else
    if ( message->message == WM_INPUTLANGCHANGE )
    {
        if ( !IGOController::instance()->igowm()->visible() )
            IGOController::instance()->setOriginKeyboardLanguage((void*)GetKeyboardLayout(0));
    }
    else
    if ( message->message == WM_ENDSESSION )
    {
        // If we don't quit out at this point for XP, the client actually blocks shutdown from occuring, so we no longer wait for an orderly exit here
        // This actually matches Vista behavior which forces the application to terminate at this point without giving it more time to shutdown. (EBIBUGS-24515)
        if (message->wParam == TRUE && mReceivedQueryEndSession && Origin::Services::PlatformService::OSMajorVersion() < 6)
        {
            ORIGIN_LOG_EVENT << "Forcing client exit on OS shutdown.";
            QApplication::exit(0);
        }
    }
    else
    if ( message->message == WM_QUERYENDSESSION )
    {   
        // If we are earlier than Vista, we have to respond true here for OS shutdown to process smoothly (EBIBUGS-24515)
        if (Origin::Services::PlatformService::OSMajorVersion() < 6)
        {
            *result = TRUE;
        }

        // Ignore any WM_QUERYENDSESSION messages not coming from the main thread
        if( QCoreApplication::instance()->thread() != QThread::currentThread() )
        {
            return true;
        }

        // EBIBUGS-18467
        // Ignore all WM_QUERYENDSESSION message other than the first one.
        // This may fix a race condition where the content controller has already been destroyed when
        // the 'gotCloseEvent' signal gets processed.
        if( mReceivedQueryEndSession )
        {
            return true;
        }
        mReceivedQueryEndSession = true;

        // EBIBUGS-15230
        // Ignore this message if we're already in the process of quitting.
        if(mHasQuit)
        {
            ORIGIN_LOG_EVENT << "Received WM_QUERYENDSESSION message, ignored because app is already shutting down.";
            return true;
        }
        
        ORIGIN_LOG_EVENT << "Received WM_QUERYENDSESSION message, initiating shutdown by emitting gotCloseEvent()";
        GetTelemetryInterface()->Metric_APP_POWER_MODE(EbisuTelemetryAPI::PowerShutdown);
        
        emit (gotCloseEvent()); // do proper flow shutdown sequence
        return true; // if we do not return true, Windows may decide to cancel the shutdown sequence on our behalf.  We don't want that.
    }
    else if(message->message == WM_WINDOWPOSCHANGING)
    {
        Origin::Client::PendingActionFlow* pendingActionFlow = Origin::Client::PendingActionFlow::instance();
        // EBIBUGS-26802: For some reason passing command line args activates the application and Windows
        // sends us an WM_WINDOWPOSCHANGING message. We don't want that.
        if(pendingActionFlow && pendingActionFlow->isActionRunning() && pendingActionFlow->shouldShowPrimaryWindow() == false)
        {
            WINDOWPOS* pos = reinterpret_cast<WINDOWPOS*>(message->lParam);
            pos->flags |= (SWP_NOACTIVATE | SWP_NOZORDER); 
        }
    }
    else if(message->message == WM_COPYDATA)
    {
        if(Origin::Client::MainFlow::instance() != NULL)
        {
            using namespace Origin::Client;

            // Do a sub-test for the specific identifier for the message
            PCOPYDATASTRUCT copyData = reinterpret_cast<PCOPYDATASTRUCT>(message->lParam);
            if (copyData)
            {
                if(copyData->dwData == kWin32ActivateIdentifier)
                {
                    if (copyData->cbData > 0)
                    {
                        QString argument;
                        QString cmdLine = QString::fromUtf16(static_cast<const ushort *>(copyData->lpData), copyData->cbData/sizeof(ushort));

                        if (Origin::Client::CommandFlow::instance())
                        {
                            bool ret = handleMessage(cmdLine, false);

                            // Signal to the requesting app, whether the command was accepted.
                            PostMessage(reinterpret_cast<HWND>(message->wParam), WM_COMMANDLINE_ACCEPTED, ret ? 1 : 0, NULL);
                        }
                    }
                }
                else if(copyData->dwData == kGameTelemetry)
                {
                    // Got a telemetry message from OIG/game
                    ProcessGameTelemetryMsg(copyData);
                }
            }
        }
        else
        {
            PCOPYDATASTRUCT copyData = reinterpret_cast<PCOPYDATASTRUCT>(message->lParam);
            if (copyData)
            {
                if(copyData->dwData == kWin32ActivateIdentifier)
                {
                    // Signal to the requesting app that we could not process the command.
                    PostMessage(reinterpret_cast<HWND>(message->wParam), WM_COMMANDLINE_ACCEPTED, 0, NULL);
                }
            }
        }
    }
    // hack the code to make sure we have a doubleclick close
    else if (message->message == WM_SYSCOMMAND)
    {
        if ( (message->wParam == SC_CLOSE2))
        {
            //if(EbisuDlg::instance()->winId() == message->hwnd)
            //{
            //  message->wParam = SC_CLOSE;
            //}
        }
    }
    else if (message->message == WM_NCHITTEST)
    {
        if (GetWindowLong(message->hwnd, GWL_STYLE) & WS_CHILD)
        {
            // Tell Windows to ask the HWND above us
            if (result)
            *result = HTTRANSPARENT;
            return true;
        }
    }
    else if (message->message == WM_POWERBROADCAST)
    {
        // Ignore any messages not coming from the main thread
        if( QCoreApplication::instance()->thread() != QThread::currentThread() )
            return true;

        // On computer sleep and wake, Windows is sending the events multiple times.
        // This flag is used to ensure that the corresponding code is executed only once.
        static volatile bool isInSleepMode = false;

        if (!isInSleepMode && message->wParam == PBT_APMSUSPEND)
        {
            isInSleepMode = true;
            Origin::Services::Network::GlobalConnectionStateMonitor::setComputerAwake(false);

            GetTelemetryInterface()->Metric_APP_POWER_MODE(EbisuTelemetryAPI::PowerSleep);
            ORIGIN_LOG_EVENT << "Received entering sleep mode event from OS.";
        }
        else if (isInSleepMode && message->wParam == PBT_APMRESUMEAUTOMATIC)
        {
            GetTelemetryInterface()->Metric_APP_POWER_MODE(EbisuTelemetryAPI::PowerWake);
            ORIGIN_LOG_EVENT << "Computer waking from sleep mode.";
            isInSleepMode = false;
            Origin::Services::Network::GlobalConnectionStateMonitor::setComputerAwake(true);
        }
        if (result)
        *result = TRUE;
        return true;
    }
    else if (message->message == WM_MOUSEMOVE || message->message == WM_KEYDOWN)
    {
        // Start active timer
        GetTelemetryInterface()->Metric_ACTIVE_TIMER_START();

        // restart the running timer if it was stopped
        // This could happen if a user logs out without closing the client
        GetTelemetryInterface()->RUNNING_TIMER_START(0);

    }
    else if(message->message == WM_ACTIVATEAPP)
    {
        if(message->wParam == false)
        {
            //Check for playing game here before we continue
            Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
            if (c && c->isPlaying())
            {
                return false;
            }
            // if we are here Origin has lost focus and we are not playing a game
            GetTelemetryInterface()->Metric_ACTIVE_TIMER_END(false);
        }
    }
    else if (WM_NOTIFY == message->message)
    {
        ENLINK *enlink = reinterpret_cast<ENLINK *>(message->lParam);
        if (EN_LINK == enlink->nmhdr.code && WM_LBUTTONUP == enlink->msg)
        {
            WCHAR linkText[4096];
            TEXTRANGEW textRange = {0};

            LONG toCopy = enlink->chrg.cpMax - enlink->chrg.cpMin;
            if (toCopy >= sizeof(linkText) / sizeof(linkText[0]))
            {
                toCopy = sizeof(linkText) / sizeof(linkText[0]) - 1;
            }

            textRange.chrg.cpMin = enlink->chrg.cpMin;
            textRange.chrg.cpMax = textRange.chrg.cpMin + toCopy;
            textRange.lpstrText = linkText;

            if (toCopy == SendMessageW(enlink->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange)))
            {
                const QUrl &url = QUrl::fromUserInput(QString::fromWCharArray(linkText, toCopy));
                Origin::Services::PlatformService::asyncOpenUrl(url);
            }
        }
    }
    else if(message->message == WM_DEVICECHANGE)
    {
#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            if ((Origin::Services::PlatformService::OSMajorVersion() < 6)/* Windows XP */)
            {
                switch (message->wParam)
                {
                case DBT_DEVICEARRIVAL:
                case DBT_DEVICEREMOVECOMPLETE:
                    {
                        PDEV_BROADCAST_HDR pHdr = (PDEV_BROADCAST_HDR)message->lParam;
                        switch( pHdr->dbch_devicetype )
                        {
                        case DBT_DEVTYP_DEVICEINTERFACE:
                            PDEV_BROADCAST_DEVICEINTERFACE pDevInf = (PDEV_BROADCAST_DEVICEINTERFACE)pHdr;
                            QString deviceName;
                            if( Origin::Services::Voice::GetDeviceName(deviceName, pDevInf, message->wParam) )
                            {
                                emit (deviceChanged());
                            }
                            break;
                        };
                    }
                    break;
                };
            }
        }
#endif
    }
    else if (message->message == 0x0118)
    {
        // EBIBUGS-28910
        // 0x0118 is an undocumented message that causes our taskbar icon to
        // unncessarily flash. By the time we process the message here, the
        // taskbar is already flashing. We ignore the message here which 
        // causes it to stop (so there is a small period of time where the
        // flash still occurs).
        return true;
    }

    return false;
}
#endif // ORIGIN_PC


#ifdef ORIGIN_MAC

void OriginApplication::openUrlFromBrowser(QUrl url)
{
    ORIGIN_LOG_EVENT << "Received URL from web browser.";
    
    // Construct a fake command line for the main flow to handle.
    QString fakeCommandLine = url.toEncoded();

    // Add a command line argument to ensure the security code handles it appropriately.
    //need to add " " around it so it gets treated as a separate command line argument
    fakeCommandLine.append( " \"-Origin_LaunchedFromBrowser\"" );
    
    // Pass the fake command line to the Command Flow
    handleMessage (fakeCommandLine, true);
}

bool OriginApplication::event(QEvent* e)
{
    static int windowHoverCheckTimer = -1;
    static QPointer<QWidget> prevHoverWidget = NULL;
    
    switch (e->type())
    {
    case QEvent::FileOpen:
        {
            QFileOpenEvent* foe = dynamic_cast<QFileOpenEvent*>(e);
            ORIGIN_ASSERT(foe);
            QUrl url(foe->url());
            if (url.scheme().toLower() == "origin" || url.scheme().toLower() == "origin2")
            {
                ORIGIN_LOG_EVENT << "Received open with URL: " << url.toString();
                handleMessage (url.toString(), true);
                    }
            }
        break;

    // I KNOW this sucks (for ApplicationActivate, ApplicationDeactivate, and Timer).
    // There isn't anything else we can do. To get the hover state on the titlebar items
    // while the applcation is deactivated. Qt Doesn't support it.
    // We can't put this inside of the ui toolkit because we don't want references
    // to the application. Also we would have to do it per instance - which would make
    // it even slower. Also we don't get ApplicationDeactivate and ApplicationActivate
    // from there. Sorry.
    case QEvent::ApplicationActivate:
        // Kill the timer - Qt will look for the hover
        killTimer(windowHoverCheckTimer);
        prevHoverWidget = NULL;
        break;

    case QEvent::ApplicationDeactivate:
        {
        // On Mac, all events but the timer events are killed when the application
        // becomes the inactive. This is bad for us because we our window have 
        // non-native chrome. So we have to check to see if the user is hovering
        // over our titlebar items.
        windowHoverCheckTimer = startTimer(500);
        // Clear the prev widget in case it was deleted since the last time
        prevHoverWidget = NULL;
        //Check for playing game here before we continue
        Origin::Engine::Content::ContentController * c = Origin::Engine::Content::ContentController::currentUserContentController();
        if (c)
        {
            if (!c->isPlaying())
            {
                // if we are here Origin has lost focus and we are not playing a game
                GetTelemetryInterface()->Metric_ACTIVE_TIMER_END(false);
            }
        }
        }
        break;

    case QEvent::Timer:
        {
            if(((QTimerEvent*)e)->timerId() == windowHoverCheckTimer)
            {
                // What widget is at our mouse position?
                QWidget* hoverWidget = widgetAt(QCursor::pos());
                
                // If we have changed what we are hovering over or we aren't hovering
                // over anything - show a leave state.
                if(prevHoverWidget && prevHoverWidget != hoverWidget)
                {
                    QEvent event = QEvent(QEvent::Leave);
                    QApplication::sendEvent(prevHoverWidget, &event);
                    prevHoverWidget = NULL;
                }
                
                if(hoverWidget)
                {
                    // If we are hovering over our titlebar buttons - we want to
                    // send the hover event to their button container parent.
                    if(Origin::UIToolkit::OriginTitlebarOSX::isHoveringInButtonContainer(hoverWidget))
                        hoverWidget = hoverWidget->parentWidget();
                    
                    QEvent event = QEvent(QEvent::Enter);
                    QApplication::sendEvent(hoverWidget, &event);
                    // Track this widget so we can send it hovering off event
                    prevHoverWidget = hoverWidget;
                }
            }
        }

    default:
        break;
    }

    return QApplication::event(e);
}

#endif

bool OriginApplication::eventFilter(QObject* obj, QEvent* event)
{
    if(g_bStartNotAllowed)
        return false;

    switch (event->type())
    {
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
        if (obj->property("origin-allow-drag").toBool() != true)
        {
            return true;
        }
        break;

    default:
        break;
    }

    //Let the dialog handle the event
    return false;
}

void OriginApplication::storeCommandLine()
{
#ifdef ORIGIN_PC
    QString Commandline = QString::fromUtf16(GetCommandLine());
#else
    QStringList args = arguments();
    QString Commandline;
    for (int i=0, count = args.size(); i != count; ++i)
    {
        Commandline += args[i];
        Commandline += " ";
    }
    Commandline = Commandline.trimmed(); // trim trailing space
#endif
    
    if(Commandline.contains("/contentIds:"))
    {
//      Origin::Services::writeSetting(Origin::Services::SETTING_COMMANDLINE, Commandline, Origin::Services::Session::SessionService::currentSession()); 
        Origin::Services::writeSetting(Origin::Services::SETTING_COMMANDLINE, Commandline); 
    }
}

bool OriginApplication::isITE()
{

//  QString commandLine = Origin::Services::readSetting(Origin::Services::SETTING_COMMANDLINE, Origin::Services::Session::SessionService::currentSession(), QVariant::String);
    QString commandLine = Origin::Services::readSetting(Origin::Services::SETTING_COMMANDLINE, Origin::Services::Session::SessionRef());

    //we used to just rely on the command line but since that gets wiped to fix a bug where ITE will restart after crash, the command line gets wiped
    //we now add a mITEActive check which get set in the Init and Cleanup
    return commandLine.contains("/contentIds:") || mITEActive;
}

void OriginApplication::onUserLoggedIn(Origin::Engine::UserRef user)
{
    loginSettings(user);

    // A dump of the user's basic settings to the client log (when the user 
    // logs in) is helpful for troubleshooting live issues.
    printUserSettings();
}

void OriginApplication::printUserSettings()
{
    QString dipLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOADINPLACEDIR);
    QString cacheLocation = Origin::Services::readSetting(Origin::Services::SETTING_DOWNLOAD_CACHEDIR);

    ORIGIN_LOG_EVENT << Origin::Services::SETTING_DOWNLOADINPLACEDIR.name() <<
        ": " << dipLocation;

    ORIGIN_LOG_EVENT << Origin::Services::SETTING_DOWNLOAD_CACHEDIR.name() <<
        ": " << cacheLocation;
}

void OriginApplication::loginSettings(Origin::Engine::UserRef user)
{
    QString accessToken = Origin::Services::Session::SessionService::accessToken(user->getSession());
    if (accessToken.size() > 0 // if authToken has content we have been authenticated online 
        && user->email().size() > 0) // if the email is not empty (edge case)
    {
        SaveOriginIdToEmailUserInfo(user);
    }

    // Set app locale to stored locale. This is the locale chosen by the user in Settings
    QString locale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE);
    user->setLocale(locale);
    // We need to apply the correct style to fit the locale we are changing to
    SetAppLanguage(locale, true);

    // Time to instantiate IGOController before any signals are hooked up/setup the core desktop
    Origin::Engine::IGOController::instance()->setupDesktop(&mExceptionSystem);
}

// setting-related notifications
void OriginApplication::onSettingChanged(const QString& settingKey, Origin::Services::Variant const& value)
{
    if (settingKey == Origin::Services::SETTING_LOCALE.name())
    {
        QString locale = value;
        GetTelemetryInterface()->SetLocale(locale.toStdString().c_str());
    }
    else if (settingKey == Origin::Services::SETTING_DOWNLOADINPLACEDIR.name() || 
        settingKey == Origin::Services::SETTING_DOWNLOAD_CACHEDIR.name() )
    {
        // Output DiP or download cache directories when they change
        ORIGIN_LOG_EVENT << settingKey << ": " << QString(value);
    }
    else if (settingKey == Origin::Services::SETTING_RUNONSYSTEMSTART.name())
    {
        Origin::Services::PlatformService::setAutoStart(value);
    }
}

void OriginApplication::onSettingsSaved()
{
    // TBD
}

#ifdef ORIGIN_MAC
void OriginApplication::onClickDock()
{
    // First we need to check to see if we have any unread messages
    Origin::Client::ClientFlow* clientFlow = Origin::Client::ClientFlow::instance();
    if(clientFlow)
    {
        Origin::Client::SocialViewController* controller = clientFlow->socialViewController();
        
        if (controller)
        {
            QSharedPointer<Origin::Engine::Social::Conversation> conversation = controller->findLastNotifiedConversation();
            // Do we have a last notified conversation
            if(conversation)
            {
                conversation->setCreationReason(Origin::Engine::Social::Conversation::InternalRequest);
                // Show chat window with conversation and we are done
                controller->chatWindowManager()->startConversation(conversation);
                // Set this to "-1" for now so we don't show this again until we actually get an
                // unread notification
                controller->setLastNotifiedConversationId("-1");
                return;
            }
        }
    }
    
    // Grab the currently focused widget for later use
    QWidget* focusedWidget = this->focusWidget();
    Origin::UIToolkit::OriginWindow* focusedWindow = NULL;
    QString fwName = "None Found";
    // If we have a focused widget get the window title for comparison
    if (focusedWidget)
        fwName = focusedWidget->window()->windowTitle();

    // Grab all of the already open not minimized windows and re-open them
    QWidgetList widgets = Origin::Client::GetOrderedTopLevelWidgets();
    foreach (QWidget *widget, widgets)
    {
        Origin::UIToolkit::OriginWindow* window = dynamic_cast<Origin::UIToolkit::OriginWindow*>(widget);
        if (window && window->isVisible())
        {
            // Get window name to determine if it was the focused widget
            QString windowName = window->windowTitle();
            
            if (windowName == fwName)
            {
                // Want to focused window to always draw on top
                // So save the window so we can re-activate it later
                focusedWindow = window;
                continue;
            }
            // Re-activate all found windows
            if (window->isMinimized())
                window->showNormal();
            else
                window->raise();
        }
    }
    // Re-activate the focused window here so it will always be drawn
    // on top of all other widgets
    if (focusedWindow)
    {
        if (focusedWindow->isMinimized())
            focusedWindow->showNormal();
        else
            focusedWindow->raise();
        // Explicitly set this to the active window to ensure that it always has focus
        // Not doing this will grant focus to any window that was previously minimized
        focusedWindow->activateWindow();
        focusedWindow = 0;
        return;
    }
    
    // This fixes https://developer.origin.com/support/browse/EBIBUGS-21146
    // Grab all of the already open windows including minimized and look for the oldest
    // The requested behavior is that only the oldest window appears when we ahve mulitple windows minized
    QWidgetList minWidgets = QApplication::topLevelWidgets();
    QTime oldestTimeStamp = QTime();
    Origin::UIToolkit::OriginWindow* oldestWindow = NULL;
    foreach (QWidget* widget, minWidgets)
    {
        if(Origin::Services::PlatformService::isOIGWindow(widget))
    {
            continue;
    }
        Origin::UIToolkit::OriginWindow* window = dynamic_cast<Origin::UIToolkit::OriginWindow*>(widget);
        if (window && window->isVisible())
        {
            QTime windowTimeStamp = window->creationTime();
    
            if (oldestTimeStamp.isNull() || !oldestWindow)
            {
                oldestTimeStamp = windowTimeStamp;
                oldestWindow = window;
            }
            else
            {
                if (oldestTimeStamp.operator>(windowTimeStamp))
    {
                    oldestTimeStamp = windowTimeStamp;
                    oldestWindow = window;
                }
            }
        
        }
    }
    // Re-open our oldest window if we found one
    if(oldestWindow)
    {
        QString name = oldestWindow->windowTitle();
        // EBIBUGS-25817 Change this to a force raise instead of showNormal. an OriginWindow's showNormal changes the
        // saves previous geometry and does other windowy things. You shouldn't call showNormal willy-nilly. Instead of
        // doing something high risk - we will just do a force raise. - which is what we wanted anyway.
        oldestWindow->show();
        oldestWindow->raise();
        oldestWindow->activateWindow();
        return;
    }

    // If we are here we can assume we don't have any open windows or minimized windows
    // Reactivate the main Client window
    Origin::Client::MainFlow::instance()->reactivateClient();
}

void OriginApplication::onDockQuit()
{
    if(Origin::Client::MainFlow::instance() && Origin::Engine::LoginController::isUserLoggedIn())
    {
        ORIGIN_LOG_EVENT << "User requested application exit via system tray, kicking off exit flow.";
        // If we are logged in, kick off the exit flow.  Skip the confirmation step.
        Origin::Client::MainFlow::instance()->exit(Origin::Client::ClientLogoutExitReason_Exit_NoConfirmation);
    }
    else
    {
        ORIGIN_LOG_EVENT << "User requested application exit via system tray, but no user has logged in yet so simply exiting.";
        // If not, we don't have a valid Session so we can't send telemetry or clean up.  Just exit in this case.
        cancelTaskAndExit(ReasonSystrayExit);
    }
}

#endif

void OriginApplication::SaveOriginIdToEmailUserInfo(UserRef user) 
{
    if(!user->isUnderAge())
    {
        const QString userNameToStore = user->eaid().toLower();
        const QString hashedUserName(QCryptographicHash::hash(userNameToStore.toLatin1(), QCryptographicHash::Md5).toHex());
        const QString userEmailToStore = user->email().toLower();

        // new - create a temorary setting and store it on a per user base!
        QScopedPointer<Origin::Services::Setting> tmpSetting;
        tmpSetting.reset(new Origin::Services::Setting(hashedUserName, Origin::Services::Variant::String, userEmailToStore, Origin::Services::Setting::LocalPerUser, Origin::Services::Setting::ReadWrite, Origin::Services::Setting::Encrypted));
        Origin::Services::writeSetting(*tmpSetting, userEmailToStore, Origin::Services::Session::SessionService::currentSession());
        // old
        //EbisuSettings::SetEncrypted(hashedUserName, userEmailToStore);

        // TODO: create a wrapper for a temporary setting!?
    }
}

void OriginApplication::showDownloadingError(const QString& mErrorTitleToken, const QString& mErrorCaptionToken)
{
    Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::Notice, mErrorTitleToken, mErrorCaptionToken, tr("ebisu_client_close"));
}

void OriginApplication::showUpdateDownloadingError()
{
    Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::Notice, tr("ebisu_client_download_update_error_title"), tr("ebisu_client_patch_check_failed"), tr("ebisu_client_close"));
}

void OriginApplication::showJITUrlRetrievalError (const QString &sErrorCode)
{
    QString errorText = QString(tr("ebisu_client_download_jiturl_error")).arg(sErrorCode);
    Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::Info, tr("ebisu_client_download_forbidden_error_title"), errorText, tr("ebisu_client_close"));
}

bool OriginApplication::patchReady(const QString& gameName)
{
    using namespace Origin::UIToolkit;
    OriginWindow *guestUserMsgBox =  new OriginWindow(
        (OriginWindow::TitlebarItems)(OriginWindow::Close),
        NULL, OriginWindow::MsgBox, QDialogButtonBox::NoButton);
    guestUserMsgBox->msgBox()->setup(OriginMsgBox::NoIcon, 
        tr("ebisu_client_game_update_ready_to_apply_title"),
        tr("ebisu_client_game_update_ready_to_apply_text").arg(gameName));
    OriginPushButton *pYesBtn  = guestUserMsgBox->addButton(QDialogButtonBox::Yes);
    pYesBtn->setText(tr("ebisu_client_game_update_button"));
    OriginPushButton *pNoBtn  = 
        guestUserMsgBox->addButton(QDialogButtonBox::No);
    pNoBtn->setText(tr("ebisu_client_not_now"));
    guestUserMsgBox->setDefaultButton(pNoBtn);
    guestUserMsgBox->manager()->setupButtonFocus();
    guestUserMsgBox->setAttribute(Qt::WA_DeleteOnClose, false);
    ORIGIN_VERIFY_CONNECT(pNoBtn, SIGNAL(clicked()), guestUserMsgBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(pYesBtn, SIGNAL(clicked()),  guestUserMsgBox, SLOT(close()));
     guestUserMsgBox->exec();
    bool isApply = ( guestUserMsgBox->getClickedButton() == pYesBtn);
     guestUserMsgBox->deleteLater();
    return isApply;
}

void OriginApplication::setITEActive(bool ite)
{
    mITEActive = ite;
}

void OriginApplication::initLanguageSettings( int & argc, char ** argv )
{
    // Set up our locale right away so we have access to our string db
    QString sBestLocale;
    //Get the available translations
    QDir dir(":/lang/", "EbisuStrings*.xml");

    QStringList files = dir.entryList(QDir::Files|QDir::Readable);

    //Create supported language array - it is stripping out the "EbisuStrings_"
    // part so if you ever change the file prefix, this has to change as well
    foreach(QString sFile, files)
    {
        mLangList.append(sFile.mid(13, 5));
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
        
#ifdef ORIGIN_PC
        sBestLocale = Origin::Services::getNearestLocale(mLangList);
#elif defined (ORIGIN_MAC)
        sBestLocale = Origin::Services::getNearestLocale(mLangList);
#else
        sBestLocale = "en_US";
#endif
    
    }

    Origin::Services::SETTING_LOCALE.updateDefaultValue(sBestLocale);   // update the default value to match the OS UI locale

    // Get the stored locale if available
    QString sSettingsLocale = Origin::Services::readSetting(Origin::Services::SETTING_LOCALE);
    
    if(!sSettingsLocale.isEmpty())
    {
        sBestLocale = sSettingsLocale;
    } 

    // See if the user has specified a locale on the command line.
    // Example command line arg: -locale:en_US
    for(int c = 1; c < argc; c++)
    {
        if(EA::StdC::Stristr(argv[c], "-locale:") == argv[c])
        {
            sBestLocale = argv[c] + strlen("-locale:");
            break;
        }
        else if(EA::StdC::Stristr(argv[c], "-showStringIDs") == argv[c])    // Do not translate -> Show string IDs
        {
            mTranslateStrings = false;
            break;
        }
        else if(strcmp(argv[c], "-AutoStart") == 0) 
        {
            mWasStartedFromAutoRun = true;
        }
    }

    bool bSetLanguageSuccessful = SetAppLanguage(sBestLocale, true);

    if (!bSetLanguageSuccessful)
        bSetLanguageSuccessful = SetAppLanguage("en_US", true);

    ORIGIN_ASSERT(bSetLanguageSuccessful);
}

bool OriginApplication::ingestDesktopConfigFile()
{
    using namespace Origin::Engine::Config;
    ConfigIngestController controller;

    // XXX: Need telemetry for this flow?
    if (controller.desktopConfigExists())
    {
        Origin::Client::ConfigIngestViewController viewController;
        viewController.setTickInterval(1000); // 1 second between updates.
        viewController.setTimeoutInterval(10000); // 10 seconds until the dialog is auto-accepted.
        
        if(viewController.exec() == QDialog::Accepted)
        {
            ConfigIngestController::ConfigIngestError error = controller.ingestDesktopConfig();

            switch(error)
            {
            case ConfigIngestController::kConfigIngestDeleteBackupFailed:
                // Show eacore.ini.bak delete error dialog
                Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::NoIcon,
                    tr("ebisu_client_notranslate_ingest_config_file_title").toUpper(),
                    tr("ebisu_client_notranslate_ingest_config_file_delete_backup_error").arg(tr("ebisu_client_notranslate_config_file")),
                    tr("ebisu_client_notranslate_close"));
                    break;

            case ConfigIngestController::kConfigIngestRenameFailed:
                // Show eacore.ini rename error dialog
                Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::NoIcon,
                    tr("ebisu_client_notranslate_ingest_config_file_title").toUpper(),
                    tr("ebisu_client_notranslate_ingest_config_file_rename_error").arg(tr("ebisu_client_notranslate_config_file")),
                    tr("ebisu_client_notranslate_close"));
                    break;

            case ConfigIngestController::kConfigIngestDeleteConfigFailed:
                // Show desktop eacore.ini delete error dialog
                Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::NoIcon,
                    tr("ebisu_client_notranslate_ingest_config_file_title").toUpper(),
                    tr("ebisu_client_notranslate_ingest_config_file_delete_desktop_error"),
                    tr("ebisu_client_notranslate_close"));
                    break;

            case ConfigIngestController::kConfigIngestCopyFailed:
                // Show new eacore.ini copy error dialog
                Origin::UIToolkit::OriginWindow::alert(Origin::UIToolkit::OriginMsgBox::NoIcon,
                    tr("ebisu_client_notranslate_ingest_config_file_title").toUpper(),
                    tr("ebisu_client_notranslate_ingest_config_file_copy_error"),
                    tr("ebisu_client_notranslate_close"));
                    break;
                
            case ConfigIngestController::kConfigIngestNoError:
            default:
                // If ITO, we want to keep the command line arguments on restarting the client. Sync changes as client process will be terminated
                storeCommandLine();
                if(Origin::Services::SettingsManager::instance())
                    Origin::Services::SettingsManager::instance()->sync();
                // Restart the client so we'll read the newly-placed eacore.ini
                restart(RestartWithCurrentCommandLine);
                return true;
            }
        }
    }
    return false;
}

void OriginApplication::CheckArguments( int & argc, char ** argv )
{
    char parameterBuffer[1024] = {0};
    for(int i = 1; i < argc; i++)
    {
        memset(parameterBuffer, 0, sizeof(parameterBuffer));
        if (sscanf(argv[i], "/authToken:%1023s",  parameterBuffer))
        {
            Origin::Services::writeSetting(Origin::Services::SETTING_CmdLineAuthToken, QString(parameterBuffer));
        }
        else if (strstr(argv[i], "/contentIds:"))
        {
            // Start on the MyGames tab if we detect ITO.
            mExternalLaunchStartupTab = TabMyGames;
        }
        else if (strcmp(argv[i], "/resetPromoTimes") == 0)
        {
            Origin::Services::reset(
                Origin::Services::SETTING_PROMOBROWSER_ORIGINSTARTED_LASTSHOWN, 
                Origin::Services::Session::SessionService::currentSession()
            );
            Origin::Services::reset(
                Origin::Services::SETTING_PROMOBROWSER_GAMEFINISHED_LASTSHOWN, 
                Origin::Services::Session::SessionService::currentSession()
            );
            Origin::Services::reset(
                Origin::Services::SETTING_PROMOBROWSER_DOWNLOADUNDERWAY_LASTSHOWN, 
                Origin::Services::Session::SessionService::currentSession()
            );
        }
        else if (strstr(argv[i], "/downloadConfigFailed"))
        {
            Origin::Services::OriginConfigService::instance()->setDynamicConfigDownloadFailed(argv[i]);
        }
        else if (strstr(argv[i], "/downloadConfigSuccess"))
        {
            Origin::Services::OriginConfigService::instance()->reportBootstrapDownloadConfigSuccess(argv[i]);
        }
    }
}

StartupTab OriginApplication::startupTab() const
{
    // Check what we have stored
    // If we won't read anything sensible from settings then default to the store
    StartupTab startupTab = TabStore;
    QString tabSetting = Origin::Services::readSetting(Origin::Services::SETTING_DefaultTab, Origin::Services::Session::SessionService::currentSession());

    // Check for old 8.2 setting values and update accordingly
    if (tabSetting == "0")
    {
        // My games
        tabSetting = "mygames";
        Origin::Services::writeSetting(Origin::Services::SETTING_DefaultTab, tabSetting, Origin::Services::Session::SessionService::currentSession());
    }
    else if (tabSetting == "1")
    {
        // Store
        tabSetting = "store";
        Origin::Services::writeSetting(Origin::Services::SETTING_DefaultTab, tabSetting, Origin::Services::Session::SessionService::currentSession());
    }

    // Priority should be: external launcher settings > server settings > User settings 
    // external launcher settings
    if(mExternalLaunchStartupTab != TabNone)
    {
        startupTab = mExternalLaunchStartupTab;
    }
    // server settings
    else if(mServerStartupTab != TabNone)
    {
        startupTab = mServerStartupTab;
    }
    // user Settings
    else if (tabSetting == "store")
    {
        startupTab = TabStore;
    }
    else if (tabSetting == "mygames")
    {
        startupTab = TabMyGames;
    }
    else if (tabSetting == "decide")
    {
        startupTab = TabLetOriginDecide;
    }

    return startupTab;
}

void OriginApplication::onGeoCountryResponse()
{
    Origin::Services::GeoCountryResponse *resp = dynamic_cast<Origin::Services::GeoCountryResponse*>(sender());
    if(resp)
    {
        QString countryCode = resp->geoCountry();
        ORIGIN_LOG_EVENT << "Country code:" << countryCode;
        Origin::Services::writeSetting(Origin::Services::SETTING_GeoCountry, countryCode);
        Origin::Services::writeSetting(Origin::Services::SETTING_CommerceCountry, resp->commerceCountry());
        Origin::Services::writeSetting(Origin::Services::SETTING_CommerceCurrency, resp->commerceCurrency());
    }
}

void OriginApplication::onGeoCountryError(Origin::Services::restError code)
{
    Origin::Services::GeoCountryResponse* response = dynamic_cast<Origin::Services::GeoCountryResponse*>(sender());
    if (response)
        ORIGIN_LOG_ERROR << "Failed to retrieve geocountry from server - rest code: " << code << "(" << response->errorString() << ")";
}

bool OriginApplication::handleMessage (QString cmdLine, bool useCmdLineAsIs)
{
    bool handled = false;
    Origin::Client::CommandFlow *commandFlow = Origin::Client::CommandFlow::instance();
    if(commandFlow)
    {
        commandFlow->setCommandLine (cmdLine);

        QString argument;
        if (useCmdLineAsIs)
        {
            //if this is coming from the Mac, we don't want to do the kludgy manipulations that are necessary since it is doing its own percent encoding manipulations...
            argument = cmdLine;
        }
        else
        {
            //can't set it directly to cmdLine because there might be some processing that happened as a result of passing it thru
            //CommandFlow::setCommandLine, e.g. to fix IE9 replaces %20 with space, so we need to fix it back to %20
            argument = commandFlow->commandLine();
        }

        //the command line is stored now, but we don't want to actually act on the request
        //until we're sure that MainFlow and RTPFlow are property started
        //commandFlow will be checked again in performPostConstructionInitialization so we're not throwing away this request, just postponing the start
        if (commandFlow->isActionPending() && Origin::Client::MainFlow::instance() && Origin::Client::MainFlow::instance()->rtpFlow() != NULL)
        {
            QUrl actionUrl = commandFlow->getActionUrl();   //it could return an empty URL if parsing failed
            if (!actionUrl.isEmpty())
            {
                // start the RTP game launch timer if we're performing RTP
                if (actionUrl.toString().contains("game/launch", Qt::CaseInsensitive))
                    GetTelemetryInterface()->Metric_PERFORMANCE_TIMER_START(EbisuTelemetryAPI::RTPGameLaunch_running);

                bool clearCommandParams = (commandFlow->isLaunchedFromBrowser() || commandFlow->isOpenAutomateParam());
                Origin::Client::PendingActionFlow::instance()->startAction(actionUrl, clearCommandParams);
            }
        }

        handled =  Origin::Client::MainFlow::instance()->handleMessage(argument);
    }

    return handled;
}

void OriginApplication::processCommandLineSwitches()
{
    for(int c = 1; c < mArgc; c++)
    {
        if (EA::StdC::Stristr(mArgv[c], "-DisableCrash") == mArgv[c])
        {
            //  Check if the crash handler needs to be disabled.
            //  This is used by the automation team.
            mbEnableException = false;
        }
        else if (EA::StdC::Stristr(mArgv[c], "-ForceException") == mArgv[c])
        {
            mbForceException = true;
        }
        else if (EA::StdC::Stristr(mArgv[c], "-ForceCrash") == mArgv[c])
        {
            mbForceException = true;
        }
        else if (EA::StdC::Stristr(mArgv[c], "/free") == mArgv[c])
        {
            mIsFreeToPlay = true;
        }
    }

    //  Initialize the exception handling
    if (mbEnableException)
    {
        mExceptionSystem.start();
    }
}
