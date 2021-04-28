// Simple utility for displaying the command line, environment variables and any other useful information from a game launch
//

#include "stdafx.h"
#include "ShowCommandLine.h"
#include <string>
#include <Windows.h>
#include <CommDlg.h>
#include <ShellAPI.h>
#include <strsafe.h>
#include <assert.h>

// Global Variables:
HINSTANCE hInst;								// current instance

BOOL InitInstance(HINSTANCE, int, LPTSTR lpCmdLine);
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL IsElevated();
void HandleHookCommand();
void HandleUnhookCommand();
HWND ghWnd;

LPWSTR* gszArgList;
int gArgCount;


int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;


    gszArgList = CommandLineToArgvW(GetCommandLine(), &gArgCount);
    if (gszArgList == NULL)
    {
        MessageBox(NULL, L"Unable to parse command line", L"Error", MB_OK);
        return -1;
    }

    std::wstring sCmdLine(lpCmdLine);
    bool bHook = sCmdLine.find(L"-showcommandline_hook") != -1;
    bool bUnhook = sCmdLine.find(L"-showcommandline_unhook") != -1;
    bool bHookUnelevated = sCmdLine.find(L"-showcommandline_unelevated") != -1;

    if (bHook)
    {
        // If we're running elevated or if it's ok to try without elevation, handle the hook request
        if (IsElevated() || bHookUnelevated)
        {
            HandleHookCommand();
            return TRUE;
        }

        // Re-run ourselves elevated
        ShellExecute(NULL, L"runas", gszArgList[0], lpCmdLine, NULL, nCmdShow);
        return TRUE;
    }
    else if (bUnhook)
    {
        if (IsElevated() || bHookUnelevated)
        {
            HandleUnhookCommand();
            return TRUE;
        }

        // Re-run ourselves elevated
        ShellExecute(NULL, L"runas", gszArgList[0], lpCmdLine, NULL, nCmdShow);
        return TRUE;
    }





	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow, lpCmdLine))
	{
		return FALSE;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

BOOL IsElevated() 
{
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if( OpenProcessToken( GetCurrentProcess( ),TOKEN_QUERY,&hToken ) ) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof( TOKEN_ELEVATION );
        if( GetTokenInformation( hToken, TokenElevation, &Elevation, sizeof( Elevation ), &cbSize ) ) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if( hToken ) {
        CloseHandle( hToken );
    }
    return fRet;
}

std::wstring GetEnvironmentVars()
{
    std::wstring sReturn;

    LPWCH block = GetEnvironmentStrings();
    LPWCH pTemp = block;

    std::wstring sLine;
    while (1)
    {
        while ((*pTemp))
        {
            sLine += *pTemp;
            pTemp++;
        }

        sReturn += sLine + L"\r\n";
        sLine.clear();
        pTemp++;

        if (*pTemp == 0)    // double null?
            break;
    }

    FreeEnvironmentStrings(block);

    return sReturn;
}
void SetLastUnhookedLocation()
{
    HKEY hKey;
    std::wstring subKey = L"SOFTWARE\\Electronic Arts\\ShowCommandLine";

    HRESULT hr = RegCreateKeyEx( HKEY_CURRENT_USER, subKey.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	assert(SUCCEEDED(hr));

    std::wstring value = L"location";
    std::wstring data(gszArgList[0]);

    hr = RegSetValueEx (hKey, value.c_str(), 0, REG_SZ, (BYTE*) data.data(), data.length()*sizeof(wchar_t));
	assert(SUCCEEDED(hr));

    RegCloseKey(hKey);
}

std::wstring ReadLastUnhookedLocation()
{
    std::wstring subKey = L"SOFTWARE\\Electronic Arts\\ShowCommandLine";
    std::wstring value = L"location";
    wchar_t buf[512];
    DWORD nLength = 512;
    RegGetValue( HKEY_CURRENT_USER, subKey.c_str(), value.c_str(), RRF_RT_REG_SZ, NULL, buf, &nLength);

    return std::wstring(buf);
}

std::wstring GetHookFilename(std::wstring baseName)
{
    return (baseName.substr(0, baseName.length()-4) +L"_showcommandlinehook.exe");
}

bool IsHooked()
{
    // this app is running in "hooked" mode if in the same folder there is a exe that matches the hooked name
    std::wstring sHookedName = GetHookFilename(gszArgList[0]);

    DWORD dwAttrib = GetFileAttributes(sHookedName.c_str());

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)); // if this exists then this app, this app is in hooked mode
}

