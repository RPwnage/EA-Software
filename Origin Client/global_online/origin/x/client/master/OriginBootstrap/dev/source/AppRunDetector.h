/////////////////////////////////////////////////////////////////////////////////////
// AppRunDetector.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
// Created by Paul Pedriana.
/////////////////////////////////////////////////////////////////////////////////////


#ifndef APPRUNDETECTOR_H
#define APPRUNDETECTOR_H

#ifdef _MSC_VER
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)
#else
#include <windows.h>
#endif

#include <string>


namespace EA
{
	/// AppRunDetector
	///
	/// Tells if an app is running already, and can switch to it.
	/// This class would be created and used early in app startup.
	///
	class AppRunDetector
	{
	public:
		static const int kWin32ActivateIdentifier = 124540; // Sent to the WinProc of the other instance to be activated.

		AppRunDetector(const wchar_t* pAppName, const wchar_t* pDetectorWindowClass, const wchar_t* pDetectorWindowName, const wchar_t* pWindowClass, HINSTANCE instance);
		~AppRunDetector();

		void Invalidate();		// forces a re-check

		// scope: system wide
		bool IsAnotherInstanceRunning();    // Tests if another instance is currently running.
		bool WasAnotherInstanceRunning();   // Returns true if the last call to 

		// scope: session wide
		bool IsAnotherInstanceRunningLocal();    // Tests if another instance is currently running.
		bool WasAnotherInstanceRunningLocal();   // Returns true if the last call to 

		bool ActivateRunningInstance(std::wstring argument, bool bFwdAppFocus = true);     // Returns true if the instance could be found.
		bool ForceRunningInstanceToTerminate();							// forcefully terminates the instance process
		void ShowWindowsRunningApplication();

	protected:
		bool           mbAnotherInstanceWasRunningLocal;
		bool           mbAnotherInstanceWasRunningGlobal;
		const wchar_t* mpAppName;
		const wchar_t* mpDetectorWindowClass;       //this class is used to detect another instance of origin running
		const wchar_t* mpDetectorWindowName;        //and name of the window used for detection
        const wchar_t* mpWindowClass;               //this class is used to retrieve the actual window of the instance of origin running

#if defined(_WIN32)
		HANDLE mhLocalInstanceMutex; // Handle to this instance.
		HANDLE mhGlobalInstanceMutex; // Handle to this instance.
#endif
	};


} // namespace EA


#endif // Header include guard




