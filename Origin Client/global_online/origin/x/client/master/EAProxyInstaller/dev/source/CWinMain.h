#ifndef UNICODE
#define UNICODE
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0500		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0500	// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		  // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0500 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <EABase/eabase.h>
#include <afxwin.h>
#include <Afxtempl.h>
#include <iostream>
#include <string>
#include <map>

class CWinMain : public CWinApp
{
public:
	BOOL InitInstance		 ( void );								  // Application start.
	void GetCommandLineParams( std::map<CString, CString> & params ); // Stores the command line parameters of *this* process into a map.

	void OpenPath_ShellExec	 ( const CString & sFullPath, 
							   const CString & sCmdLineArgs, 
							   const CString & sCurrentDir, 
							   bool bShowUI = true, 
							   bool bWait = true); 

	bool OpenPath_CreateProcess(
				const CString &sFullPath,
				const CString &sCmdLineArgs,
				const CString &sCurrentDir,
				bool bShowUI=true,
				bool bLowerElevation = false,
				bool bWait = true);					  // Launches a process.
	bool WaitForProcessGroup( HANDLE hProcessGroup );
};

