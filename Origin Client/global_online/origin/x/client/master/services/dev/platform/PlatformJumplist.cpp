//  PlatformJumplist.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include <QtWidgets>

#ifdef NTDDI_VERSION
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7  // Specifies that the minimum required platform is Windows 7.
#endif
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600	// Change this to the appropriate value to target Windows 7 or later.
#define WIN32_LEAN_AND_MEAN
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>

#include <objectarray.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlobj.h>
#endif

#include "services/platform/PlatformService.h"
#include "services/platform/PlatformJumplist.h"
#include "services/debug/DebugService.h"

using namespace Origin::Services;

namespace
{
	unsigned int s_iRecentListSize = 10;	// default
#if defined(Q_OS_WIN)
    bool sInitialized = false;
	ITaskbarList3 *s_pTaskbarList = NULL;		// Windows 7 Taskbar list instance
	HWND s_hProgressBar = NULL;					// Window handle of the ProgressBar control
	HWND s_hMainWindow = NULL;
	IObjectArray *s_pRemovedList = NULL;
	QList<HWND> s_lhWindowsList;
#endif
}

// helpers

#if defined(Q_OS_WIN)

struct structMainWindowSearch
{
    HINSTANCE   hInst;  // hInst to find window for
    HWND	hWnd;	// Found handle of requested hInst
};

BOOL CALLBACK getProcMainWindowCallback(HWND hWnd, LPARAM lParam)
{
    if (!hWnd) 
        return TRUE;

    structMainWindowSearch* pSrch = (structMainWindowSearch*)lParam;

    // Invalid search request with missing PID?
    if (0 == pSrch->hInst)
        return TRUE;

    HINSTANCE AppInstance = (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE);  

    if (AppInstance == pSrch->hInst)
    {
		s_lhWindowsList.push_back(hWnd);
        pSrch->hWnd = hWnd;
    }

    return TRUE; // No match so keep searching
}

HWND getProcMainWindow()
{
    structMainWindowSearch oSrch;
    oSrch.hInst = qWinAppInst();
    oSrch.hWnd = NULL;
	s_lhWindowsList.clear();

    ::EnumWindows(getProcMainWindowCallback, (LPARAM)&oSrch);
    return oSrch.hWnd;  // Success or failure
}

// Creates a CLSID_ShellLink to insert into the Tasks section of the Jump List.  This type of Jump
// List item allows the specification of an explicit command line to execute the task.
HRESULT CreateShellLink(PCWSTR pszArguments, PCWSTR pszTitle, IShellLink **ppsl)
{
    IShellLink *psl;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&psl));
    if (SUCCEEDED(hr))
    {
        // Determine our executable's file path so the task will execute this application
        WCHAR szAppPath[MAX_PATH];
        if (GetModuleFileName(NULL, szAppPath, ARRAYSIZE(szAppPath)))
        {
            hr = psl->SetPath(szAppPath);
            if (SUCCEEDED(hr))
            {
                hr = psl->SetArguments(pszArguments);
                if (SUCCEEDED(hr))
                {
                    // The title property is required on Jump List items provided as an IShellLink
                    // instance.  This value is used as the display name in the Jump List.
                    IPropertyStore *pps;
                    hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
                    if (SUCCEEDED(hr))
                    {
                        PROPVARIANT propvar;
                        hr = InitPropVariantFromString(pszTitle, &propvar);
                        if (SUCCEEDED(hr))
                        {
                            hr = pps->SetValue(PKEY_Title, propvar);
                            if (SUCCEEDED(hr))
                            {
                                hr = pps->Commit();
                                if (SUCCEEDED(hr))
                                {
                                    hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
                                }
                            }
                            PropVariantClear(&propvar);
                        }
                        pps->Release();
                    }
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        psl->Release();
    }
    return hr;
}

// The Tasks category of Jump Lists supports separator items.  These are simply IShellLink instances
// that have the PKEY_AppUserModel_IsDestListSeparator property set to TRUE.  All other values are
// ignored when this property is set.
HRESULT CreateSeparatorLink(IShellLink **ppsl)
{
    IPropertyStore *pps;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pps));
    if (SUCCEEDED(hr))
    {
        PROPVARIANT propvar;
        hr = InitPropVariantFromBoolean(TRUE, &propvar);
        if (SUCCEEDED(hr))
        {
            hr = pps->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
            if (SUCCEEDED(hr))
            {
                hr = pps->Commit();
                if (SUCCEEDED(hr))
                {
                    hr = pps->QueryInterface(IID_PPV_ARGS(ppsl));
                }
            }
            PropVariantClear(&propvar);
        }
        pps->Release();
    }
    return hr;
}

