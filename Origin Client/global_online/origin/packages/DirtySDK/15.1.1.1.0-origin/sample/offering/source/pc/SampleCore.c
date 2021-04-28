/*H********************************************************************************/
/*!
    \File SampleCore.c

    \Description
        A simple test harness for exercising high level modal screen code

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 03/14/2004 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <commctrl.h>
#include <richedit.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "samplecore.h"

/*** Defines **********************************************************************/

#define WM_USER_DIALOG  (WM_APP+1000)       // create a dialog
#define WM_USER_QUIT    (WM_APP+1001)       // quit the sample

/*** Type Definitions *************************************************************/

typedef struct DialogRef        // dialog state
{
    HWND pHandle;               // handle to dialog window
    char strResource[256];      // resource identifier
    volatile int32_t iCreated;      // flag indicating create is complete
    volatile int32_t iButton;       // last button clicked
    struct {
        int32_t iItem;              // item number
        uint32_t uRGB;      // color
    } Color[32];
} DialogRef;

/*** Externals ********************************************************************/

void SampleStartup(char *strCommandLine);
void SampleShutdown(void);
int32_t SampleThread(const char *pConfig);

/*** Variables ********************************************************************/

static DWORD _DialogThread = 0;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _DialogCallback

    \Description
        Dialog window callback

    \Input pHandle - window handle
    \Input iMessage - message number
    \Input wparm - message specific parameter
    \Input lparm - message specific parameter

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
static LRESULT CALLBACK _DialogCallback(HWND pHandle, UINT iMessage, WPARAM wparm, LPARAM lparm)
{
    int32_t iIndex;
    DialogRef *pDialog = (DialogRef *) GetWindowLong(pHandle, DWL_USER);

    // handle init special (create the class)
    if (iMessage == WM_INITDIALOG)
    {
        // store reference value
        SetWindowLong(pHandle, DWL_USER, lparm);
        pDialog = (DialogRef *)lparm;
        // no button pressed yet
        pDialog->iButton = 0;
    }

    // handle buttons
    if (iMessage == WM_COMMAND)
    {
        char strClass[256] = "";
        GetClassName(GetDlgItem(pHandle, LOWORD(wparm)), strClass, sizeof(strClass));
        if (strcmp(strClass, "Button") == 0)
        {
            pDialog->iButton = LOWORD(wparm);
        }
    }

    // handle draw color messages
    if (iMessage == WM_CTLCOLORSTATIC)
    {
        int32_t iItem = GetWindowLong((HWND)lparm, GWL_ID);
        // search for new color info
        for (iIndex = 0; iIndex < sizeof(pDialog->Color)/sizeof(pDialog->Color[0]); ++iIndex)
        {
            if (pDialog->Color[iIndex].iItem == iItem)
            {
                SetTextColor((HDC)wparm, pDialog->Color[iIndex].uRGB);
                SetBkColor((HDC)wparm, GetSysColor(COLOR_MENU));
                return((LRESULT)GetStockObject(NULL_BRUSH));
            }
        }
    }

    // handle draw color messages
    if (iMessage == WM_CTLCOLORBTN)
    {
        int32_t iItem = GetWindowLong((HWND)lparm, GWL_ID);
        // search for new color info
        for (iIndex = 0; iIndex < sizeof(pDialog->Color)/sizeof(pDialog->Color[0]); ++iIndex)
        {
            if (pDialog->Color[iIndex].iItem == iItem)
            {
                RECT Rect;
                int32_t iCount;
                HBRUSH pBrush = CreateSolidBrush(pDialog->Color[iIndex].uRGB);
                GetClientRect((HWND)lparm, &Rect);
                for (iCount = 0; iCount < 3; ++iCount)
                {
                    Rect.top -= 1;
                    Rect.left -= 1;
                    Rect.bottom += 1;
                    Rect.right += 1;
                    FrameRect((HDC)wparm, &Rect, pBrush);
                }
                DeleteObject(pBrush);
            }
        }
    }

    // let windows handle
    return(FALSE);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DialogCreate

    \Description
        Create/open a new dialog from a resource template

    \Input pResource - identifier for resource template

    \Output HWND - Window handle

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
HWND DialogCreate(const char *pResource)
{
    HWND pHandle;
    DialogRef *pDialog;

    // allocate storage
    pDialog = (DialogRef *)HeapAlloc(GetProcessHeap(), 0, sizeof(*pDialog));
    memset(pDialog, 0, sizeof(*pDialog));

    // save reference
    strcpy(pDialog->strResource, pResource);
    // tell dialog thread to create
    while (PostThreadMessage(_DialogThread, WM_USER_DIALOG, 0, (LPARAM)pDialog) == 0)
    {
        Sleep(25);
    }

    // wait for response
    while (pDialog->iCreated == 0)
    {
        Sleep(25);
    }

    // kill structure if create failed
    pHandle = pDialog->pHandle;
    if (pHandle == NULL)
    {
        HeapFree(GetProcessHeap(), 0, pDialog);
    }

    // return reference
    return(pHandle);
}

/*F********************************************************************************/
/*!
    \Function DialogDestroy

    \Description
        Close/destroy a dialog previously opened with DialogCreate

    \Input pHandle - window handle from DialogCreate

    \Output NULL (to allow inline destroy/variable clear)

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
HWND DialogDestroy(HWND pHandle)
{
    DialogRef *pDialog = (DialogRef *) GetWindowLong(pHandle, DWL_USER);

    // close & destroy if window handle is valid
    if (pHandle != NULL)
    {
        ShowWindow(pHandle, SW_HIDE);
        DestroyWindow(pHandle);
        HeapFree(GetProcessHeap(), 0, pDialog);
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function DialogCheckButton

    \Description
        Check to see if a button was clicked

    \Input pHandle - window handle from DialogCreate

    \Output int32_t - button id or zero

    \Version 04/13/2004 gschaefer
*/
/********************************************************************************F*/
int32_t DialogCheckButton(HWND pHandle)
{
    int32_t iButton;
    DialogRef *pDialog = (DialogRef *) GetWindowLong(pHandle, DWL_USER);

    // see if button pressed
    iButton = pDialog->iButton;
    if (iButton != 0)
        pDialog->iButton = 0;
    return(iButton);
}

/*F********************************************************************************/
/*!
    \Function DialogWaitButton

    \Description
        Wait for user to click a button and return button identifier

    \Input pHandle - window handle from DialogCreate

    \Output int32_t - button id

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
int32_t DialogWaitButton(HWND pHandle)
{
    int32_t iButton;
    DialogRef *pDialog = (DialogRef *) GetWindowLong(pHandle, DWL_USER);

    // show window and wait for button press (from window thread)
    ShowWindow(pHandle, SW_SHOW);
    while ((iButton = pDialog->iButton) == 0)
    {
        Sleep(100);
    }
    ShowWindow(pHandle, SW_HIDE);

    // reset button indicator and return
    pDialog->iButton = 0;
    return(iButton);
}

/*F********************************************************************************/
/*!
    \Function DialogColor

    \Description
        Set the color of a dialog item

    \Input None

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogColor(HWND pHandle, int32_t iItem, uint32_t uRGB)
{
    int32_t iIndex;
    int32_t iAvail = -1;
    DialogRef *pDialog = (DialogRef *) GetWindowLong(pHandle, DWL_USER);

    // look for match or empty slot
    for (iIndex = 0; iIndex < sizeof(pDialog->Color)/sizeof(pDialog->Color[0]); ++iIndex)
    {
        if (pDialog->Color[iIndex].iItem == 0)
        {
            iAvail = iIndex;
        }
        else if (pDialog->Color[iIndex].iItem == iItem)
        {
            break;
        }
    }

    // see if we need to use new entry
    if (iIndex >= sizeof(pDialog->Color)/sizeof(pDialog->Color[0]))
    {
        iIndex = iAvail;
    }

    // set the color
    if (iIndex >= 0)
    {
        pDialog->Color[iIndex].iItem = iItem;
        pDialog->Color[iIndex].uRGB = uRGB;
    }
}

/*F********************************************************************************/
/*!
    \Function DialogQuit

    \Description
        Signal end of application

    \Input None

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogQuit(void)
{
    while (PostThreadMessage(_DialogThread, WM_USER_QUIT, 0, (LPARAM)0) == 0)
    {
        Sleep(25);
    }
}

/*F********************************************************************************/
/*!
    \Function DialogComboInit

    \Description
        Init items in combo box (sequence of strings with "" at end)

    \Input pHandle - dialog handle
    \Input iItem - item number of combo box
    \Input pList - pointer to string list

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogComboInit(HWND pHandle, int32_t iItem, const char *pList)
{
    SendDlgItemMessage(pHandle, iItem, CB_RESETCONTENT, 0, 0);

    // walk list and populate
    while (*pList != 0)
    {
        SendDlgItemMessage(pHandle, iItem, CB_ADDSTRING, 0, (LPARAM)pList);
        pList += strlen(pList)+1;
    }

    // default to first item in list
    SendDlgItemMessage(pHandle, iItem, CB_SETCURSEL, 0, 0);
}

/*F********************************************************************************/
/*!
    \Function DialogComboAdd

    \Description
        Add an individual item to a combo box

    \Input pHandle - dialog handle
    \Input iItem - item number of combo box
    \Input pAdd - pointer to string

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogComboAdd(HWND pHandle, int32_t iItem, const char *pAdd)
{
    SendDlgItemMessage(pHandle, iItem, CB_ADDSTRING, 0, (LPARAM)pAdd);

    // if we added first item, make it the default (can be changed later)
    if (SendDlgItemMessage(pHandle, iItem, CB_GETCOUNT, 0, 0) == 1)
    {
        SendDlgItemMessage(pHandle, iItem, CB_SETCURSEL, 0, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function DialogComboSelect

    \Description
        Set default combo box item

    \Input pHandle - dialog handle
    \Input iItem - item number of combo box
    \Input pSelect - name of entry to select

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogComboSelect(HWND pHandle, int32_t iItem, const char *pSelect)
{
    SendDlgItemMessage(pHandle, iItem, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)pSelect);
}

/*F********************************************************************************/
/*!
    \Function DialogComboQuery

    \Description
        Signal end of application

    \Input pHandle - dialog handle
    \Input iItem - item number of combo box
    \Input pDataBuf - pointer to return buffer
    \Input pDataLen - size of return buffer

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogComboQuery(HWND pHandle, int32_t iItem, const char *pDataBuf, int32_t iDataLen)
{
    SendDlgItemMessage(pHandle, iItem, CB_GETCURSEL, 0, 0);
    SendDlgItemMessage(pHandle, iItem, CB_GETLBTEXT, iItem, (LPARAM)pDataBuf);
}

/*F********************************************************************************/
/*!
    \Function DialogButtonCenter

    \Description
        Center buttons on a dialog (centering depends on number of buttons)

    \Input pHandle - dialog handle
    \Input iBtn1 - button 1 identifier (required)
    \Input iBtn2 - button 2 identifier (optional, 0 if not used)
    \Input iBtn3 - button 3 identifier (optional, 0 if not used)

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogButtonCenter(HWND pHandle, int32_t *pButtons, int32_t iCount)
{
    RECT Rect;
    int32_t iStep;
    int32_t iIndex;
    HANDLE pButton;

    // figure out step size
    GetClientRect(pHandle, &Rect);
    iStep = (Rect.right-Rect.left) / iCount;

    // position the buttons
    for (iIndex = 0; iIndex < iCount != 0; ++iIndex)
    {
        // get the button position
        pButton = GetDlgItem(pHandle, pButtons[iIndex]);
        GetWindowRect(pButton, &Rect);
        // convert to client coordinates
        ScreenToClient(pHandle, ((LPPOINT)&Rect)+0); 
        ScreenToClient(pHandle, ((LPPOINT)&Rect)+1); 
        // reposition
        SetWindowPos(pButton, NULL, (iStep*iIndex+iStep/2)-(Rect.right-Rect.left)/2, Rect.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
    }
}

/*F********************************************************************************/
/*!
    \Function DialogImageSet

    \Description
        Store a bitmap into a static dialog object

    \Input pHandle - dialog handle
    \Input iBtn1 - button 1 identifier (required)
    \Input iBtn2 - button 2 identifier (optional, 0 if not used)
    \Input iBtn3 - button 3 identifier (optional, 0 if not used)

    \Output None

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
void DialogImageSet(HWND pHandle, int32_t iItem, int32_t iWidth, int32_t iHeight, unsigned char *pPalette, unsigned char *pBitmap)
{
    int32_t width;
    int32_t height;
    int32_t bwidth;
    HBITMAP hBitmap;
    uint32_t uAlpha;
    unsigned char *bmiPixels;
    BITMAPINFOHEADER *bmiHeader;

    // figure bitmap width with required alignment
    // apr 25, 2005: does not appear to be required in modern windoze versions
    // removed since it created problems with the random final pixel (didn't know
    // what color to make it)
//  bwidth = (iWidth+1)&0x7ffe;
    // replace with real width
    bwidth = iWidth;

    // alloc and open our new buffer
    bmiHeader = malloc(sizeof(BITMAPINFOHEADER)+(bwidth*iHeight*4));
    if (bmiHeader == NULL)
    {
        return;
    }

    // create compound dibitmap object
    bmiHeader->biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader->biWidth = bwidth;
    bmiHeader->biHeight = iHeight;
    bmiHeader->biPlanes = 1;
    bmiHeader->biBitCount = 32;
    bmiHeader->biCompression = BI_RGB;
    bmiHeader->biSizeImage = 0;
    bmiHeader->biXPelsPerMeter = 0;
    bmiHeader->biYPelsPerMeter = 0;
    bmiHeader->biClrUsed = 0;
    bmiHeader->biClrImportant = 0;

    // convert from palette indexes into RGB data
    bmiPixels = (unsigned char *)(bmiHeader+1);
    for (height = 0; height < iHeight; ++height)
    {
        for (width = 0; width < iWidth; ++width)
        {
            // note that palette is in RGBA order but windows
            // wants BGRA -- hense the +2/+1/+0/+3 offsets
            *bmiPixels++ = pPalette[(*pBitmap*4)+2];
            *bmiPixels++ = pPalette[(*pBitmap*4)+1];
            *bmiPixels++ = pPalette[(*pBitmap*4)+0];
            *bmiPixels++ = pPalette[(*pBitmap*4)+3];
            pBitmap += 1;
        }
        for (; width < bwidth; ++width)
        {
            // set extra pixels to full alpha transparent
            *bmiPixels++ = 0;
            *bmiPixels++ = 0;
            *bmiPixels++ = 0;
            *bmiPixels++ = 0;
        }
    }

    // figure out the dialog backgroud color to make alpha work
    uAlpha = GetSysColor(COLOR_MENU);
    if (GetSysColorBrush(COLOR_BTNFACE) != NULL)
    {
        // we must be under XP -- use btnface instead
        uAlpha = GetSysColor(COLOR_BTNFACE);
    }

    // replace transparent colors with appropriate background color
    bmiPixels = (unsigned char *)(bmiHeader+1);
    for (height = 0; height < iHeight; ++height)
    {
        for (width = 0; width < bwidth; ++width)
        {
            if (bmiPixels[3] == 0)
            {
                bmiPixels[0] = (unsigned char)(uAlpha>>16);
                bmiPixels[1] = (unsigned char)(uAlpha>>8);
                bmiPixels[2] = (unsigned char)(uAlpha>>0);
            }
            bmiPixels += 4;
        }
    }

    // create a bitmap handle
    hBitmap = CreateDIBitmap(GetDC(NULL), bmiHeader, CBM_INIT, (bmiHeader+1), (BITMAPINFO *)bmiHeader, DIB_RGB_COLORS);
    hBitmap = (HBITMAP)SendDlgItemMessage(pHandle, iItem, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
    // delete the old bitmap
    DeleteObject(hBitmap);
}



/*F********************************************************************************/
/*!
    \function : DialogImageSet2
    
    \Description 
        Store a bitmap into a static dialog object
  
    \Input pHandle - the windows handle
    \Input iItem   - the Dialog identifier
    \Input pBitmap - the image data 
    \Input iWidth  - the Width of the image
    \Input iHeight - Height of the image
    
    \Output - None
    
    \Version 06/09/2006 (ozietman)
*/ 
/********************************************************************************F*/
void DialogImageSet2(HWND pHandle, int32_t iItem, unsigned char *pBitmap, int32_t iWidth, int32_t iHeight)
{
    HBITMAP hbm;
    BITMAPINFOHEADER bmiHeader;
    BITMAPINFO bmInfo;
    int32_t width;
    int32_t height;
    unsigned char *bmiPixels;
    uint32_t uAlpha;


    bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmiHeader.biWidth = iWidth;
    bmiHeader.biHeight = iHeight;
    bmiHeader.biPlanes = 1;
    bmiHeader.biBitCount = 32;
    bmiHeader.biCompression = BI_RGB;
    bmiHeader.biSizeImage = 0;
    bmiHeader.biXPelsPerMeter = 0;
    bmiHeader.biYPelsPerMeter = 0;
    bmiHeader.biClrUsed = 0;
    bmiHeader.biClrImportant = 0;

    bmInfo.bmiHeader = bmiHeader;


    // figure out the dialog backgroud color to make alpha work
    uAlpha = GetSysColor(COLOR_MENU);
    if (GetSysColorBrush(COLOR_BTNFACE) != NULL)
    {
        // we must be under XP -- use btnface instead
        uAlpha = GetSysColor(COLOR_BTNFACE);
    }

    // replace transparent colors with appropriate background color
    bmiPixels = (unsigned char *)(pBitmap) ;
    for (height = 0; height < iHeight; ++height)
    {
        for (width = 0; width < iWidth; ++width)
        {
            if (bmiPixels[3] == 0)
            {
                bmiPixels[0] = (unsigned char)(uAlpha>>16);
                bmiPixels[1] = (unsigned char)(uAlpha>>8);
                bmiPixels[2] = (unsigned char)(uAlpha>>0);
            }
            bmiPixels += 4;
        }
    }

    hbm = CreateDIBitmap(GetDC(NULL), &bmiHeader, CBM_INIT, pBitmap, &bmInfo, DIB_RGB_COLORS);

    hbm = (HBITMAP)SendDlgItemMessage(pHandle, iItem, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
    DeleteObject(hbm);
}

/*F********************************************************************************/
/*!
    \Function WinMain

    \Description
        Main dialog handling loop. Actual work done by SampleStartup,
        SampleThread and SampleShutdown.

    \Input inst     - handle to current instance
    \Input prev     - handle to previous instance
    \Input *cmdline - command line
    \Input show     - show state
    
    \Output Zero

    \Version 03/13/2004 gschaefer
*/
/********************************************************************************F*/
int32_t APIENTRY WinMain(HINSTANCE inst, HINSTANCE prev, char *cmdline, int32_t show)
{
    MSG Msg;
    DWORD uPid;
    HWND pWindow;
    HANDLE pThread;
    char strConfig[256];

    static INITCOMMONCONTROLSEX _InitCommonCtrls =
    {
        // dwSize
        sizeof(INITCOMMONCONTROLSEX),
        // dwICC
        ICC_BAR_CLASSES |       // Load toolbar, status bar, trackbar, and ToolTip control classes.  
        ICC_LISTVIEW_CLASSES |  // Load list-view and header control classes.  
        ICC_PAGESCROLLER_CLASS |// Load pager control class.  
        ICC_TAB_CLASSES |       // Load tab and ToolTip control classes.  
        ICC_USEREX_CLASSES |    // Load ComboBoxEx class.  
        ICC_WIN95_CLASSES       // Load Win95 classes
    };

    // make common controls available
    LoadLibrary("RICHED32.DLL");
    InitCommonControlsEx(&_InitCommonCtrls);

    // save the dialog thread
    _DialogThread = GetCurrentThreadId();

    // setup default ini filename
    GetModuleFileName(NULL, strConfig, sizeof(strConfig));
    strcpy(strrchr(strConfig, '.'), ".ini");

    // startup the sample
    SampleStartup(cmdline);

    // startup the worker thread
    pThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&SampleThread, (LPVOID)strConfig, 0, &uPid);
    if (pThread != NULL)
    {
        CloseHandle(pThread);
    }

    // main message loop
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        // check for quit
        if (Msg.message == WM_USER_QUIT)
        {
            break;
        }

        // check for dialog create
        if (Msg.message == WM_USER_DIALOG)
        {
            DialogRef *pDialog = (DialogRef *)Msg.lParam;
            pDialog->pHandle = CreateDialogParam(GetModuleHandle(NULL), pDialog->strResource, HWND_DESKTOP, (DLGPROC)_DialogCallback, (LPARAM)pDialog);
            if (pDialog->pHandle == NULL)
            {
                int32_t iError = GetLastError();
                iError = iError;
            }
            SetForegroundWindow(pDialog->pHandle);
            pDialog->iCreated = 1;
        }

        // see if we own foreground window
        pWindow = GetForegroundWindow();
        GetWindowThreadProcessId(pWindow, &uPid);
        if (uPid != GetCurrentProcessId())
        {
            pWindow = NULL;
        }

        // special code to allow select-all in text boxes
        if ((pWindow != NULL) && (Msg.message == WM_CHAR) && (Msg.wParam == 1))
        {
            char strClass[32];
            HWND hEdit = GetFocus();
            GetClassName(hEdit, strClass, sizeof(strClass));
            if (strcmp(strClass, "Edit") == 0)
            {
                SendMessage(hEdit, EM_SETSEL, 0, -1);
                continue;
            }
        }

        // let dialog manager run
        if ((pWindow == NULL) || (!IsDialogMessage(pWindow, &Msg)))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    // shutdown the sample
    SampleShutdown();
    return(0);
}