void HandleUnhookCommand()
{
    if (!IsElevated())
    {
        MessageBox(NULL, L"This app may need to be elevated to unhook...going to try anyway.", L"Warning", MB_OK|MB_ICONWARNING);
    }

    std::wstring originalAppName = gszArgList[2];
    std::wstring appToMove = gszArgList[3];


/*    std::wstring sMessage = L"original:\"";
    sMessage += originalAppName;
    sMessage += L"\" toMove: \"";
    sMessage += appToMove;
    sMessage += L"\""; 

    MessageBox(NULL, sMessage.c_str(), L"move params", MB_OK);*/

    if (DeleteFile(originalAppName.c_str()) == FALSE)
    {
        std::wstring sError = L"Couldn't delete file: " + originalAppName;
        MessageBox(NULL, sError.c_str(), L"Unhook Error!", MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    if (MoveFile(appToMove.c_str(), originalAppName.c_str()) == FALSE)
    {
        std::wstring sError = L"Couldn't rename file: " + appToMove + L" to " + originalAppName;
        MessageBox(NULL, sError.c_str(), L"Unhook Error!", MB_OK | MB_ICONEXCLAMATION);
        return;
    }

    MessageBox(NULL, L"Unhook successful", L"Success", MB_OK);
}

void HandleHookCommand()
{
    if (!IsElevated())
    {
        MessageBox(NULL, L"This app may need to be elevated to hook...going to try anyway.", L"Warning", MB_OK|MB_ICONWARNING);
    }


    OPENFILENAME ofn;       // common dialog box structure
    wchar_t szFile[260];       // buffer for file name

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"*.exe\0*.exe\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box. 

    if (GetOpenFileName(&ofn)==TRUE)
    {
        std::wstring newName = GetHookFilename(szFile);
        if (MoveFile(szFile, newName.c_str()))
        {
            if (CopyFile(gszArgList[0], szFile, TRUE))
            {
                MessageBox(NULL, L"Hook successful", L"Success", MB_OK);
                return;
            }
        }
    }

    MessageBox(NULL, L"Couldn't hook", L"Error", MB_OK|MB_ICONEXCLAMATION);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, LPTSTR lpCmdLine)
{
   hInst = hInstance; // Store instance handle in our global variable

   ghWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);

   std::wstring sText = L"=EXE Path=\r\n";
   sText.append(gszArgList[0]);
   sText.append(L"\r\n\r\n=Args=\r\n");
   sText.append(lpCmdLine);

   sText.append(L"\r\n\r\n=Program State=\r\n");

   if (IsElevated())
        sText.append(L"Running Elevated=yes");
   else
       sText.append(L"Running Elevated=no");

   sText.append(L"\r\n\r\n=Environment=\r\n");
   sText.append(GetEnvironmentVars());

   SetDlgItemText(ghWnd, IDC_EDIT1, sText.c_str());


   if (!ghWnd)
   {
      return FALSE;
   }

   int nScreenWidth = 1920;
   int nScreenHeight = 1080;

   RECT rClient;
   GetClientRect(ghWnd, &rClient);

   int nScreenX = (nScreenWidth - rClient.right - rClient.left)/2;
   int nScreenY = (nScreenHeight - rClient.bottom - rClient.top)/2;

   SetWindowPos(ghWnd, NULL, nScreenX, nScreenY, -1, -1, SWP_NOSIZE | SWP_NOZORDER);

   if (IsHooked())
   {
       SetDlgItemText(ghWnd, IDC_BUTTON_HOOK, L"Unhook");
       EnableWindow( GetDlgItem( ghWnd, IDC_BUTTON_CONTINUE), TRUE );
   }

   ShowWindow(ghWnd, nCmdShow);
   UpdateWindow(ghWnd);

   return TRUE;
}


INT_PTR CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        case IDC_BUTTON_CLIP:
            {
                std::wstring sText;

                const int kMaxSize = 16*1024;// 16k
                wchar_t buf[kMaxSize];   
                GetDlgItemText(ghWnd, IDC_EDIT1, buf, kMaxSize);

                sText.assign( buf );

                if (OpenClipboard(ghWnd))
                {
                    EmptyClipboard();

                    HLOCAL hMem =  GlobalAlloc( LHND, sText.length() * sizeof(wchar_t) );
                    wchar_t* cptr = (wchar_t*) GlobalLock(hMem);
                    memcpy(cptr, sText.data(), sText.length() * sizeof(wchar_t));
                    SetClipboardData(CF_UNICODETEXT, hMem);
                    LocalUnlock( hMem );
                    LocalFree( hMem );
                    CloseClipboard();
                }
            }
            break;
        case IDC_BUTTON_HOOK:
            {
                if (IsHooked())
                {
                    std::wstring sUnhookExecuteString( ReadLastUnhookedLocation() );
                    sUnhookExecuteString += L" \"";
                    sUnhookExecuteString += gszArgList[0];
                    sUnhookExecuteString += L"\" \"";
                    sUnhookExecuteString += GetHookFilename( gszArgList[0] );
                    sUnhookExecuteString += L"\" -showcommandline_unhook";
                    ShellExecute(NULL, L"runas", ReadLastUnhookedLocation().c_str(), sUnhookExecuteString.c_str(), NULL, SW_SHOWNORMAL);
                }
                else
                {
                    SetLastUnhookedLocation();

                    std::wstring sExecuteWithHook(GetCommandLine());
                    sExecuteWithHook += L" -showcommandline_hook";
                    ShellExecute(NULL, L"runas", gszArgList[0], sExecuteWithHook.c_str(), NULL, SW_SHOWNORMAL);
                }

                PostQuitMessage(0);
            }
            break;
        case IDC_BUTTON_CONTINUE:
            {
                ShellExecute(NULL, L"open", GetHookFilename( gszArgList[0] ).c_str(), GetCommandLine(), NULL, SW_SHOWNORMAL);
                PostQuitMessage(0);
            }
            break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
		break;
	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

