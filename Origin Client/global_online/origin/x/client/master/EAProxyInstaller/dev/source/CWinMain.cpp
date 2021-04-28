/* ===========================================================================================
	Name: ProxyInstaller
	
	Purpose: o This application copies the staging keys from the HKEY_CURRENT_USER of a 
			   standard user to the HKEY_CURRENT_USER of an admin user and then executes an 
			   installer that utilizes those keys.
	

	Notes: o The process this application performs is:

				- Self-elevate upon execution on a UAC enabled operating system (ex. Vista).
				- Import a .reg file into *this* processes HKEY_CURRENT_USER that contains 
				  the following registry branch from the the calling process:

						HKEY_CURRENT_USER/Software/Electronic Arts/EA Core/Staging

				- Remove the .reg file that was imported.
				- Execute an external installer whose path and parameters are sent via 
				  command-line options to this process:

						/proxyFullPath=<path to processes>
						/proxyCmdLineArgs=<Optional command line arguments for process>
						/proxyCurrentDir=<Optional current directory for process>
						/proxyShowUI=<[0|1] Optional flag to show or hide the UI for the process. A missing parameter or empty value is interpreted as '1' (true)>
						/proxyRegPath=<Optional path to the .reg file to import. No importing is attempted if this paremter is missing or its value is empty>
	
		   o A calling process should perform the following operations:

				- Optionally Export HKEY_CURRENT_USER/Software/Electronic Arts/EA Core/Staging 
				  to a .reg file.
			    - Call this process with the necessary command line options. For example:

						ProxyInstaller /proxyFullPath=C:\Installer.exe /proxyCmdLineArgs=/i /e /proxyCurrentDir=C:\ /proxyShowUI=1 /proxyRegPath=C:\staging.reg

		   o Game installers look for staging keys in the HKEY_CURRENT_USER of an admin 
		     account. If EA Link launches an installer from a standard user account on 
		     Windows Vista then the staging keys will be put in the HKEY_CURRENT_USER of the 
		     standard account and the game installer willl be searching for non-existent  
			 branch in its admin HKEY_CURRENT_USER branch. 

		   o The registry import will occur regardless of whether or not EA Link is being
		     run as a standard or admin user.

		   o If the registry import fails then the installer will still be launched. 

		   o When not in debug mode, errors will not be shown to the user (same behavior
		     as EAL) but will be sent to OutputDebugString(). Use external applications such 
			 as DebugView to view this application's logs if errors are suspect.

		   o regedit.exe is executed silently and failures will therefore be supressed.
   =========================================================================================== */

#pragma warning(disable:4266) // no override available for virtual member function from base 'CException'; function is hidden


#include "CWinMain.h"
#include <shlwapi.h>
#include <WinSafer.h>
#include <SDDL.h>

CWinMain theApp;
HANDLE ghExitEvent = NULL;

/* ===========================================================================================
	Purpose: Application start. It parses the command line, imports the .reg file, and 
			 executes the installer.

	Returns: TRUE - Always.
  =========================================================================================== */
