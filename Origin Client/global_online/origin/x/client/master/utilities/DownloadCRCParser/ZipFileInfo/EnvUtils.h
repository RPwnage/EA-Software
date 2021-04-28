///////////////////////////////////////////////////////////////////////////////
// EnvUtils.cpp
// Environmental Utilities
// Copyright (c) 2010-2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef _ENVUTILS_H_INCLUDED_
#define _ENVUTILS_H_INCLUDED_

#include <AclAPI.h>
#include <QString>

namespace EnvUtilsStrings
{
	const QString COMPANY_NAME		= "Electronic Arts";
	const QString PRODUCT_NAME		= "EA Core";
	const QString CACHE_FOLDER_NAME	= "cache";
}

typedef DWORD (WINAPI *fdGetNamedSecurityInfo)(
	LPTSTR pObjectName,
	SE_OBJECT_TYPE ObjectType,
	SECURITY_INFORMATION SecurityInfo,
	PSID* ppsidOwner,
	PSID* ppsidGroup,
	PACL* ppDacl,
	PACL* ppSacl,
	PSECURITY_DESCRIPTOR* ppSecurityDescriptor);
typedef VOID (WINAPI *fdBuildExplicitAccessWithName)(
	PEXPLICIT_ACCESS pExplicitAccess,
	LPTSTR pTrusteeName,
	DWORD AccessPermissions,
	ACCESS_MODE AccessMode,
	DWORD Inheritance);
typedef DWORD (WINAPI *fdSetEntriesInAcl)(
	ULONG cCountOfExplicitEntries,
	PEXPLICIT_ACCESS pListOfExplicitEntries,
	PACL OldAcl,
	PACL* NewAcl);
typedef DWORD (WINAPI *fdSetNamedSecurityInfo)(
	LPTSTR pObjectName,
	SE_OBJECT_TYPE ObjectType,
	SECURITY_INFORMATION SecurityInfo,
	PSID psidOwner,
	PSID psidGroup,
	PACL pDacl,
	PACL pSacl);

class EnvUtils
{
protected:
	// Want the library handle closed when THE static is released
	class S_AutoCloseLibHandle {
	public:
		HMODULE hLib;
	public:
		S_AutoCloseLibHandle() : hLib(NULL) {}
		~S_AutoCloseLibHandle() { Close(); }

		bool	Open(const wchar_t* sLibFilePath) { Close(); hLib = ::LoadLibraryW(sLibFilePath); return (hLib != NULL); }
		void	Close() { if (hLib) { FreeLibrary(hLib); hLib = NULL; } }
		HMODULE	GetHandle() const { return hLib; }
	};

public:
	static QString	QualifyToModulePath(const wchar_t* sFileName);
	static QString	ConvertToShortPath(const QString& sPath);
	static QString	ConvertToUnicodePath(const QString& sFilePath);
	static bool		DeleteFileIfPresent  ( QString sFileName );
	static bool		DeleteFolderIfPresent( const QString& sFolderName, bool bSafety = false, bool bIncludingRoot = false, void *pProgressBar = NULL, void (*pfcnCallback)(void*) = NULL );
	static int		CountFiles           ( const QString& sFolderName );
	static QString	GetOS				( bool bIncludeServicePackInfo = false );						 // Retrieve a string representation of the Operating System.
	static QString  GetOSVersion		();	// Format "major.minor"   more info on versions available here: http://msdn.microsoft.com/en-us/library/ms724833(v=vs.85).aspx
	static QString	GetServicePack		( );
	static void		DoEvents				( void );						 // Pop Win32 messages off of the queue and process them.
	static bool		HasUAC();
	static QString  GetDecodedCommandLine();
	static bool		GrantEveryoneAccessToFile(const QString& sFileName);
	static bool		CreateEmptyUnicodeFile(const QString& sFilePath);
};

//#define max(a,b) (a>=b)?a:b

#endif

