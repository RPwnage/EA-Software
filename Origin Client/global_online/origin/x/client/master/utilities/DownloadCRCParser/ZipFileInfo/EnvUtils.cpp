///////////////////////////////////////////////////////////////////////////////
// EnvUtils.cpp
//
// Copyright (c) 2010-2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include "EnvUtils.h"
//#include "GameDRM.h" // used to get EAD Registry key locations...
#include <shlwapi.h>
#include "StringHelpers.h"
//#include "WideCharUtils.h"
//#include "Win32Registry.h"
//#include "Timer.h"
//#include "LastError.h"

#ifdef EADM
#include "Logger.h"
#endif

#include <QMutex>
#include <QMutexLocker>

//
// Create a fully qualified path for safe use with LoadLibrary where a bug in XP causes
// a failure even when the target file is sitting right next to the module.
//
QString EnvUtils::QualifyToModulePath(const wchar_t* sFileName)
{
	if (!sFileName)
		return QString();

	// Figure out the current directory and prepend to create the full dll path
	static wchar_t wcModulePath[MAX_PATH] = {0};
	if (!wcModulePath[0])
	{
		GetModuleFileName(NULL, wcModulePath, MAX_PATH);

		// we just want the folder w/o the file
		wchar_t* sPtr = max(wcsrchr(wcModulePath, L'/'), wcsrchr(wcModulePath, L'\\'));
		sPtr[0] = NULL;

		// Replace / with \ for consistency sake
		while (true)
		{
			sPtr = wcsrchr(wcModulePath, L'/');
			if (!sPtr) break;
			sPtr[0] = L'\\';
		}
	}

	QString sFullDllPath = QString("%1\\%2").arg(QString::fromWCharArray(wcModulePath)).arg(QString::fromWCharArray(sFileName));

	return sFullDllPath;
}


QString EnvUtils::ConvertToShortPath(const QString& sPath)
{
#ifdef _WIN32
	wchar_t* sPath_Short = new wchar_t[MAX_PATH];
	memset(sPath_Short, 0, sizeof(wchar_t) * MAX_PATH);
    ::GetShortPathName((LPCWSTR)sPath.utf16(), sPath_Short, MAX_PATH);
	QString shortPath = QString::fromWCharArray(sPath_Short);
	delete[] sPath_Short;

	return shortPath;
#else
	return sPath;
#endif
}