BOOL CWinMain::InitInstance( void )
{
	// Command line parameter constants.
	const CString SPROXY_FULLPATH	 = L"/proxyFullPath";	 // Path to the installer process.
	const CString SPROXY_CMDLINEARGS = L"/proxyCmdLineArgs"; // (Optional) Command line arguments for the installer process.
	const CString SPROXY_CURRENTDIR	 = L"/proxyCurrentDir";	 // (Optional) Current directory for the installer process.
	const CString SPROXY_SHOWUI		 = L"/proxyShowUI";		 /* (Optional) [0|1] Whether or not to show or hide the UI of the installer process. 
															    A missing parameter or empty value is interpreted as '1' (true)					 */
	const CString SPROXY_REGPATH	 = L"/proxyRegPath";	 /* (Optional) Path to a .reg file containing an HKCU import. No importing is 
																attempted if this parameter is missing or its value is empty			  */

	const CString SPROXY_LOWERELEVATION	 = L"/proxyLowerElevation";	 // (Optional) Lower the elevation privilege of the execution to USER
	const CString SPROXY_WAIT		 = L"/proxyWait";		 // Optional  Specify that proxy installer should wait for launched process to exit. True by default.

	// Parameter info.
	std::map<CString, CString> params; // Map of command line parameters.


	// -----------------------------------------------------------
	// Get the command line parameters for all /proxyXXX= entries.
	//

	GetCommandLineParams( params );


	// ---------------------------------------------------------------------------------------------------------------------------
	// Silently import an associated .reg file if needed. This is typically a set of keys from a user's HKEY_CURRENT_USER branch.
	//

	// If we have a path to a .reg file and it exists.
	if ( !params[SPROXY_REGPATH].IsEmpty() && ::PathFileExists( params[SPROXY_REGPATH] ) ) 
	{	
		OpenPath_ShellExec( L"regedit.exe", L"/s \"" + params[SPROXY_REGPATH] + L"\"", L"", true, true );
		::DeleteFile( params[SPROXY_REGPATH] );
	}

	ghExitEvent = CreateEvent(NULL, TRUE, FALSE, _T("ProxyInstallerShutdownEvent"));
	ASSERT( ghExitEvent != NULL );

	// Check for existence of requested file before we cause silly & confusing error dialogs
	if (!PathFileExists(params[SPROXY_FULLPATH]))
	{
		if (!params[SPROXY_FULLPATH].IsEmpty())
        {
            ASSERT(false);
		    ::OutputDebugString( L"Invalid file specified" );
        }
		return FALSE;
	}

	// --------------------------
	// Execute the real process.
	//

	OpenPath_CreateProcess( params[SPROXY_FULLPATH], params[SPROXY_CMDLINEARGS], 
			params[SPROXY_CURRENTDIR], (( params[SPROXY_SHOWUI] == L"0" ) ? false : true), 
			(( params[SPROXY_LOWERELEVATION] == L"true" ) ? true : false),
			(( params[SPROXY_WAIT] == L"0" ) ? false : true) );  // Default to true

	if ( ghExitEvent )
	{
		CloseHandle(ghExitEvent);
		ghExitEvent = NULL;
	}
	
	return TRUE;
}

/* ===========================================================================================
	Purpose: Parses *this* processes command line for parameters and stores the parameters 
			 and their values in a std::map.

	Parameters:	params - Map of parameters.

	Notes: o All parameters for *this* process are of the syntax: 
	
				/proxy<Some Name>=<Some Value>
				/proxyFullPath=C:\EACore\RC_5.x\Core\DownloadManager\runtime\Core.exe

		   o The order of parameters does not matter. Only the syntax does.
  =========================================================================================== */
void CWinMain::GetCommandLineParams( std::map<CString, CString> & params )
{
	// Command line parameter parsing constants.
	const CString SPROXY_PARAM	   = L"/proxy"; // All parameter names have this prefix.
	const CString SPROXY_DELIMITER = L"=";      // Delimiter between the parameter name and value.

	// Parameter info.
	CString sCommandLine( ::GetCommandLineW() ); // The command line of *this* process.
	CString sProxyParam;						 // The full name of the current parameter.
	CString sProxyValue;						 // The full value of the current parameter.
	int		nProxyParamIndex;					 // Index to the starting location of the current parameter in the command line.
	int		nProxyDelmiterIndex;				 // Index to the delimiter between the current parameter and its value in the command line.

	// Miscellaneous.
	CString sMsg; // General message.

	params.clear();

	// ---------------------------------------------------------
	// Parse the command line for all the /proxyXXX= parameters.
	//

	nProxyParamIndex = sCommandLine.Find( SPROXY_PARAM ); // First parameter.

	// While parameters exist
	while ( nProxyParamIndex >= 0 ) 
	{
		nProxyDelmiterIndex = sCommandLine.Find( SPROXY_DELIMITER, nProxyParamIndex ); // Get the delimiter between the parameter and its value.

		// If the delimiter exists.
		if ( nProxyDelmiterIndex >= 0 )
		{
			int		nNextProxyParamIndex;				 // Index to the starting location of the next parameter in the command line.
			nNextProxyParamIndex	= sCommandLine.Find( SPROXY_PARAM, nProxyDelmiterIndex ); // Look ahead for the next parameter.
			sProxyParam = sCommandLine.Mid( nProxyParamIndex, nProxyDelmiterIndex - nProxyParamIndex ).Trim();

			// Does the next parameter exist?
			if ( nNextProxyParamIndex >= 0 ) { sProxyValue = sCommandLine.Mid( nProxyDelmiterIndex + 1, nNextProxyParamIndex - ( nProxyDelmiterIndex + 1 ) ); }
			else							 { sProxyValue = sCommandLine.Mid( nProxyDelmiterIndex + 1 ); }

			params[ sProxyParam ] = sProxyValue.Trim(); // Store the parameter and value.

			// Output the parameter and value.
			sMsg.Format( L"CWinMain::GetCommandLineParams() Parameter %s=%s\r\n", sProxyParam, sProxyValue );			
			::OutputDebugString( sMsg );

			// Next parameter.
			nProxyParamIndex = nNextProxyParamIndex;
		}
		else
		{
			break; // No delimiter present between the most recent parameter and its value.
		} // if ( nProxyDelmiterIndex >= 0 )

	} // while ( nProxyParamIndex >= 0 )

}

