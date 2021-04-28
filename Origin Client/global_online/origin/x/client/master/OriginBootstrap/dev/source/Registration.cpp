///////////////////////////////////////////////////////////////////////////////
// Registration.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Registration.h"
#include "LogService_win.h"

#include <windows.h>
#include <sstream>
#include <string>

static std::wstring ApplicationPath()
{
	wchar_t currentExePath[MAX_PATH];
	GetModuleFileName(NULL, currentExePath, MAX_PATH);
	return std::wstring(currentExePath);
}

bool Registration::setAutoStartEnabled(bool enabled)
{
	HKEY key = NULL;
	wchar_t* keyPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	RegOpenKeyEx( HKEY_CURRENT_USER, keyPath, 0, KEY_SET_VALUE, &key );

	if (key)
	{
		if (enabled)
		{
			wchar_t launcherPath[ MAX_PATH ] = L"\0";

            // NOTE!!!!
            //the string "-AutoStart" is used to check whether the application was launched via autostart so if the command line parameter changes
            //please update wasLaunchedViaAutoStart () in main.cpp with the new string
			wsprintfW( launcherPath, L"\"%s\" -AutoStart", ApplicationPath().c_str());
			RegSetValueEx(key, L"EADM", 0, REG_SZ, (BYTE*) launcherPath,( DWORD ) ( wcslen( launcherPath ) * sizeof(wchar_t) ) ); 
		}
		else
		{
			RegDeleteValue(key, L"EADM");
		}

		RegCloseKey(key);
		return true;
	} 

	return false;
}

bool Registration::registerUrlProtocol(wchar_t* protocol_name, unsigned long protocol_size, wchar_t* keyName)
{
	// Points to HKEY_CLASSES_ROOT\keyName
	HKEY rootKey;

	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, keyName, 0, NULL, 0, KEY_SET_VALUE, NULL, &rootKey, NULL) != ERROR_SUCCESS)
	{
        ORIGINBS_LOG_ERROR << "Unable to create key " << keyName;
		return false;
	}

	// Set the protocol name
	if (RegSetValueEx(rootKey, NULL, 0, REG_SZ, (BYTE*)protocol_name, protocol_size) != ERROR_SUCCESS)
	{
        ORIGINBS_LOG_ERROR << "Unable to set protocol name: " << protocol_name;
		RegCloseKey(rootKey);
		return false;
	}

	// Set the "URL Protocol" key so Windows knows what this is
	if (RegSetValueEx(rootKey, L"URL Protocol", 0, REG_SZ, (BYTE*)"", 1) != ERROR_SUCCESS)
	{
        ORIGINBS_LOG_ERROR << "Unable to set the URL Protocol key";
		RegCloseKey(rootKey);
		return false;
	}

	// Set the command to run
	HKEY commandKey;

	if (RegCreateKeyEx(rootKey, L"shell\\open\\command", 0, NULL, 0, KEY_SET_VALUE, NULL, &commandKey, NULL) != ERROR_SUCCESS)
	{
        ORIGINBS_LOG_ERROR << "Unable to set the url protocol command key";
		RegCloseKey(rootKey);
		return false;
	}

	std::wostringstream command_line;
	std::wstring uiPath = ApplicationPath();

	command_line << L"\"" << uiPath.c_str() << L"\" \"%1\"";

	if (RegSetValueEx(commandKey, NULL, 0, REG_SZ, (BYTE*)command_line.str().c_str(), DWORD( (command_line.str().length() + 1) * sizeof(wchar_t)) ) != ERROR_SUCCESS)
	{
        ORIGINBS_LOG_ERROR << "Unable to set the command line for url protocol command key";
		RegCloseKey(rootKey);
		RegCloseKey(commandKey);
		return false;
	}

	RegCloseKey(rootKey);
	RegCloseKey(commandKey);

	return true;
}

bool Registration::registerUrlProtocol()
{
	wchar_t protocol_name_Eadm[] = L"URL:EADM Protocol";
	wchar_t protocol_name_Origin[] = L"URL:ORIGIN Protocol";
	bool result = true; // no error

	if(!registerUrlProtocol(protocol_name_Eadm, sizeof(protocol_name_Eadm), L"eadm"))
	{
        ORIGINBS_LOG_ERROR << "Unable to register Url Protocol eadm";
		result = false;
	}

	if(!registerUrlProtocol(protocol_name_Origin, sizeof(protocol_name_Origin), L"origin"))
	{
        ORIGINBS_LOG_ERROR << "Unable to register Url Protocol origin";
		result = false;
	}

    if(!registerUrlProtocol(protocol_name_Origin, sizeof(protocol_name_Origin), L"origin2"))
    {
        ORIGINBS_LOG_ERROR << "Unable to register Url Protocol origin2";
        result = false;
    }

	return result;
}
