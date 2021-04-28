///////////////////////////////////////////////////////////////////////////////
// OriginApplication.h
//
// Copyright (c) 2010 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef OriginApplication_H
#define OriginApplication_H

#include <QApplication>
#include <QMap>
#include <QScopedPointer>


#include "services/exception/ExceptionSystem.h"
#include "services/network/ClientNetworkConfig.h"
#include "services/plugin/PluginAPI.h"
#include "../../flows/MainFlow.h"
#include "services/settings/SettingsManager.h"
#include "engine/login/User.h"
#include "services/debug/DebugService.h"
#include "services/settings/StartupTab.h"

#if defined (ORIGIN_PC)
#include "qt_windows.h"
#endif

// Used to identify the app and Window uniquely throughout the system.
#define EBISU_APP_NAME               L"OriginApp"   // Global\ means Mutex is system wide, not session wide
#define EBISU_WINDOW_CLASS           L"QWidget"
#define EBISU_INTERNAL_WINDOW_TITLE  L"Origin"
#define EBISU_ACCOUNT_NAME           "Origin"

static const int kWin32ActivateIdentifier = 124540; // Sent to the WinProc of the other instance to be activated.
static const int k3rdPartyDll = 124541;             // Sent from IGO DLL to pass the third party DLL name
static const int kGameTelemetry = 124542;           // Sent from IGO DLL to record some telemetry info

class OriginTranslator;
class MandatoryUpdateWatcher;
class Logger;

namespace Origin 
{
/// \brief The %UIToolkit namespace provides the various Qt UI widgets.
///
/// <span style="font-size:14pt;color:#404040;"><b>See the relevant wiki docs:</b></span> <a style="font-weight:bold;font-size:14pt;color:#eeaa00;" href="https://developer.origin.com/documentation/display/EBI/UI-Qt">UI-Qt</a>
namespace UIToolkit
{
    class OriginWindow;
    }

namespace Client
{
    class TelemetryManager;
}

namespace SDK
{
    namespace Lsx
    {
        class LSXThread;
    }
}

}

namespace Content
{
    class Platform;
};


enum NetworkConfigReadState
{
    ConfigReadPending,
    ConfigReadFailed,
    ConfigReadSucceeded,
    // Means our configuration has been read and processed
    ConfigProcessed
};

enum ExitReason
{
    ReasonUnknown = 0,
    ReasonSystrayExit,
    ReasonLoginExit,
    ReasonLogoutExit,
    ReasonException,
    ReasonEarlyRestart,     // Used for /uninstall and eacore.ini ingestion
    ReasonRestart,
    ReasonEscalationHelperDeclined,
    ReasonOS                //On PC, got WM_QUERYENDSESSIONMESSAGE
};

enum RestartMethod
{
    RestartClean,
    RestartWithCurrentCommandLine
};

enum StartupState
{
    StartNormal,
    StartInTaskbar,
    StartInSystemTray,
    StartMaximized
};

class ORIGIN_PLUGIN_API OriginApplication : public QApplication, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:

    // convenience accessor to the application "singleton"
    static OriginApplication& instance();

    OriginApplication(int &argc, char **argv);

    ~OriginApplication();

    const QString& locale() const { return mLocale; }

    const QString& commandLine(int& argc, char**& argv) { argc = mArgc; argv = mArgv; return mCommandLine; }
    const QString& GetEnvironmentName() const { return mEnvironmentName; }
    void configCEURL(QString& settingValue, const QString& authCode = QString());
    bool GetEbisuCustomerSupportInBrowser(QString & pCustomerSupport, const QString& authCode = QString()) const;

    bool BlockingBehaviorEnabled() const { return mBlockingBehaviorEnabled; }   
    const int& qaForcedUpdateCheckInterval() const  { return mQaForcedUpdateCheckInterval;}
    bool forceLockboxURL() const { return mForceLockboxURL; }

    bool abortDueToGuestUser();
    bool showAppNotAllowedError();
    bool showCompatibilityModeWarning();
    void showLimitedUserWarning(bool fromITE_Flow = false);
    bool getWasStartedFromAutoRun() const { return mWasStartedFromAutoRun; }
    StartupState startupState () const { return mStartupState; }
    
    void SaveOriginIdToEmailUserInfo(Origin::Engine::UserRef user);
    // SetTranslator has been replaced by SetAppLanguage, which is much more modular
    bool SetAppLanguage(const QString &locale, bool updateQSS = true );