/* ===========================================================================================
	Purpose: Launches a process.

	Parameters:	sFullPath		   - Fully qualified path to the process.
				sCmdLineArgs	   - Command line parameters for the process.
				sCurrentDir		   - Current directory for the process.
				bShowUI (Optional) - Whether or not to show the UI. The default is true.
				bWait   (Optional) - Whether or not to wait for the process to finish 
									 execution. The default is false.
	Notes: o All parameters for *this* process are of the syntax: 

			  /proxy<Some Name>=<Some Value>

		   o The order of parameters does not matter. Only the syntax does.
   =========================================================================================== */
void CWinMain::OpenPath_ShellExec( const CString & sFullPath, const CString & sCmdLineArgs, const CString & sCurrentDir, bool bShowUI /*=true*/, bool bWait /*=true*/ )
{
	const CString SVERB = L"open";
	
	// Process information.
	SHELLEXECUTEINFO shellExecuteInfo;
	CString			 sErrMessage;

	// Miscellaneous.
	CString sMsg; // General message.

	// Quick validation... Do we have a path?
	if ( !sFullPath.IsEmpty() ) // Yes we have a path.
	{
		// -----------------------------
		// Prepare for a ShellExecute()
		//

		ZeroMemory( &shellExecuteInfo, sizeof( shellExecuteInfo ) );
		shellExecuteInfo.cbSize		  = sizeof( SHELLEXECUTEINFO );
		shellExecuteInfo.fMask		  = SEE_MASK_FLAG_NO_UI; // Make the process a group jobless root. 

		if ( bWait ) shellExecuteInfo.fMask |= SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;

		shellExecuteInfo.hwnd		  = 0;
		shellExecuteInfo.lpVerb		  = SVERB;
		shellExecuteInfo.lpFile		  = sFullPath;
		shellExecuteInfo.lpParameters = sCmdLineArgs;
		shellExecuteInfo.lpDirectory  = sCurrentDir;
		shellExecuteInfo.hProcess	  = 0;
		shellExecuteInfo.nShow		  = bShowUI ? SW_SHOW : SW_HIDE;


		// -------------------------
		// Perform the ShellExecute()
		//
		
		sMsg.Format( L"CWinMain::OpenPath( %s, %s )\r\n", sFullPath, sCmdLineArgs );
		::OutputDebugString( sMsg );

		// Did the the process launch correctly?
		if ( !::ShellExecuteEx( &shellExecuteInfo ) ) // No.
		{
			int				 result;
			// Capture the error message.
			#pragma warning( disable : 4311 ) // warning C4311: 'type cast' : pointer truncation from 'HINSTANCE' to 'int'
				result = (int) shellExecuteInfo.hInstApp;
			#pragma warning( default : 4311 )

			// Begin an error string.
			sErrMessage.Format( L"ShellExecute Failed: \nexe: '%s %s' \ncurrentDir: '%s'\n", sFullPath, sCmdLineArgs, sCurrentDir );

			// Append details to the error string.
			switch ( result )
			{
			case SE_ERR_FNF:
				sErrMessage.Append( L"The specified file was not found." );
				break;

			case SE_ERR_PNF:
				sErrMessage.Append( L"The specified path was not found." );
				break;

			case SE_ERR_ACCESSDENIED:
				sErrMessage.Append( L"The operating system denied access to the specified file." );
				break;

			case SE_ERR_SHARE:
				sErrMessage.Append( L"A sharing violation occurred." );
				break;

			default:
				sErrMessage.Append( L"An error occurred with error code: %d", result );
				break;
			}

			// Output the error string.
			::OutputDebugString( sErrMessage );

			#ifdef _DEBUG
				::MessageBox( NULL, sErrMessage, L"Error", MB_OK );
			#endif
		} // if ( !::ShellExecuteEx( &shellExecuteInfo ) )


		// --------------------------------
		// Wait for the process if needed.
		//

		sErrMessage.Format( L"Process Handle: %X", shellExecuteInfo.hProcess);
		::OutputDebugString( sErrMessage );

		// Do we need to wait for process completion?
		if ( shellExecuteInfo.hProcess )
		{
			if ( ghExitEvent )
			{
				HANDLE handles[2];
				handles[0] = shellExecuteInfo.hProcess;
				handles[1] = ghExitEvent;
				::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
			}
			else
			{
				::WaitForSingleObject( shellExecuteInfo.hProcess, INFINITE ); // Wait.
			}
			
			::CloseHandle( shellExecuteInfo.hProcess );	// Cleanup.
		} // if ( shellExecuteInfo.hProcess )...

	} 
	else // No path.
	{
		// Begin an error string.
		sErrMessage.Format( L"ShellExecute Failed: Path is empty\n" );

		// Output the error string.
		::OutputDebugString( sErrMessage );

		#ifdef _DEBUG
			::MessageBox( NULL, sErrMessage, L"Error", MB_OK );			
		#endif
	} // if ( sFullPath.IsEmpty() )
}