// Builds the collection of task items and adds them to the Task section of the Jump List.  All tasks
// should be added to the canonical "Tasks" category by calling ICustomDestinationList::AddUserTasks.
HRESULT AddTasksToList(ICustomDestinationList *pcdl, bool isUnderAgeUser)
{
    IObjectCollection *poc;
    HRESULT hr = CoCreateInstance(CLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
    if (SUCCEEDED(hr))
    {
        IShellLink * psl;

        hr = CreateShellLink(L"-Origin_NoAppFocus origin://redeem", QObject::tr("ebisu_client_redeem_game_code").toStdWString().data(), &psl);
        if (SUCCEEDED(hr))
        {
            hr = poc->AddObject(psl);
            psl->Release();
        }

        hr = CreateSeparatorLink(&psl);
        if (SUCCEEDED(hr))
        {
            hr = poc->AddObject(psl);
            psl->Release();
        }

        hr = CreateShellLink(L"-Origin_NoAppFocus origin://logout", QObject::tr("ebisu_mainmenuitem_logout").toStdWString().data(), &psl);
        if (SUCCEEDED(hr))
        {
            hr = poc->AddObject(psl);
            psl->Release();
        }

        hr = CreateShellLink(L"-Origin_NoAppFocus origin://quit", (QObject::tr("ebisu_mainmenuitem_quit_app").arg(QObject::tr("application_name"))).toStdWString().data(), &psl);
        if (SUCCEEDED(hr))
        {
            hr = poc->AddObject(psl);
            psl->Release();
        }

#if 0
        if (SUCCEEDED(hr))
        {
            hr = CreateSeparatorLink(&psl);
            if (SUCCEEDED(hr))
            {
                hr = poc->AddObject(psl);
                psl->Release();
            }
        }
#endif

        if (SUCCEEDED(hr))
        {
            IObjectArray * poa;
            hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
            if (SUCCEEDED(hr))
            {
                // Add the tasks to the Jump List. Tasks always appear in the canonical "Tasks"
                // category that is displayed at the bottom of the Jump List, after all other
                // categories.
                hr = pcdl->AddUserTasks(poa);
                poa->Release();
            }
        }
        poc->Release();
    }
    return hr;
}

// Determines if the provided IShellLink is listed in the array of items that the user has removed
bool IsLinkInArray(const QString link_param, IObjectArray *poaRemoved)
{
	const int kMaxArgSize = 256;
    bool fRet = false;
    UINT cItems;
//	WCHAR link_arg[kMaxArgSize];
	WCHAR comp_arg[kMaxArgSize];

//	psi->GetArguments(link_arg, kMaxArgSize);

//	const QString link_arg_str = QString::fromWCharArray(link_arg);

    if (SUCCEEDED(poaRemoved->GetCount(&cItems)))
    {
        IShellLink *psiCompare;
        for (UINT i = 0; !fRet && i < cItems; i++)
        {
            if (SUCCEEDED(poaRemoved->GetAt(i, IID_PPV_ARGS(&psiCompare))))
            {
				psiCompare->GetArguments(comp_arg, kMaxArgSize);
				const QString comp_arg_str = QString::fromWCharArray(comp_arg);

                fRet = (link_param == comp_arg_str);
                psiCompare->Release();
            }

			if (fRet)
				break;
        }
    }
    return fRet;
}
#endif

void PlatformJumplist::init()
{
#ifdef Q_OS_WIN
	ORIGIN_ASSERT(!sInitialized);
    if (!sInitialized)
    {
		s_hMainWindow = NULL;
		s_hProgressBar = NULL;
		s_pTaskbarList = NULL;
		s_pRemovedList = NULL;
		s_lhWindowsList.clear();

		unsigned int os_major_version = PlatformService::OSMajorVersion();
		if ((os_major_version < 6) || ((os_major_version == 6) && (PlatformService::OSMinorVersion() == 0)))
			return;	// less than Windows 7? then just return

		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (!SUCCEEDED(hr))
			return;

		sInitialized = true;
        
        // Clear the jumplist so that we don't have any leftover links from earlier sessions.
        clear_jumplist();
    }
#endif
}

void PlatformJumplist::release()
{
#ifdef Q_OS_WIN
    if (sInitialized)
    {
		s_hMainWindow = NULL;
		s_hProgressBar = NULL;

		if (s_pTaskbarList)
		{
			s_pTaskbarList->Release();
			s_pTaskbarList = NULL;
		}

		s_lhWindowsList.clear();

		if (s_pRemovedList)
			s_pRemovedList->Release();

		s_pRemovedList = NULL;

		sInitialized = false;
		CoUninitialize();
    }
#endif
}

void PlatformJumplist::create_jumplist(bool isUnderAgeUser)
{
#ifdef Q_OS_WIN
	if (!sInitialized)
		return;
	
	// Create the custom Jump List object.
	ICustomDestinationList *pcdl = NULL;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));

	if (SUCCEEDED(hr) && pcdl)
	{
		hr = pcdl->BeginList(&s_iRecentListSize, IID_PPV_ARGS(&s_pRemovedList));
		if (SUCCEEDED(hr))
		{
			pcdl->AppendKnownCategory(KDC_RECENT);

			hr = AddTasksToList(pcdl, isUnderAgeUser);
			if (SUCCEEDED(hr))
			{
				// Commit the list-building transaction.
				pcdl->CommitList();
			}
		}
		pcdl->Release();
	}