// Returns true if the specified file was deleted
bool EnvUtils::DeleteFileIfPresent( QString sFileName )
{
	if (sFileName.isEmpty())
		return false;

	sFileName = ConvertToUnicodePath(sFileName);

    if (::PathFileExists((LPCWSTR)sFileName.utf16()))
	{
        if (::DeleteFile((LPCWSTR)sFileName.utf16()) != TRUE)
		{
#ifdef _DEBUG
			DWORD nError = ::GetLastError();
			QString sError;
			sError = QString("Failed to delete \"%1\"! error code [%2]\r\n").arg(sFileName).arg(nError);
			//sError += LastError::GetLastErrorString(nError);
			//EBILOGERROR << L"DeleteFileIfPresent ERROR = " << sError;
#endif
			return false;
		}
		else
		{
#ifdef _DEBUG
			//EBILOGEVENT << QString("Deleted [%1]").arg(sFileName);
#endif
			return true;
		}
	}
	else
	{
#ifdef _DEBUG
		//EBILOGERROR << QString("bool EnvUtils::DeleteFileIfPresent( QString sFileName ) File [%1] does not exist.").arg(sFileName);
#endif
		return false;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// EnvUtils::DeleteFolderIfPresent
//
// delete folder with contents
//
// Note: Used recursively for sub-folders
//		 Shell function SHFileOperation moves files to recycle bin so not used here
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
bool EnvUtils::DeleteFolderIfPresent( const QString& sFolderName, bool bSafety /* = false */, bool bIncludingRoot /* = false */, void *pProgressBar /* = NULL */, void (*pfcnCallback)(void*) /* = NULL */ )
{
	bool bRetVal = true;

	// Make working copy of folder name
	QString sWorkingFolderName = sFolderName;

	// Path and search construction constants.
	const QString ALL_FILES_AND_FOLDERS  = "*.*"; // An implementation of search criteria.
	const char    SEPARATOR              = '\\';
	const QString THIS_DIRECTORY         = ".";
	const QString PARENT_DIRECTORY       = "..";
	const QString UNICODE_PREFIX         = "\\\\?\\";
	const long    UNICODE_MAX_PATH		 = 32767; // Maximum path size (in characters).
	static const size_t NPOS             = -1;    // NULL position constant for any index of a basic_string derivative.

	// Debug / Exception constants.
	const QString ID                               = "EnvUtils::DeleteFolderIfPresent ERROR: ";
	const QString ERROR_PATH_DOES_NOT_EXIST		   = "Path does not exist: ";
	const QString ERROR_PATH_EMPTY                 = "Path is empty.";
	const QString ERROR_PATH_NOT_SAFE              = "Path is out of safety range: ";
	const QString ERROR_CANNOT_MAKE_PATH_DELETABLE = "Cannot make path deletable: ";
	const QString ERROR_CANNOT_DELETE_FILE         = "Cannot delete file: ";
	const QString ERROR_CANNOT_DELETE_DIRECTORY    = "Cannot delete directory: ";

	// Locals.
	int               nFirstSeparatorIndex;			 // Index of the first SEPARATOR in TargetPath.
	int               nLastSeparatorIndex;			 // Index of the last SEPARATOR in TargetPath.
	int               nSeparatorStartIndex = 0;		 // Index of where to begin scanning for SEPARATORs in TargetPath.
	int               nFirstLastSeparatorDifference; // nLastSeparator - nFirstSeparator
	WIN32_FIND_DATAW  oFindData;					 // Data structure containing file / folder data.
	HANDLE            hSearch;						 // File system search handle.
	QString			  sCriteria;					 // Criteria used to search for files / folder.
	QString           sName;                         // The name of a file system node.
	QString           sCurrentPath;					 // 'TargetPath' + 'name'
	QString           sDebugMsg;					 // Debug Message
	bool              bIsFile;						 // Is the current file system node a file?
	bool              bIsSubDirectory;				 // Is the current file system node a child folder?
	bool              bSearchResultsExist;			 // Do file search results exist?
	bool              bSafetyCheckPassed = true;	 // Defaulted to true.

	// Only process path's with values.
	if ( !sWorkingFolderName.isEmpty() )
	{     
		// Grab the difference in position between the first and last path separation characters taking into account any existing UNICODE_PREFIX.
		if ( sWorkingFolderName.compare( UNICODE_PREFIX ) == 0 ) { nSeparatorStartIndex = UNICODE_PREFIX.length(); }
		nFirstSeparatorIndex = sWorkingFolderName.indexOf( SEPARATOR, nSeparatorStartIndex );
		nLastSeparatorIndex  = sWorkingFolderName.lastIndexOf( SEPARATOR );
		nFirstLastSeparatorDifference = ( ( nLastSeparatorIndex  != -1 ) ? nLastSeparatorIndex  : 0 ) - 
										( ( nFirstSeparatorIndex != -1 ) ? nFirstSeparatorIndex : 0 );

		// Safety check. This operates by examining the distance between the first and last non-UNICODE_PREFIX separators. If there is not at least
		// one character in between then we know the path is a root (ex. c:\\, d:\\, \\, etc.)
		if ( bSafety && ( nFirstLastSeparatorDifference <= 1 ) )
		{ 
			bRetVal = false;
			bSafetyCheckPassed = false; // Flag a failure in the safety check.

#ifdef _DEBUG
			sDebugMsg = ID + ERROR_PATH_NOT_SAFE + sWorkingFolderName + "\r\n";
            ::OutputDebugString( (LPCWSTR)sDebugMsg.utf16() );
#endif
		}  // if ( Safety...

		// Did the safety check pass?
		if ( bSafetyCheckPassed )
		{
			// Trim an existing trailing backslash.
			if ( ( sWorkingFolderName.at(sWorkingFolderName.length() - 1) == SEPARATOR ) && ( nFirstLastSeparatorDifference > 1 ) )
				sWorkingFolderName.remove( sWorkingFolderName.length() - 1, sWorkingFolderName.length() ); 

			// Pre-pend the Unicode prefix if it's not there.
			//                  if ( sWorkingFolderName.Compare( UNICODE_PREFIX ) != 0 )
			//					  sWorkingFolderName.Insert( 0, UNICODE_PREFIX );
			// Setup the criteria and perform a search for the first file system node.
			sCriteria           = sWorkingFolderName + SEPARATOR + ALL_FILES_AND_FOLDERS;
            hSearch             = ::FindFirstFile( (LPCWSTR)sCriteria.utf16(), &oFindData ); // find the first file
			bSearchResultsExist = ( hSearch != INVALID_HANDLE_VALUE );

			// Do search results exist?
			if ( bSearchResultsExist )
			{
				// Enumerate the search results.
				while ( bSearchResultsExist )
				{
					// Build the current path.
					sName        = QString::fromWCharArray(oFindData.cFileName); 
					sCurrentPath = ConvertToUnicodePath(sWorkingFolderName + SEPARATOR + sName); 

					// Determine if the file system node is a file or child folder ("." and ".." folders are ignored).
					bIsFile                  = !( oFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ? true : false;
					bIsSubDirectory    = ( !bIsFile && ( ( sName != THIS_DIRECTORY ) && ( sName != PARENT_DIRECTORY ) ) ) ? true : false; 

					// If we have a file or child folder then process it.
					if ( bIsFile || bIsSubDirectory ) 
					{
						// Make it deletable.
                        if ( ::SetFileAttributesW( (LPCWSTR)sCurrentPath.utf16(), FILE_ATTRIBUTE_NORMAL ) )
						{
							// Are we dealing with a file?
							if ( bIsFile ) 
							{ 
								// Yes, delete it.
                                if ( !::DeleteFile( (LPCWSTR)sCurrentPath.utf16() ) )
								{
									bRetVal = false;
#ifdef _DEBUG
									sDebugMsg = ID + ERROR_CANNOT_DELETE_FILE + sCurrentPath + "\r\n";
                                    ::OutputDebugString( (LPCWSTR)sDebugMsg.utf16() );
#endif
								} // if ( !::DeleteFileW...
								else
								{
									// If a progress bar and a progress bar callback was passed in, call it
									if (pProgressBar && pfcnCallback)
										pfcnCallback(pProgressBar);
								}
							} 
							else // It's a isSubDirectory
							{ 
								DeleteFolderIfPresent( sCurrentPath, false, false, pProgressBar, pfcnCallback ); // Recurse.
								// Delete the subdirectory.
                                if ( !::RemoveDirectory( (LPCWSTR)sCurrentPath.utf16() ) )
								{     
									bRetVal = false;
#ifdef _DEBUG
									sDebugMsg = ID + ERROR_CANNOT_DELETE_DIRECTORY + sCurrentPath + "\r\n";
                                    ::OutputDebugString((LPCWSTR)sDebugMsg.utf16());
#endif
								} // if ( !::RemoveDirectoryW...
							} // if ( isFile )
						}
						else
						{
							bRetVal = false;
#ifdef _DEBUG
							sDebugMsg = ID + ERROR_CANNOT_MAKE_PATH_DELETABLE + sCurrentPath + "\r\n";
                            ::OutputDebugString( (LPCWSTR)sDebugMsg.utf16());
#endif
						} // if ( ::SetFileAttributesW...
					} // if  ( isFile || isSubDirectory...
					bSearchResultsExist = ::FindNextFile( hSearch, &oFindData ) ? true : false;
				} // while ( searchResultsExist )...

				if ( hSearch != INVALID_HANDLE_VALUE ) ::FindClose ( hSearch ); // Cleanup 

				// Delete the root?
				if ( bIncludingRoot )
				{
                    if ( ::SetFileAttributes( (LPCWSTR)sWorkingFolderName.utf16(), FILE_ATTRIBUTE_NORMAL ) )
					{
						// Delete the root directory.
                        if ( !::RemoveDirectory( (LPCWSTR)sWorkingFolderName.utf16()) )
						{     
							bRetVal = false;

#ifdef _DEBUG
							sDebugMsg = ID + ERROR_CANNOT_DELETE_DIRECTORY + sWorkingFolderName + "\r\n";
                            ::OutputDebugString( (LPCWSTR)sDebugMsg.utf16() );
#endif
						} // if ( !::RemoveDirectoryW...
					}
					else
					{
						bRetVal = false;
#ifdef _DEBUG
						sDebugMsg = ID + ERROR_CANNOT_MAKE_PATH_DELETABLE + sWorkingFolderName + "\r\n";
                        ::OutputDebugString( (LPCWSTR)sDebugMsg.utf16() );
#endif
					} // if ( ::SetFileAttributesW...
				} // if ( IncludingRoot )...
			}
			else
			{
				bRetVal = false;

#ifdef _DEBUG
				sDebugMsg = ID + ERROR_PATH_DOES_NOT_EXIST + sWorkingFolderName + "\r\n";
                ::OutputDebugString( (LPCWSTR)sDebugMsg.utf16() );
#endif
			} // if ( searchResultsExist )
		} // if ( safetyCheckPassed )
	}
	else
	{
		bRetVal = false;
#ifdef _DEBUG
		sDebugMsg = ID + ERROR_PATH_EMPTY + "\r\n";
        ::OutputDebugString( (LPCWSTR)sDebugMsg.utf16() );
#endif
	} // if ( !sWorkingFolderName.Empty() )
	return bRetVal;
}

// This function returns a count of the files inside sFolderName and it's subfolders
// The code was taken from the EnvUtils::DeleteFolderIfPresent() and then optimized for speed.
// All error reporting has been removed since it doesn't really matter here.
int EnvUtils::CountFiles( const QString& sFolderName )
{
	int nFileCount = 0;

	// Locals.
	WIN32_FIND_DATAW  oFindData;					 // Data structure containing file / folder data.
	HANDLE            hSearch;						 // File system search handle.
	bool              bSearchResultsExist;			 // Do file search results exist?

	// Only process path's with values.
	if ( sFolderName.isEmpty() )
		return 0;

	QString sCriteria   = sFolderName + "\\*.*";
    hSearch             = ::FindFirstFile( (LPCWSTR)sCriteria.utf16(), &oFindData ); // find the first file
	bSearchResultsExist = ( hSearch != INVALID_HANDLE_VALUE );

	// Enumerate the search results.
	while ( bSearchResultsExist )
	{

		// If this is a subdirectory
		if (oFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// See if it's a root directory
			if (oFindData.cFileName[0] != '.')
			{
				nFileCount += CountFiles( sFolderName + "\\" + QString::fromWCharArray(oFindData.cFileName) ); // Recurse.
			}
		}
		else // else it must be a file, so count it
			nFileCount++;


		bSearchResultsExist = ::FindNextFile( hSearch, &oFindData ) ? true : false;
	} // while ( searchResultsExist )...

	if ( hSearch != INVALID_HANDLE_VALUE )
		::FindClose ( hSearch ); // Cleanup 

	return nFileCount;
}

/* ===========================================================================================
    Purpose: Returns a string representing a short name of the present Operating System.

	Returns: o "vista", "longhorn", "2003", "2003r2", "xp", "xp64", "2000", "nt", "me", "98", 
				or "95"
			 o "unknown" if the operating system could not be identified.
   =========================================================================================== */
QString EnvUtils::GetOS( bool bIncludeServicePackInfo )
{
	// Already defined for _WIN32_WINNT >= 0x0501
#ifndef SM_SERVERR2
	const int SM_SERVERR2 = 89; // Special GetSystemMetrics() index used to retrieve the OS build number if the
#endif
	// system is Windows Server 2003 R2 (the build number is returned as 0 otherwise).

	// Major version numbers of the Windows Operating Systems.
	const int NMAJOR_WIN7				 = 6;
	const int NMAJOR_VISTA_LONGHORN		 = 6;
	const int NMAJOR_2003R2_2003_XP_2000 = 5;
	const int NMAJOR_NT					 = 4;
	const int NMAJOR_95_98_ME			 = 4;

	// Minor version numbers of the Windows Operating Systems.
	const int NMINOR_VISTA_LONGHORN_2000_NT = 0;
	const int NMINOR_2003R2_2003_XP64	    = 2;
	const int NMINOR_XP					    = 1;
	const int NMINOR_WIN7					= 1;
	const int NMINOR_ME					    = 90;
	const int NMINOR_98					    = 10;
	const int NMINOR_95					    = 0;

	// All return values identifying *this* Operating System.
	const QString SOS_UNKNOWN  = "unknown";
	const QString SOS_VISTA	   = "vista";
	const QString SOS_LONGHORN = "longhorn";
	const QString SOS_2003	   = "2003";
	const QString SOS_2003R2   = "2003r2";
	const QString SOS_XP	   = "xp";
	const QString SOS_XP64	   = "xp64";
	const QString SOS_2000	   = "2000";
	const QString SOS_NT	   = "nt";
	const QString SOS_ME	   = "me";
	const QString SOS_98	   = "98";
	const QString SOS_95	   = "95";
	const QString SOS_WIN7	   = "win7";

	OSVERSIONINFOEX	osvi;
	SYSTEM_INFO		si;
	QString			sOS = SOS_UNKNOWN; // Default return value.


	// ----------------------------------------
	// Prepare the structs for data population
	//

	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
	ZeroMemory( &si, sizeof( SYSTEM_INFO ) );
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );


	// ---------------------------------------------------
	// Determine what *this* specific Operating System is
	//

	if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
	{
		::GetSystemInfo( &si ); // Has no return value and we assume it always works.

		// Is this OS and NT or 9x platform?
		switch ( osvi.dwPlatformId )
		{

			case VER_PLATFORM_WIN32_NT:
			{
				if ( osvi.dwMajorVersion == NMAJOR_WIN7 && osvi.dwMinorVersion == NMINOR_WIN7 ) 
				{	
					sOS = SOS_WIN7;
				} 
				else if ( osvi.dwMajorVersion == NMAJOR_VISTA_LONGHORN && osvi.dwMinorVersion == NMINOR_VISTA_LONGHORN_2000_NT ) 
				{	
					sOS = ( osvi.wProductType == VER_NT_WORKSTATION ) ? SOS_VISTA : SOS_LONGHORN;
				} 
				else if ( osvi.dwMajorVersion == NMAJOR_2003R2_2003_XP_2000 && osvi.dwMinorVersion == NMINOR_2003R2_2003_XP64 )
				{
					if( ::GetSystemMetrics( SM_SERVERR2 ) ) { sOS = SOS_2003R2; }
					else if( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) { sOS = SOS_XP64; }
					else { sOS = SOS_2003; }
				} 
				else if ( osvi.dwMajorVersion == NMAJOR_2003R2_2003_XP_2000 && osvi.dwMinorVersion == NMINOR_XP ) { sOS = SOS_XP; }
				else if ( osvi.dwMajorVersion == NMAJOR_2003R2_2003_XP_2000 && osvi.dwMinorVersion == NMINOR_VISTA_LONGHORN_2000_NT ) { sOS = SOS_2000; }
				else if ( osvi.dwMajorVersion <= NMAJOR_NT )	{ sOS = SOS_NT; }
			} // case VER_PLATFORM_WIN32_NT:
			
			case VER_PLATFORM_WIN32_WINDOWS:
			{
				if		( osvi.dwMajorVersion == NMAJOR_95_98_ME && osvi.dwMinorVersion == NMINOR_95 ) { sOS = SOS_95; }
				else if ( osvi.dwMajorVersion == NMAJOR_95_98_ME && osvi.dwMinorVersion == NMINOR_98 ) { sOS = SOS_98; }
				else if ( osvi.dwMajorVersion == NMAJOR_95_98_ME && osvi.dwMinorVersion == NMINOR_ME ) { sOS = SOS_ME; }
			} // case VER_PLATFORM_WIN32_WINDOWS:

		} // switch ( osvi.dwPlatformId )...

		if (bIncludeServicePackInfo)
		{
			sOS += " " + QString::fromWCharArray(osvi.szCSDVersion);
		}

	} // if ( ::GetVersionEx...


	return sOS;
}

QString EnvUtils::GetOSVersion()
{
	// format major.minor
	OSVERSIONINFOEX	osvi;
	QString sVersion("0.0");

	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

	if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
	{
		sVersion = QString("%1.%2").arg(osvi.dwMajorVersion, osvi.dwMinorVersion);
	}

	return sVersion;
}


QString	EnvUtils::GetServicePack()
{
	OSVERSIONINFOEX	osvi;
	SYSTEM_INFO		si;

	// Prepare the structs for data population
	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );
	ZeroMemory( &si, sizeof( SYSTEM_INFO ) );
	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

	if ( ::GetVersionEx( ( OSVERSIONINFO * ) &osvi ) )
	{
		return QString::fromWCharArray(osvi.szCSDVersion);
	}
	return QString("");
}

bool EnvUtils::HasUAC()
{
	DWORD dwUACEnabled = 0;
	DWORD regValueType = REG_DWORD;
	DWORD dwBufferSize = sizeof(DWORD);
	return (SHGetValue(
			HKEY_LOCAL_MACHINE,
			L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
			L"EnableLUA",
			&regValueType,
			&dwUACEnabled,
			&dwBufferSize) == ERROR_SUCCESS
				&& (dwUACEnabled != 0));
}


void EnvUtils::DoEvents()
{
	for ( MSG msg; ::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ); )
	{
		::TranslateMessage( &msg );
		::DispatchMessage ( &msg );
		::Sleep			  ( 0	 );
	} // for ( MSG msg...
}