// This function always waits for the process to finish (using WaitForProcessGroup)
bool CWinMain::OpenPath_CreateProcess(const CString &sFullPath, 
	const CString &sCmdLineArgs, 
	const CString &sCurrentDir, 
	bool bShowUI /*=true*/, 
	bool /* bLowerElevation  false */, 
	bool  bWait /*  true */)
{
	CString sErrMessage;

	{
		HANDLE hProcess = NULL;

		{
			//  Create the process with the normal user compatible access token.
			STARTUPINFO startupInfo;
			memset(&startupInfo, 0, sizeof(startupInfo));
			startupInfo.cb = sizeof(startupInfo);
			startupInfo.dwFlags = STARTF_USESHOWWINDOW;
			startupInfo.wShowWindow = bShowUI ? SW_SHOW : SW_HIDE;
			PROCESS_INFORMATION processInfo;

			// CreateProcessXXXX does some very odd things and command arguments, current directory etc. don't aways work as expected
			// so use second argument for command AND arguments and use CD to change directory instead of unreliable curdir arg.
			// See  http://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx
			// For max length for command line parameter in CreateProcess
			const static size_t FULLCMDLEN = 32768;
			wchar_t sFullCmd[FULLCMDLEN+1] = L"";

			// If the full path is not quoted, then quote it
			// This is due to legacy GameLauncher that assumes the command line contains
			// a quoted command - it auto strips the first and last character from "GetCommandLine" (doh!)
			int res(0);
			if (  !sFullPath.IsEmpty()
				&& sFullPath[0] != '"' 
				&& sFullPath[sFullPath.GetLength()-1] != '"')
				res = swprintf_s(sFullCmd, _countof(sFullCmd)-1, L"\"%s\" %s", sFullPath, sCmdLineArgs);
			else
				res = swprintf_s(sFullCmd, _countof(sFullCmd)-1, L"%s %s", sFullPath, sCmdLineArgs);

			if (0 > res)
				return false;

			//			CoreLogMessage(L"Open Path (%s)", sFullCmd);

			bool bProcSuccess = false;
			// Note current directory

			const static size_t DIRPATHLEN = 8192;
			TCHAR sRememDir[DIRPATHLEN + 1] = L""; 
			res = ::GetCurrentDirectory(_countof(sRememDir)-1, sRememDir);

			if (0 == res)
				return false;

			// Change to specified directory
			if (sCurrentDir.IsEmpty() || ::SetCurrentDirectory(sCurrentDir))
			{
				// Kick off process
				//				bProcSuccess = (CreateProcessAsUserW( hAccessToken, NULL, sFullCmd, 
				//					NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &startupInfo, &processInfo ) == TRUE);
				bProcSuccess = CreateProcess( sFullPath,   // No module name (use command line)
					sFullCmd,        // Command line
					NULL,           // Process handle not inheritable
					NULL,           // Thread handle not inheritable
					FALSE,          // Set handle inheritance to FALSE
					CREATE_NEW_PROCESS_GROUP|CREATE_BREAKAWAY_FROM_JOB,              // required to let us monitor the process
					NULL,           // Use parent's environment block
					sCurrentDir,    // Starting directory 
					&startupInfo,            // Pointer to STARTUPINFO structure
					&processInfo ) == TRUE;           // Pointer to PROCESS_INFORMATION structure

				if (!bProcSuccess)
				{
#ifdef _DEBUG
					CString sError;
					sError.Format(L"CreateProcess failed (%d).\n", GetLastError());
					MessageBox(NULL, sError , L"Error", MB_OK);
#endif
					return false;
				}

				// Restore current directory
				if (!sCurrentDir.IsEmpty())
					::SetCurrentDirectory(sRememDir);
			}

			if (bProcSuccess)
			{
				hProcess = processInfo.hProcess;
				// Process cleanup.
				//					CloseHandle( processInfo.hProcess );
				CloseHandle( processInfo.hThread );
			}
			else
			{
				sErrMessage.Format(L"CreateProcess(%s) failed with error %u.", sFullCmd, GetLastError());
				::OutputDebugString(sErrMessage);

#ifdef _DEBUG
				MessageBox(NULL, sErrMessage, L"Error", MB_OK);
#endif
			}
		}  // if ( SaferComputeTokenFromLevel...

		// Failed?
		if (NULL == hProcess)
			return false;

		if (bWait)
			WaitForProcessGroup(hProcess);

		// Success
		CloseHandle(hProcess);
		return true;
	}

#ifdef _DEBUG
	// Error Handling
	int iResult = GetLastError();
	sErrMessage.Format(L"CreateProcessAsUser FAILED (%d)", iResult);

	MessageBox(NULL, sErrMessage, L"Error", MB_OK);
#endif

	return false;


}