#endif
}

void PlatformJumplist::clear_jumplist()
{
#ifdef Q_OS_WIN
	if (!sInitialized)
		return;
	
	// Create the custom Jump List object.
	ICustomDestinationList *pcdl = NULL;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pcdl));

	if (SUCCEEDED(hr) && pcdl)
	{
		hr = pcdl->BeginList(&s_iRecentListSize, IID_PPV_ARGS(&s_pRemovedList));
		if (SUCCEEDED(hr))
		{
			// Commit the empty list.
			pcdl->CommitList();
		}
		pcdl->Release();
	}
#endif
}

void PlatformJumplist::add_launched_game(QString title, QString product_id, QString icon_path, int itemSubType)
{
#ifdef Q_OS_WIN
	if (!sInitialized)
		return;
	
	IShellLink *psl;
	QString param("origin://launchgamejump/");	// needs to add "jump" to the end for telemetry otherwise we can't distinguish when a game is launched if it was from the jumplist
	param += product_id;
    param += QString("?ItemSubType=%1").arg(itemSubType);

	if (s_pRemovedList && IsLinkInArray(param, s_pRemovedList))
		return;

	HRESULT hr = CreateShellLink(param.toStdWString().data(), title.toStdWString().data(), &psl);
	if (SUCCEEDED(hr))
	{
		psl->SetIconLocation(icon_path.toStdWString().data(), 0);
		
		SHAddToRecentDocs(SHARD_LINK, psl);
		psl->Release();
	}
#endif
}

void PlatformJumplist::clear_recently_played()
{
#ifdef Q_OS_WIN
	if (!sInitialized)
		return;

	IApplicationDestinations *pDests;
	HRESULT hr = CoCreateInstance(CLSID_ApplicationDestinations, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDests));

    if ( SUCCEEDED(hr) )
    {
		pDests->RemoveAllDestinations();

		pDests->Release();
	}
#endif
}

void PlatformJumplist::update_progress(double progress, bool paused)
{
#ifdef Q_OS_WIN
    if (!sInitialized)
		return;

	// for Taskbar Progress
	bool same_window = true;
	HWND wind = getProcMainWindow();
	if (s_hMainWindow != wind)
	{
		same_window = false;	// window changed
		s_hMainWindow = wind;
	}

	if (s_hMainWindow)
	{
		HRESULT hr;

		// Initialize the Windows 7 Taskbar list instance
		// if we already have the taskbar but the window changed - reset and get the new one
		if (s_pTaskbarList && !same_window)
		{
			s_pTaskbarList->Release();
			s_pTaskbarList = NULL;
		}

		if (s_pTaskbarList == NULL)
		{
			hr = CoCreateInstance(CLSID_TaskbarList, 
				NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&s_pTaskbarList));
			if (!SUCCEEDED(hr))
				s_pTaskbarList = NULL;
		}

		if (s_pTaskbarList)
		{
			for(QList<HWND>::const_iterator it = s_lhWindowsList.begin(); it != s_lhWindowsList.end(); it++)
			{
				HWND hwnd = (*it);
			
				// Set the Taskbar ProgressBar to the NORMAL state and half of the
				// progress value
				if (paused)
					s_pTaskbarList->SetProgressState(hwnd, TBPF_PAUSED);
				else
				{
					if (progress == 0)
					{
						s_pTaskbarList->SetProgressState(hwnd, TBPF_NOPROGRESS);	// otherwise there is a little sliver of green 
						continue;
					}
					else
						s_pTaskbarList->SetProgressState(hwnd, TBPF_NORMAL);
				}
				s_pTaskbarList->SetProgressValue(hwnd, (ULONG)progress, (ULONG)100);
			}
		}
	}
#endif
}

unsigned int PlatformJumplist::get_recent_list_size()
{
	return s_iRecentListSize;
}