#if defined(EA_DEBUG)
    void ShowLocalizationKey(const bool showLocalizationKey);
#endif

    // Debugging functions
    void dumpEACoreToLog();

    // Localization
    void initEAText();
    void shutdownEAText();

    // Configuration
    void readConfiguration();

    bool EACoreIniExists();

    /// \fn detectDeveloperMode
    /// \brief Detects if the user has enabled Developer Mode for this machine.
    /// 
    /// Developer Mode is enabled by launching Origin Developer Tool 2.0 or later.
    void detectDeveloperMode();

    //InstallThroughEbisu
    bool isITE();
    void setITEActive(bool ite);

    bool IsLimitedUser() const { return mbLimitedUser; }

    static QMap<QString, QString>   mCustomerSupportLocales;

    QStringList& getOverrideDownloadPaths(){return mOverrideDownloadPaths;}

    // Single login
    bool isSingleLoginEnabled() const { return mSingleLoginEnabled; }

    // Tab to show on startup
    StartupTab startupTab() const;
    void SetServerStartupTab(StartupTab tab) { mServerStartupTab = tab; }
    void SetExternalLaunchStartupTab(StartupTab tab) { mExternalLaunchStartupTab = tab; }

    bool GetReceivedQueryEndSession() const { return mReceivedQueryEndSession; }
    void ResetQueryEndSessionFlag() { mReceivedQueryEndSession = false; }

    int exec();

    /// \brief returns true if we are in the process of shutdown, otherwise it returns false.
    bool isAppShuttingDown() const;

signals:
    // Note: Emitted after APP has processed related base user actions
    void gameDownloadError(const QString&, const QString&);
    void updateDownloadError();
    void jitUrlRetrievalError (const QString &);
    void silentAutoUpdateDownloadError ();
    void generalCoreError();
    void checkRTP();
    void gotCloseEvent();
    void deviceChanged(); // XP
    void stopLSXThread();

private slots:
    
    /// Performs any initialization necessary after the constructor exits.
    /// This can initialize things that are unable to be done in the constructor.
    void performPostConstructionInitialization();
    
    public slots:

    void  quit(ExitReason);
    void  restart(RestartMethod method = RestartClean);
    void  cancelTaskAndExit(ExitReason);
    void  onAboutToQuit();

        // 403 Error from Core while downloading a game
        void showDownloadingError(const QString&, const QString&);
        // 404 Error from Core while downloading an update
        void showUpdateDownloadingError();
        //115 Error from Core when unable to retrieve JIT Url
        void showJITUrlRetrievalError (const QString &);

    void applyStylesheet();

        //Auto-Patching messages
        bool patchReady(const QString& gameName);

#if defined(ORIGIN_MAC)
    /// Handle URLs opened from web browsers.
    void openUrlFromBrowser(QUrl url);
#endif
    
        /// \ingroup OriginApplication
        /// \class OriginApplication
        /// \brief Gets a list of the supported language. 
        const QStringList& LanguageList() const { return mLangList; }

private:


    /// \brief to define whether to send or not high-number telemetry hooks
    const static int SENDHOOKTHRESHOLD;

    /// \brief we might need a Exception, crash or we are free to play
    void processCommandLineSwitches();

    /// \brief tests for premature termination of the application. If found it was premature, it generates telemetry.
    void testPrematureTermination();

    void sendResponseToError( const QString& sHandle, const QString& sOption);

    // LSX
    void startLSXThread();

    void ExecNoWait(wchar_t* executablePath, wchar_t* args, wchar_t* workingDir);

    void readVariables();

    // Constructor helper functions.
    void CheckArguments( int & argc, char ** argv );
    void initLanguageSettings( int & argc, char ** argv );
    bool ingestDesktopConfigFile();

    /// \brief Check for repeated login settings and fixes it
    void loginSettings(Origin::Engine::UserRef user);

    /// \brief Dumps basic user settings to the log
    void printUserSettings();

