//
// NSIS installer extension DLL
// 
extern "C" BOOL PASCAL EXPORT GetCurrentCachePath(wchar_t* sEADMCachePath,  // In/Out: Really, it's just a buffer for the command path
										   		  size_t nBufferSize);		// In: The number of bytes in the buffer

extern "C" BOOL PASCAL EXPORT GetCurrentLocale(wchar_t* sLocale,		// In/Out: just a buffer
											   size_t nBufferSize);		// In: The number of bytes in the buffer

//extern "C" BOOL PASCAL EXPORT ShutdownEACore();
extern "C" BOOL PASCAL EXPORT ShutdownProxyInstaller();

extern "C" BOOL PASCAL EXPORT RelativizeManifestPaths(LPCTSTR sCacheDir,		//Cache dir
													  LPCTSTR sOriginalPath);	//Path to use as base (cache dir if empty)

extern "C" void PASCAL EXPORT CleanUserConfigDir();

extern "C" void PASCAL EXPORT GrantEveryoneAccessToFile(wchar_t* sFileName);

extern "C" BOOL PASCAL EXPORT GrabDBGPrivilege();

extern "C" BOOL PASCAL EXPORT IsUpdate();

extern "C" BOOL PASCAL EXPORT GetLockingProcessList(wchar_t *processList, DWORD numberOfElements);

extern "C" void PASCAL EXPORT RunProcess(wchar_t *fullProcessName, wchar_t *fullProcessFolder, bool wait,  wchar_t *cmdLine = NULL);

extern "C" BOOL PASCAL EXPORT MigrateCacheAndSettings();

extern "C" void PASCAL EXPORT ShowProgressWindow(wchar_t* message, bool start);

extern "C" void PASCAL EXPORT KillProcess(wchar_t* sProcName, bool respectPath);

extern "C" BOOL PASCAL EXPORT FindProcess(wchar_t* sProcName, bool respectPath);

extern "C" BOOL PASCAL EXPORT DoPendingDownloadsExist();

extern "C" BOOL PASCAL EXPORT IsUserInCanada();