QString EnvUtils::GetDecodedCommandLine()
{
#ifdef UNICODE
	QString strCommandLine = QString::fromWCharArray(GetCommandLine());
#else
	QString strCommandLine = QString::fromAscii(GetCommandLine());
#endif

#ifdef _DEBUG
	// The shell is interpreting the '&' character as a break in the line to string together commands
	// instead of encoding it.  
	// in DEBUG MODE ONLY to get around the lack of encoding, we are going to change any real "&" into 
	// the correct encoding of %26, and use the string "%amp;" as the EADM argument delimiter. 
	// e.g. 	-eadcommand:?cmd=app_auth_game&TitleID=eagames%2fbf2142%2fonline%5fcontent%2fbf2142%5fna&LauncherPath=Z%3a%5cProgram%20Files%5cElectronic%20%26%20Arts%5cBattlefield%202142%5cBF2142Launcher%2eexe&LicenseLocation=C%3a%5cDocuments%20and%20Settings%5cAll%20Users%5cApplication%20Data%5cPACE%20Anti%2dPiracy%5cLicense%20Files%5ceagames%2dbf2142%2donline%5fcontent%2dbf2142%5fna%2eilf&DRMProcessName=BF2142%2eexe&DRMProcessName=BF2142Pace%2eexe&LicenseSecondsLeft=-1&cmdArgs=%2bmenu%201%20%2bfullscreen%200%20 -silent
	// should be entered in the command shell as
	// -eadcommand:?cmd=app_auth_game&amp;TitleID=eagames%2fbf2142%2fonline%5fcontent%2fbf2142%5fna&amp;LauncherPath=Z%3a%5cProgram%20Files%5cElectronic%20%26%20Arts%5cBattlefield%202142%5cBF2142Launcher%2eexe&amp;LicenseLocation=C%3a%5cDocuments%20and%20Settings%5cAll%20Users%5cApplication%20Data%5cPACE%20Anti%2dPiracy%5cLicense%20Files%5ceagames%2dbf2142%2donline%5fcontent%2dbf2142%5fna%2eilf&amp;DRMProcessName=BF2142%2eexe&amp;DRMProcessName=BF2142Pace%2eexe&amp;LicenseSecondsLeft=-1&amp;cmdArgs=%2bmenu%201%20%2bfullscreen%200%20 -silent
	strCommandLine.replace("%amp;", "&");
#endif
	return strCommandLine;
}

