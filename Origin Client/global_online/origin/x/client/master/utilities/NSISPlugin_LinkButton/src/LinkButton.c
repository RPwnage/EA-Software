#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <CommCtrl.h>
#include "nsis/pluginapi.h"

#define MAX_SYSLINK_INSTANCES 1024

int (NSISCALL *pfnExecuteCodeSegment)(int, HWND) = NULL;

typedef struct
{  
    HWND hwSysLinkInst;
    BOOL clicking;
    int callbackSegment;
} HwndInstanceData;

HwndInstanceData instances[MAX_SYSLINK_INSTANCES] = { 0 };
int numInstances = 0;

LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

BOOL updateInstanceSubclass(HWND hwInstance, int iFunc)
{
    int i = 0;
    int curInstance = 0;

    for (i = 0; i < numInstances; ++i)
    {
        if (instances[i].hwSysLinkInst == hwInstance)
        {
            instances[i].callbackSegment = iFunc;
            return TRUE;
        }
    }

    // Don't do too many
    if (numInstances == MAX_SYSLINK_INSTANCES)
        return FALSE;

    curInstance = numInstances;
    ++numInstances;

    instances[curInstance].hwSysLinkInst = hwInstance;
    instances[curInstance].clicking = FALSE;
    instances[curInstance].callbackSegment = iFunc;

    SetWindowSubclass(hwInstance, SubclassProc, 0, 0);

    return TRUE;
}

BOOL getCallbackDetailsForHwnd(HWND hwInstance, int* iFunc, BOOL* fClicking)
{
    int i = 0;
    for (i = 0; i < numInstances; ++i)
    {
        if (instances[i].hwSysLinkInst == hwInstance)
        {
            *iFunc = instances[i].callbackSegment;
            *fClicking = instances[i].clicking;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL setCallbackDetailsForHwnd(HWND hwInstance, BOOL fClicking)
{
    int i = 0;
    for (i = 0; i < numInstances; ++i)
    {
        if (instances[i].hwSysLinkInst == hwInstance)
        {
            instances[i].clicking = fClicking;
            return TRUE;
        }
    }

    return FALSE;
}

LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                               LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    int iFunc = 0;
    BOOL fClicking = FALSE;
    POINT pt;
	RECT rc;

    if (pfnExecuteCodeSegment != NULL && getCallbackDetailsForHwnd(hWnd, &iFunc, &fClicking))
    {
        switch (uMsg)
        {
        case WM_NCHITTEST:
            return HTCLIENT;
        case WM_LBUTTONDOWN:
            SetCapture(hWnd);
            setCallbackDetailsForHwnd(hWnd, TRUE);
            break;
        case WM_LBUTTONUP:
            ReleaseCapture();
            setCallbackDetailsForHwnd(hWnd, FALSE);
            if (fClicking)
            {
				pt.x=(short)LOWORD(lParam);
				pt.y=(short)HIWORD(lParam);
				ClientToScreen(hWnd,&pt);
				GetWindowRect(hWnd,&rc);

                // Callback
				if(PtInRect(&rc,pt))
                {
                    pfnExecuteCodeSegment(iFunc - 1, hWnd);
                }
                return 0;
            }
            break;
        }
    }

    //MessageBox(NULL, _T("PluginTest"), _T("PluginTest"), MB_OK | MB_ICONEXCLAMATION);               

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void __declspec(dllexport) BindLinkToAction(HWND hwndParent, int string_size, 
                                TCHAR *variables, stack_t **stacktop,
                                extra_parameters *extra)
{
    TCHAR szCallback[1024];
    HWND hwSysLink;
    int iFunc;

    EXDLL_INIT();

    {
        // Save callback
        pfnExecuteCodeSegment = extra->ExecuteCodeSegment;

        hwSysLink = (HWND) popint();
    
        if (hwSysLink == NULL || popstring(szCallback))
        {
            extra->exec_flags->exec_error = TRUE;
            pushint(0);
            return;
        }

        iFunc = myatoi(szCallback);
        
        if (!updateInstanceSubclass(hwSysLink, iFunc))
        {
            extra->exec_flags->exec_error = TRUE;
            pushint(0);
            return;
        }

        // Success?
        pushint(1);
    }
}

BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