bool CWinMain::WaitForProcessGroup( HANDLE hProcessGroup )
{
	const std::wstring ID = L"Win32ProcessUtils::WaitForProcessGroup() ";

	JOBOBJECT_ASSOCIATE_COMPLETION_PORT portID;				  // IO completion port identification.
	HANDLE								hIOCP = NULL;		  // IO completion port handle.
	HANDLE								hJob = NULL;		  // Job handle.
	BOOL								bQueueExists = TRUE;  // Whether or not the notification queue of an IO Completion Port exists.
	DWORD								nJobStatus   = 0;	  // Status of the job the process group is bound to.
	LPOVERLAPPED						pOverlapped;		  // A dummy API parameter not used.
	ULONG_PTR							nCompletionKey;		  // A dummy API parameter not used.
	bool								bRC			 = FALSE; // Return code.
#ifdef _DEBUG
	std::wstring						sMsg;				  // Debugger message.
#endif
	// -------------------------------
	// Create an unnamed job object.
	//

    hJob = ::CreateJobObject( NULL, NULL );
	if ( hJob )
	{
		// ------------------------------------------
		// Allow CreateProcess with CREATE_BREAKAWAY_FROM_JOB flag
		//
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli;
		ZeroMemory( &jeli, sizeof(jeli) );
		jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
		SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));

		// ------------------------------
		// Create an IO Completion Port.
		//

        hIOCP = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
		if ( hIOCP )
		{
			// Set up all the necessary information required to associate the job to the IO completion port.
			portID.CompletionKey  = ( PVOID ) 0; // We don't use a completion key as we only have 1 job.
			portID.CompletionPort = hIOCP;

			// ------------------------------------------
			// Assign the IO completion port to the job.
			//
// Adding this breaks AssignProcessToJobObject on XP and Vista with UAC enabled. Doesn't effect Vista
//			Sleep(1000);
			if ( ::SetInformationJobObject( hJob, JobObjectAssociateCompletionPortInformation, &portID, sizeof( portID ) ) )
			{
				// -------------------------------------
				// Assign the process group to the job.
				//
				if ( ::AssignProcessToJobObject( hJob, hProcessGroup ) )
				{
					// --------------------------------------------------------------------------------
					// Wait for a notification that indicates the job object has zero active processes.
					//

					bool bSignaledToLeave = false;

					// Add a global event for installers to use to signal completion in case they 
					// launch other processes and wouldn't otherwise terminate when expected
					CStringA sEventID = "EACore_GameInstallComplete";
					HANDLE m_hEarlyCompletionEvent = CreateEventA(NULL, FALSE, FALSE, sEventID);
					if (!m_hEarlyCompletionEvent)
						m_hEarlyCompletionEvent = ::OpenEventA(SYNCHRONIZE, FALSE, sEventID);
					ASSERT(m_hEarlyCompletionEvent);
					HANDLE hObjects[2];
					hObjects[0] = ghExitEvent;
					hObjects[1] = m_hEarlyCompletionEvent;

					while ( bQueueExists && ( nJobStatus != JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO ) )
					{
						// Wait for either proxy process & child processes to complete or the global event to be set
						DWORD dwResult = ::WaitForMultipleObjects(2, hObjects, false, 0);
						if (dwResult == WAIT_OBJECT_0 || dwResult == WAIT_OBJECT_0+1)
						{
							bSignaledToLeave = (dwResult == WAIT_OBJECT_0+1);
							break;
						}

						if ( ::GetQueuedCompletionStatus( hIOCP, &nJobStatus, &nCompletionKey, &pOverlapped, 50 ) == FALSE )
						{
							bQueueExists = (GetLastError() == WAIT_TIMEOUT);
						}
					} // while ( nQueueStatus...

					if (m_hEarlyCompletionEvent)
						CloseHandle(m_hEarlyCompletionEvent);

					bRC = bSignaledToLeave || ( nJobStatus == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO );

					// ----------------------------------------------------------------------------------
					// Display a status message as to whether or not the above operation was successful.
					//
					if ( bRC ) 
					{ 
#ifdef _DEBUG
						if ( bSignaledToLeave )
						{
							sMsg = ID + L"Synchronization Complete. Signaled to leave.";
						}
					}
					else	   
					{ 
						sMsg = ID + L"Error: The IO completion port's queue was unexpectedly destroyed."; 
					} // if ( bRC )
				}
				else
				{
					sMsg= ID + L"Error: Could not assign the process group root to the job.";
				} // if ( ::AssignProcessToJobObject...
			}
			else
			{
				sMsg = ID + L"Error: Could not assign the IO completion port to the job.";
			} // if ( SetInformationJobObject...
		}
		else
		{
			sMsg = ID + L"Error: Could not create an IO completion port.";
		} // if ( hIOCP...
	}
	else
	{
		sMsg = ID + L"Error: Could not create a job.";
	} // if ( hJob )

	// Output any messages.
	if (!bRC && !sMsg.empty())
	{
		DWORD dwLastError = GetLastError();
		sMsg += L" (Error Code ";
		sMsg += wchar_t(dwLastError);
		sMsg += L")";

		::OutputDebugString( sMsg.c_str() );
		::MessageBox( NULL, sMsg.c_str(), L"Error", MB_OK );		
	}
#else
					}
				}
			}
		}
	}
#endif

	// Cleanup.
	if ( hIOCP ) { ::CloseHandle( hIOCP ); }
	if ( hJob  ) { ::CloseHandle( hJob  ); }

	return bRC;
}