// NOTE: Routine is used in Sony SecuROM DRM program PAUL.dll where it needs to support running under Cider on MAC OSX
//       so the security APIs are manually bound here instead of statically so that this routine doesn't create a
//       dependancy on an unsupported DLL under Cider.
bool EnvUtils::GrantEveryoneAccessToFile(const QString& sFileName)
{
	// Block until binding is completed for first caller
	// Also blocking the whole call while we're at it because the performance would only be worse if split between calls.
	static QMutex csBinding;
	QMutexLocker oAutoLock(&csBinding);

	//
	// Cache API binding to avoid killing performance of this already slow routine.

	static	S_AutoCloseLibHandle s_oAutoLib;
	static	fdGetNamedSecurityInfo s_fpGetNamedSecurityInfo;
	static	fdBuildExplicitAccessWithName s_fpBuildExplicitAccessWithName;
	static	fdSetEntriesInAcl s_fpSetEntriesInAcl;
	static	fdSetNamedSecurityInfo s_fpSetNamedSecurityInfo;
	static	bool s_bAPIBindingAttempted = false,
				 s_bAPIBindingFailed = false;
	if (!s_bAPIBindingAttempted)
	{
		// Tried and failed?
		if (s_bAPIBindingFailed)
			return false;
		
		// Bind library
		if (!s_oAutoLib.Open(L"AdvAPI32.DLL"))
		{
			s_bAPIBindingFailed = true;
			return false;
		}

		// Bind APIs
		s_fpGetNamedSecurityInfo = (fdGetNamedSecurityInfo)GetProcAddress(s_oAutoLib.GetHandle(), "GetNamedSecurityInfoW");
		s_fpBuildExplicitAccessWithName = (fdBuildExplicitAccessWithName)GetProcAddress(s_oAutoLib.GetHandle(), "BuildExplicitAccessWithNameW");
		s_fpSetEntriesInAcl = (fdSetEntriesInAcl)GetProcAddress(s_oAutoLib.GetHandle(), "SetEntriesInAclW");
		s_fpSetNamedSecurityInfo = (fdSetNamedSecurityInfo)GetProcAddress(s_oAutoLib.GetHandle(), "SetNamedSecurityInfoW");

		// Found what we need?
		s_bAPIBindingFailed = !(s_fpGetNamedSecurityInfo && s_fpBuildExplicitAccessWithName && 
				s_fpSetEntriesInAcl && s_fpSetNamedSecurityInfo);
		if (s_bAPIBindingFailed)
			return false;

		// Bound successfully at this point
		s_bAPIBindingAttempted = true;
	}

	PACL ExistingDacl = NULL;
	PACL NewAcl = NULL;
	PSECURITY_DESCRIPTOR psd = NULL;

	DWORD dwError;

	wchar_t* sNonConstFileName = new wchar_t[sFileName.size() +1];
	memset(sNonConstFileName, 0, (sFileName.size()+1) * sizeof(wchar_t));
	sFileName.toWCharArray(sNonConstFileName);
	dwError = s_fpGetNamedSecurityInfo( sNonConstFileName,//sNonConstFileName.GetBuffer(),
							    SE_FILE_OBJECT,
								DACL_SECURITY_INFORMATION,
								NULL,
								NULL,
								&ExistingDacl,
								NULL,
								&psd
								);

	if ( dwError !=  ERROR_SUCCESS )
	{
#ifdef EADM
		EBILOGERROR << sFileName << L": GetNamedSecurityInfo failure results = " << dwError;
#endif
		goto cleanup;
	}

	EXPLICIT_ACCESS explicitaccess;
	ZeroMemory(&explicitaccess, sizeof(EXPLICIT_ACCESS));

	s_fpBuildExplicitAccessWithName( &explicitaccess,
                                 L"Users",
								 GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL,
								 GRANT_ACCESS, 
								 (PathIsDirectory(sNonConstFileName) ? (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE) : NULL)  // XP doesn't allow these for files.
							   );

	//
	// add specified access to the object
	//
	dwError = s_fpSetEntriesInAcl( 1,
							   &explicitaccess,
							   ExistingDacl,
							   &NewAcl
							  );

	if ( dwError !=  ERROR_SUCCESS )
	{
#ifdef EADM
		EBILOGERROR << sFileName << L": SetEntriesInAcl results = " << dwError;
#endif
		goto cleanup;
	}

	memset(sNonConstFileName, 0, (sFileName.size()+1) * sizeof(wchar_t));
	sFileName.toWCharArray(sNonConstFileName);

	//
	// apply new security to file
	//
#ifdef _DEBUG
#ifdef EADM
	// Taking suspiciously long here so time these
	long long llTick = GetTick();
#endif
#endif

	dwError = s_fpSetNamedSecurityInfo( sNonConstFileName,
									SE_FILE_OBJECT, // object type
									DACL_SECURITY_INFORMATION,
									NULL,
									NULL,
									NewAcl,
									NULL
								  );
#ifdef EADM
	if (dwError != ERROR_SUCCESS)
	{
		EBILOGERROR << sFileName << L": SetNamedSecurityInfo results = " << dwError;
	}
#endif

#ifdef _DEBUG
#ifdef EADM
	EBILOGDEBUG << L"GrantEveryoneAccessToFile - SetNamedSecurityInfo() on file/folder target " << sFileName << L" call took " << GetTick()-llTick << L" ms.";
#endif
#endif

cleanup:
	if(psd != NULL) 
		LocalFree((HLOCAL) psd); 
	if(NewAcl != NULL)
		LocalFree((HLOCAL) NewAcl); 
	delete[] sNonConstFileName;

#ifdef EADM
	if (dwError != ERROR_SUCCESS)
		EBILOGERROR << L"GrantEveryoneAccessToFile() exiting with result " << dwError;
#endif

	return ( dwError !=  ERROR_SUCCESS );
}

