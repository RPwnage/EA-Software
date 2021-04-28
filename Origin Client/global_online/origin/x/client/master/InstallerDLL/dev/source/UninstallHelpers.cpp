// telemetry
#include "TelemetryAPIDLL.h"
#include "UninstallHelpers.h"

#define MAX_PATH_LEN 256
#define DEFAULT_DIR_SEPARATOR L"\\"


inline std::wstring GetUsersDirectory (const std::wstring& str)
{
	const size_t found = str.find_last_of('\\');
	return (str.substr(0,found));
}

void CleanUserDataFolders()
{
	/*MessageBox(NULL,L"CleanUserDataFolders",L"CleanUserDataFolders called ",0);*/
	TCHAR   appData[MAX_PATH_LEN];
	SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, appData ); 

	const std::wstring userDataPath(appData);
	std::vector<CString> userList;

	std::wstring usersDir = GetUsersDirectory(userDataPath);
	usersDir.append(DEFAULT_DIR_SEPARATOR);

	ListWindowsUsers(usersDir.c_str(),userList);

	TCHAR   appData2[MAX_PATH_LEN];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appData2 ); 
	const std::wstring userDataPath2(appData2);

	std::vector<CString> configList;
	FindEbisuConfigFiles(userDataPath2.c_str(),configList);

	std::wstring userDataPath3(userDataPath2);
	userDataPath3.erase(0,userDataPath.size());
	static const std::wstring EADM_PATH = L"\\Electronic Arts\\Origin";

	for(unsigned int i = 0; i < userList.size(); ++i)
	{
		const std::wstring userName(userList.at(i).GetString());
		const std::wstring dir = usersDir + userName + userDataPath3 + EADM_PATH;
		DeleteDirectory(dir.c_str());
	}

	TCHAR commonAppData[MAX_PATH_LEN];
	SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, commonAppData);
	const std::wstring commonDataPath(commonAppData);
	const std::wstring dir = commonDataPath + EADM_PATH;
	DeleteDirectory(dir.c_str());
}

void FindEbisuConfigFiles(
						  LPCTSTR dirName,
						  std::vector< CString > & filepaths )
{
	// Check input parameters
	ASSERT( dirName != NULL );

	// Clear filename list
	filepaths.clear();

	// Object to enumerate files
	CFileFind finder;

	// Build a string using wildcards *.*,
	// to enumerate content of a directory
	CString wildcard( dirName );
	wildcard += _T("\\Settings.xml");

	// Init the file finding job
	BOOL working = finder.FindFile( wildcard );

	// For each file that is found:
	while ( working )
	{
		// Update finder status with new file
		working = finder.FindNextFile();

		// Skip '.' and '..'
		if ( finder.IsDots() )
		{
			continue;
		}

		if(finder.IsDirectory() && !finder.IsDots())
		{
			const CString strRootPath = finder.GetFilePath();
			FindEbisuConfigFiles(strRootPath, filepaths);
		}

		// Add file path to container
		filepaths.push_back( finder.GetFilePath());
	}

	// Cleanup file finder
	finder.Close();
}




bool DeleteDirectory(LPCTSTR lpszDir, const bool noRecycleBin /*= true*/)
{
	const size_t len = _tcslen(lpszDir);
	TCHAR *pszFrom = new TCHAR[len+2];
	_tcscpy(pszFrom, lpszDir);
	pszFrom[len] = 0;
	pszFrom[len+1] = 0;

	SHFILEOPSTRUCT fileop;
	fileop.hwnd   = NULL;    // no status display
	fileop.wFunc  = FO_DELETE;  // delete operation
	fileop.pFrom  = pszFrom;  // source file name as double null terminated string
	fileop.pTo    = NULL;    // no destination needed
	fileop.fFlags = FOF_NOCONFIRMATION|FOF_SILENT;  // do not prompt the user

	if(!noRecycleBin)
		fileop.fFlags |= FOF_ALLOWUNDO;

	fileop.fAnyOperationsAborted = FALSE;
	fileop.lpszProgressTitle     = NULL;
	fileop.hNameMappings         = NULL;

	const int ret = SHFileOperation(&fileop);
	delete [] pszFrom;  
	return (ret == 0);
}

void ListWindowsUsers(
					  LPCTSTR dirName,
					  std::vector< CString > & filepaths )
{
	// Check input parameters
	ASSERT( dirName != NULL );

	// Clear filename list
	filepaths.clear();

	// Object to enumerate files
	CFileFind finder;

	// Build a string using wildcards *.*,
	// to enumerate content of a directory
	CString wildcard( dirName );
	wildcard += _T("\\*.*");

	// Init the file finding job
	BOOL working = finder.FindFile( wildcard );

	// For each file that is found:
	while ( working )
	{
		// Update finder status with new file
		working = finder.FindNextFile();

		if ( finder.IsDots() )
		{
			continue;
		}

		// Skip sub-directories
		if ( finder.IsDirectory() )
		{
			// Add file path to container
			filepaths.push_back( finder.GetFileName());
		}
	}

	// Cleanup file finder
	finder.Close();
}
