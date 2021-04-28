#include "stdafx.h"
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

void CleanUserDataFolders();

void ListWindowsUsers(
					  LPCTSTR dirName,
					  std::vector< CString > & filepaths );

void FindEbisuConfigFiles(
						  LPCTSTR dirName,
						  std::vector< CString > & filepaths );

bool DeleteDirectory(LPCTSTR lpszDir, const bool noRecycleBin = true);