bool EnvUtils::CreateEmptyUnicodeFile(const QString& sFilePath)
{
    HANDLE hFile = CreateFile((LPCWSTR)sFilePath.utf16(), GENERIC_ALL, FILE_SHARE_READ, NULL,
			CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwBytesWritten = 0;

	if (hFile)
	{
		// Start new file with Unicode magic and resave all preferences
		unsigned short uBOM = 0xFEFF;

		WriteFile(hFile, (void*)&uBOM, 2, &dwBytesWritten, NULL);
		CloseHandle(hFile);
	}
	
	if (!hFile || dwBytesWritten < 2)
		return false;

	return true;
}

QString EnvUtils::ConvertToUnicodePath(const QString& sFilePath)
{
	// MAX_PATH is defined as 260 in windows.  However we have a constant int definition of it in Akamai.c
	// To avoid any possible issues I'm sticking with a constant integer here.

	QString sResult;

	// If the path is >= 260 characters and doesn't yet have a unicode path prefix, add it.		
	QString sUnicodePathPrefix("\\\\?\\");
	if (sFilePath.length() >= 260 && sFilePath.left( sUnicodePathPrefix.length() ) != sUnicodePathPrefix)
		sResult = sUnicodePathPrefix + sFilePath;		
	else
		sResult = sFilePath;

	sResult.replace('/', '\\');

	return sResult;
}