#ifdef ORIGIN_PC
    void ProcessGameTelemetryMsg(PCOPYDATASTRUCT msg);
    QString fixOriginProtocolCmdLine (QString& cmdLine);
#endif

    /// \brief sets the commandline in CommandFlow and triggers pendingActionFlow if it is an origin2:// or origin2:// message, otherwise, calls MainFlow's handleMessage
    bool handleMessage (QString cmdLine, bool useCmdLineAsIs);

    private slots:

        void forceQuitOnIgoStateChangeTimeout();
        void finishQuitOnIgoStateChange(bool igoState);

        bool GetStyleSheetFromFile(const QString& styleSheetPath, QString& styleSheet);

        void storeCommandLine();

        /// \brief Called when user has logged in
        void onUserLoggedIn(Origin::Engine::UserRef user);

        // setting-related notifications
        void onSettingChanged(const QString& settingKey, Origin::Services::Variant const& value);
        void onSettingsSaved();

        void onGeoCountryResponse();
        void onGeoCountryError(Origin::Services::restError code);

protected:

#ifdef ORIGIN_MAC
    virtual bool event(QEvent* e);
#endif
    bool eventFilter(QObject* obj, QEvent* event);
#if defined(ORIGIN_PC)
    bool disableUIPI(HWND hwnd, unsigned int msg);
    bool winEventFilter(MSG* message, long* result);
    HWND mOriginDetectionWindow;
    static LRESULT WINAPI OriginDetectionWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif
    virtual bool nativeEventFilter(const QByteArray & eventType, void* message, long* result) Q_DECL_OVERRIDE;

public:
#ifdef ORIGIN_MAC
    void onClickDock();
    void onDockQuit();
#endif
    
private:
    int             mArgc;
    char**          mArgv;
    QString         mLocale;
    QString         mIniFilePath;           // Path to Origin_App.ini
    QString         mLogConfigFilePath;     // Path to EALogConfig.ini
    QString         mCommandLine;
    QString         mEnvironmentName;
    QStringList     mLangList;
    QStringList     mOverrideDownloadPaths;

    // Default start-up tab by source
    StartupTab mExternalLaunchStartupTab; // Valid when Origin is launched from an external source (ITO, web store, etc.)
    StartupTab mServerStartupTab;         // The default tab returned from server

    Origin::Services::Exception::ExceptionSystem mExceptionSystem;

    int             mQaForcedUpdateCheckInterval; 

#if defined(ORIGIN_MAC)
    QTranslator mQtDefaultTranslator;
#endif

    // Languages are small and if we ever delete one we run the risk
    // of breaking another thread which might be reading from it at
    // that moment. We reference them by locale (eg. "en_US").
    QMap<QString,OriginTranslator*> mTranslators;

    NetworkConfigReadState mNetworkConfigReadState;
    Origin::Services::ClientNetworkConfig mNetworkConfig;
    Origin::SDK::Lsx::LSXThread *mLSXThread;
    QScopedPointer<Origin::Client::TelemetryManager> mTelemetryManager;

    bool mBlockingBehaviorEnabled;
    bool mQADebugLang;
    bool mSingleLoginEnabled;
    bool mForceLockboxURL;
    bool mCrashOnStartup;
    bool mITEActive;
    bool mbLimitedUser;
    bool mWasStartedFromAutoRun;
    bool mHasQuit;
    bool mReceivedQueryEndSession;
    StartupState mStartupState;
    bool mbForceException;                              // Used for testing exception handling.
    bool mbEnableException;
    bool mTranslateStrings;
    bool mIsFreeToPlay; // so mark if it is free to play
};

inline OriginApplication& OriginApplication::instance()
{
    ORIGIN_ASSERT(qApp != NULL);
    return *static_cast<OriginApplication*>(qApp);
}




#endif // Header include guard
