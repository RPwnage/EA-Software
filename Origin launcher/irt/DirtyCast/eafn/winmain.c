/*H********************************************************************************/
/*!
    \File winmain.cpp

    \Description
        Windows based main() handler. Implements a console with stdout/stderr support,
        pausing and simple signal simulation.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 12/18/2004 (gschaefer) Initial version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

/* nonstandard extension used : nameless struct / union
   no function prototype given : converting from () to (void) */
#pragma warning(push, 0)
#include <richedit.h>
#pragma warning(pop)

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "winmain.h"

int32_t main(int32_t argc, char **argv);

/*** Defines **********************************************************************/

#define IDM_SIGHUP      1010            // send a sighup
#define IDM_SIGTERM     1011            // send a sigterm

#define IDM_EXEC        1020            // execute main()

#define SIG_MIN         0               // first signal
#define SIG_MAX         31              // last signal

#define COLOR_BACKGND   0x00000000      // background is black
#define COLOR_STDOUT    0x00ffff00      // stdout is cyan
#define COLOR_STDERR    0x000000ff      // stderr is red
#define COLOR_SYSTEM    0x0000ffff      // system is yellow

#define PROG_QUERY      0               // query for startup parms
#define PROG_PARSE      1               // parse the startup parms
#define PROG_START      2               // start main() running
#define PROG_RUN        3               // main() is running
#define PROG_EXIT       4               // main() has completed

#ifndef DWL_DLGPROC
#define DWL_DLGPROC 4
#endif

#define WINDOW_MAX      (1024*1024)     // the maximum amount of bytes we display in the window

/*** Type Definitions *************************************************************/

typedef struct PrintDataT
{
    FILE *pStream;              // which stream to capture
    uint32_t uNotify;           // thread to notify
    HANDLE hShutdown;           // shutdown event
    HANDLE hEvent;              // completion event
    HANDLE hThread;             // capture thread handle
    uint32_t uColor;            // output color
    CRITICAL_SECTION *pMutual;  // mutual exclusion
    const char *pText;          // output text
    char strPipeName[256];      // name of the pipe
} PrintDataT;

typedef struct ProgramStateT
{
    int32_t iState;             // program run state
    HANDLE hMainThread;         // main() thread
    int32_t iArgCount;          // argument count
    char *pArgParm[32];         // pointers into buffer
    char strArgData[2048];      // argument buffer
} ProgramStateT;

/*** Variables ********************************************************************/

// signal handler table
static WinSignalHandlerT *_SignalTable[SIG_MAX-SIG_MIN+1] = { 0 };

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ReadStream

    \Description
        Create a FILE* pipe and process the input in real-time. The pipe contents
        are changed into Windoze display text (CR/LF formatting) and passed to the
        main thread for display. The main thread is woken up via a PostThreadMessage
        call and this thread blocks for confirmation before continuing.

    \Input pPrint - Contains the print state for this channel

    \Output None

    \Version 12/18/2004 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ReadStream(PrintDataT *pPrint)
{
    int32_t iIndex;
    char *pFormat;
    HANDLE hPipe;
    int32_t iRead, iRead2;
    char strRead[4096];
    char strFormat[sizeof(strRead)*2+4];

    // create a new named pipe and redirect the stream's data to it
    hPipe = CreateNamedPipe(pPrint->strPipeName, PIPE_ACCESS_DUPLEX | PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, PIPE_WAIT, 1, 8192, 8192, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    pPrint->pStream = freopen(pPrint->strPipeName, "w", pPrint->pStream); /* save the new file so we can close later */

    // let caller know we are initialized
    SetEvent(pPrint->hEvent);

    // keep reading data and waking up / informing main thread
    while (WaitForSingleObject(pPrint->hShutdown, 1) == WAIT_TIMEOUT)
    {
        /* let us peek the pipe to see if we have any data waiting.
           readfile blocks so we want to make sure not be but stuck blocking and a signal might be waiting for us to shutdown */
        if (PeekNamedPipe(hPipe, strRead, sizeof(strRead), (LPDWORD)&iRead, NULL, NULL) == FALSE || iRead <= 0)
        {
            continue;
        }

        // at this point we know we have data so do the normal work
        if (ReadFile(hPipe, strRead, sizeof(strRead), (LPDWORD)&iRead, NULL))
        {
            if (iRead > 0)
            {
                // enforce mutual exclusion between stdout/stderr processors
                EnterCriticalSection(pPrint->pMutual);
                // if it did not end on a newline, sleep and look for more
                if ((strRead[iRead - 1] != '\n') && (iRead < sizeof(strRead)))
                {
                    // wait for more data to come in
                    Sleep(0);
                    if (PeekNamedPipe(hPipe, strRead + iRead, sizeof(strRead) - iRead, (LPDWORD)&iRead2, NULL, NULL) && (iRead2 > 0))
                    {
                        // scan backwards for newline character
                        while ((iRead2 > 0) && (strRead[iRead + iRead2 - 1] != '\n'))
                        {
                            --iRead2;
                        }
                        // read rest of line
                        if (iRead2 > 0)
                        {
                            ReadFile(hPipe, strRead + iRead, iRead2, (LPDWORD)&iRead2, NULL);
                            iRead += iRead2;
                        }
                    }
                }

                // format data into buffer (fix endlines)
                for (pFormat = strFormat, iIndex = 0; iIndex < iRead; ++iIndex)
                {
                    char iChar = strRead[iIndex];
                    if (iChar != '\r')
                    {
                        if (iChar == '\n')
                        {
                            *pFormat++ = '\r';
                        }
                        *pFormat++ = iChar;
                    }
                }
                *pFormat = 0;

                // point to text
                pPrint->pText = strFormat;
                // wake up the main thread
                PostThreadMessage(pPrint->uNotify, WM_APP, 0, 0);
                // let other thread run
                LeaveCriticalSection(pPrint->pMutual);
                // wait for main thread to consume data
                WaitForSingleObject(pPrint->hEvent, INFINITE);
            }
        }
    }

    // close the read handle (write was closed externally)
    CloseHandle(hPipe);
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _WindowProc

    \Description
        Basic window message processing.

    \Input hWnd  - The window handle
    \Input uMsg  - The message identifier
    \Input wparm - The windows-specific wparm parameter (context specific)
    \Input lparm - The windows-specific lparm parameter (context specific)

    \Output int32_t - Indicates whether Windows should do further processing

    \Version 12/18/2004 (gschaefer)
*/
/********************************************************************************F*/
LRESULT CALLBACK _WindowProc(HWND hWnd, uint32_t uMsg, WPARAM wparm, LPARAM lparm)
{
    if (uMsg == WM_CLOSE)
    {
        PostQuitMessage(0);
        return(DefWindowProc(hWnd, uMsg, wparm, lparm));
    }
    else if (uMsg == WM_SIZE)
    {
        HWND hEdit = FindWindowEx(hWnd, NULL, RICHEDIT_CLASS, NULL);
        int32_t iWidth = LOWORD(lparm);
        int32_t iHeight = HIWORD(lparm);
        MoveWindow(hEdit, 0, 0, iWidth, iHeight, TRUE);
    }

    // let windows handle
    return(DefWindowProc(hWnd, uMsg, wparm, lparm));
}
/*F********************************************************************************/
/*!
    \Function _DialogText

    \Description
        Display text into the output buffer. Handles deleting old text if buffer
        gets too large. This version maintains between 16-24K of output buffer.
        This under 32K limit is leftover from the original edit control version
        and can be eliminated by using the extended selection size functions.

    \Input  hEdit   - The dialog control handle
    \Input *pFormat - Pointer to character style record
    \Input *pText   - Pointer to the output text

    \Output None

    \Version 12/18/2004 (gschaefer)
*/
/********************************************************************************F*/
static void _DialogText(HWND hEdit, CHARFORMAT *pFormat, const char *pText)
{
    int32_t iLen;

    // see if we need to delete old data
    if (SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0) > WINDOW_MAX)
    {
        SendMessage(hEdit, EM_SETSEL, 0, 8192);
        SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)"");
        iLen = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
        SendMessage(hEdit, EM_SETSEL, iLen-1, iLen);
        SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)"");
    }

    // add new text
    iLen = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(hEdit, EM_SETSEL, iLen, iLen);
    SendMessage(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)pFormat);
    SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)pText);
}

/*F********************************************************************************/
/*!
    \Function _ParseCmdLine

    \Description
        Parse a command line into a main() compatible argc/argv structure. Quoting
        is supported with ' and ".

    \Input *pArgList - The argument list to populate
    \Input  iArgLimt - Size of argument list
    \Input *pBuffer  - Buffer to store actual argv data in
    \Input  iLength  - Length of buffer
    \Input *pCmdLine - The raw command line

    \Output int32_t - Count of arguments

    \Version 12/18/2004 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ParseCmdLine(char **pArgList, int32_t iArgLimit, char *pBuffer, int32_t iLength, const char *pCmdLine)
{
    int32_t iTerm;
    char *pParse;
    int32_t iArgCount = 0;

    // setup argv[0] with application name
    GetModuleFileName(NULL, pBuffer, iLength);
    pArgList[iArgCount++] = pBuffer;
    iLength -= (int32_t)strlen(pBuffer)+1;
    pBuffer += strlen(pBuffer)+1;

    // copy over command line
    ds_strnzcpy(pBuffer, (char *)pCmdLine, iLength);

    // parse the command line
    for (pParse = pBuffer; iArgCount+1 < iArgLimit; ++iArgCount)
    {
        // skip to next token
        while ((*pParse != 0) && (*pParse <= ' '))
        {
            ++pParse;
        }
        // see if we are done
        if (*pParse == 0)
        {
            break;
        }
        // handle quoted string
        if ((*pParse == '"') || (*pParse == '\''))
        {
            iTerm = *pParse++;
            pArgList[iArgCount] = pParse;
            for (; (*pParse != 0) && (*pParse != iTerm); ++pParse)
                ;
        }
        else
        {
            // allow quoted strings within argument
            pArgList[iArgCount] = pParse;
            for (iTerm = 0; (*pParse != 0) && ((iTerm != 0) || (*pParse > ' ')); ++pParse)
            {
                if ((*pParse == '"') || (*pParse == '\''))
                {
                    if (iTerm == 0)
                        iTerm = *pParse;
                    else if (iTerm == *pParse)
                        iTerm = 0;
                }
            }
        }

        // terminate token
        if (*pParse != 0)
        {
            *pParse++ = 0;
        }
    }

    // terminate the arglist
    pArgList[iArgCount] = NULL;
    return(iArgCount);
}

/*F********************************************************************************/
/*!
    \Function _MainThread

    \Description
        Launch the main() function in its own thread.

    \Input pState_ - The program state structure

    \Output int32_t - Result code from main() function

    \Version 12/18/2004 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _MainThread(void *pState_)
{
    int32_t iResult;
    ProgramStateT *pState = (ProgramStateT *)pState_;

    // run the main thread
    iResult = main(pState->iArgCount, pState->pArgParm);
    // need to sleep before exit to let printf queues empty
    Sleep(20);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProgramControl

    \Description
        Simple state machine to handle program control.

    \Input *pState_  - The program state structure
    \Input  iAction  - Optional user action (IDM_message)
    \Input  hEdit    - The richedit handle
    \Input *pFormat  - The character format structure
    \Input *pCmdLine - Command line from application startup

    \Output None

    \Version 12/22/2004 (gschaefer)
*/
/********************************************************************************F*/
static void _ProgramControl(ProgramStateT *pState, int32_t iAction, HWND hEdit, CHARFORMAT *pFormat, const char *pCmdLine)
{
    int32_t iIndex;
    int32_t iResult;
    int32_t iSel1, iSel2;
    uint32_t uPid;
    char strBuffer[1024];

    // setup for command line input
    if (pState->iState == PROG_QUERY)
    {
        // if application parms start with "~" then bypass query
        if (pCmdLine[0] == '~')
        {
            pCmdLine += 1;
            pState->iState = PROG_START;
        }
        else
        {
            // query user for parms
            pFormat->crTextColor = COLOR_SYSTEM;
            sprintf(strBuffer, "Enter command line arguments:\n");
            _DialogText(hEdit, pFormat, strBuffer);

            iSel1 = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
            pFormat->crTextColor = COLOR_STDOUT;
            _DialogText(hEdit, pFormat, pCmdLine);
            iSel2 = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0);
            SendMessage(hEdit, EM_SETSEL, iSel1-1, iSel2-1);

            SendMessage(hEdit, EM_SETREADONLY, FALSE, 0);
            SendMessage(hEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)pFormat);
            pState->iState += 1;
        }
    }

    // get command line input
    if ((pState->iState == PROG_PARSE) && (iAction == IDM_EXEC))
    {
        iSel1 = SendMessage(hEdit, EM_LINEFROMCHAR, (WPARAM)-1, (LPARAM)0);
        strBuffer[0] = (char)(sizeof(strBuffer)&0xff);
        strBuffer[1] = (char)(sizeof(strBuffer)>>8);
        iSel2 = SendMessage(hEdit, EM_GETLINE, (WPARAM)iSel1, (LPARAM)strBuffer);
        if (iSel2 > 0)
            strBuffer[iSel2] = 0;
        SendMessage(hEdit, EM_SETREADONLY, TRUE, 0);
        _DialogText(hEdit, pFormat, "\r\n");
        pCmdLine = strBuffer;
        pState->iState += 1;
    }

    // launch main()
    if (pState->iState == PROG_START)
    {
        pState->iArgCount = _ParseCmdLine(pState->pArgParm, sizeof(pState->pArgParm)/sizeof(pState->pArgParm[0]), pState->strArgData, sizeof(pState->strArgData), pCmdLine);
        pFormat->crTextColor = COLOR_SYSTEM;
        sprintf(strBuffer, "main(%d, [0]=%s", pState->iArgCount, pState->pArgParm[0]);
        _DialogText(hEdit, pFormat, strBuffer);
        for (iIndex = 1; pState->pArgParm[iIndex] != NULL; ++iIndex)
        {
            sprintf(strBuffer, " [%d]=%s", iIndex, pState->pArgParm[iIndex]);
            _DialogText(hEdit, pFormat, strBuffer);
        }
        _DialogText(hEdit, pFormat, ")\r\n");
        pState->hMainThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_MainThread, (LPVOID)pState, 0, (LPDWORD)&uPid);
        pState->iState += 1;
    }

    // check for thread completion
    if (pState->iState == PROG_RUN)
    {
        // see if main thread has completed
        if (WaitForSingleObject(pState->hMainThread, 0) == WAIT_OBJECT_0)
        {
            GetExitCodeThread(pState->hMainThread, (LPDWORD)&iResult);
            // show exit code in title bar
            GetModuleFileName(NULL, strBuffer, sizeof(strBuffer));
            sprintf(strBuffer, "%s - exit(%d)", strrchr(strBuffer, '\\')+1, iResult);
            SetWindowText(GetWindow(hEdit, GW_OWNER), strBuffer);
            // show admin message with exit code
            sprintf(strBuffer, "exit(%d)\r\n", iResult);
            pFormat->crTextColor = COLOR_SYSTEM;
            _DialogText(hEdit, pFormat, strBuffer);
            CloseHandle(pState->hMainThread);
            pState->iState += 1;
            //the next state does nothing, lets close the program
            exit(iResult);
        }
    }

    // handle sighup
    if (iAction == IDM_SIGHUP)
    {
        pFormat->crTextColor = COLOR_SYSTEM;
        if (pState->iState != PROG_RUN)
        {
            _DialogText(hEdit, pFormat, "***NOT RUNNING***\r\n");
        }
        else
        {
            WinSignalHandlerT *pHandler = _SignalTable[SIGHUP-SIG_MIN];
            _DialogText(hEdit, pFormat, "***SIGHUP***\r\n");
            if (pHandler > (WinSignalHandlerT*)256)
            {
                // reset to default behavior
                _SignalTable[SIGHUP-SIG_MIN] = SIG_DFL;
                // call the signal handler
                (pHandler)(SIGHUP);
            }
        }
    }

    // handle sigterm
    if (iAction == IDM_SIGTERM)
    {
        pFormat->crTextColor = COLOR_SYSTEM;
        if (pState->iState != PROG_RUN)
        {
            _DialogText(hEdit, pFormat, "***NOT RUNNING***\r\n");
        }
        else
        {
            WinSignalHandlerT *pHandler = _SignalTable[SIGTERM-SIG_MIN];
            _DialogText(hEdit, pFormat, "***SIGTERM***\r\n");
            if (pHandler == SIG_IGN)
            {
                // ignore
            }
            else if (pHandler == SIG_DFL)
            {
                TerminateThread(pState->hMainThread, (DWORD)-999);
            }
            else if (pHandler > (WinSignalHandlerT*)256)
            {
                // reset to default behavior
                _SignalTable[SIGTERM-SIG_MIN] = SIG_DFL;
                // call the signal handler
                (pHandler)(SIGTERM);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function WinMain

    \Description
        Main application message loop. Contains a lot of logic, much of which could
        be broken out into individual functions. When first implementing, it was
        unclear what could go where so it ended up all together.

    \Input  hCurrInst - Current application instance (Windows deprecated)
    \Input  hPrevInst - Previous application instance (Windows deprecated)
    \Input *pCmdLine  - Startup command line
    \Input  iShow     - Initial window state

    \Output int32_t - Application exit value

    \Version 12/18/2004 (gschaefer)
*/
/********************************************************************************F*/
int32_t APIENTRY WinMain(HINSTANCE hCurrInst, HINSTANCE hPrevInst, char *pCmdLine, int32_t iShow)
{
    MSG Msg;
    RECT Rect;
    int32_t iPause;
    HWND hWin, hEdit;
    uint32_t uPid;
    char strBuffer[256];
    CHARFORMAT CharFormat;
    CRITICAL_SECTION Critical;
    PrintDataT StdOut, StdErr;
    ProgramStateT ProgState;
    HMENU hMenu;

    // make richedit available
    LoadLibrary("RICHED20.DLL");

    // create console dialog
    hWin = CreateWindow("#32770", "Console", DS_MODALFRAME|WS_MINIMIZEBOX|WS_SIZEBOX|WS_POPUP|WS_VISIBLE|WS_CAPTION|WS_SYSMENU|WS_MAXIMIZEBOX,
        5, 725, 800, 200, HWND_DESKTOP, NULL, GetModuleHandle(NULL), 0);
    SetWindowLongPtr(hWin, GWLP_WNDPROC, (LPARAM)_WindowProc);
    // add a richedit textbox which uses the entire client area
    GetClientRect(hWin, &Rect);
    hEdit = CreateWindow(RICHEDIT_CLASS, "", ES_READONLY|ES_NOHIDESEL|ES_MULTILINE|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE|WS_CHILD,
        Rect.left, Rect.top, Rect.right, Rect.bottom, hWin, NULL, GetModuleHandle(NULL), 0);
    SendMessage(hEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)COLOR_BACKGND);
    SetFocus(hEdit);
    SetTimer(hWin, 100, 100, NULL);

    // add special menu items
    hMenu = GetSystemMenu(hWin, FALSE);
    AppendMenu(hMenu, MF_SEPARATOR, 0, "");
    AppendMenu(hMenu, MF_STRING, IDM_SIGHUP, "Send SIGHUP\tF5");
    AppendMenu(hMenu, MF_STRING, IDM_SIGTERM, "Send SIGTERM\tF6");

    // set working directory to location of .exe
    GetModuleFileName(NULL, strBuffer, sizeof(strBuffer));
    if (strchr(strBuffer, '\\') != NULL)
    {
        SetWindowText(hWin, strrchr(strBuffer, '\\')+1);
        (strrchr(strBuffer, '\\')+1)[0] = 0;
        SetCurrentDirectory(strBuffer);
    }

    // setup output attributes
    ds_memclr(&CharFormat, sizeof(CharFormat));
    CharFormat.cbSize = sizeof(CharFormat);
    CharFormat.dwMask = CFM_SIZE|CFM_COLOR|CFM_FACE;
    CharFormat.yHeight = 200;
    CharFormat.crTextColor = 0x000000ff;
    CharFormat.bPitchAndFamily = FF_DONTCARE|FIXED_PITCH;
    strcpy(CharFormat.szFaceName, "Courier New");

    // setup critical section to synchronous output threads
    InitializeCriticalSection(&Critical);

    // setup "stdout" thread (display in cyan)
    ds_memclr(&StdOut, sizeof(StdOut));
    StdOut.pMutual = &Critical;
    StdOut.uNotify = GetCurrentThreadId();
    StdOut.hEvent = CreateEvent(NULL, FALSE, 0, NULL);
    StdOut.hShutdown = CreateEvent(NULL, FALSE, 0, NULL);
    StdOut.uColor = COLOR_STDOUT;
    StdOut.pStream = stdout;
    // use rand to make sure we don't get collisions when we have other instances running
    ds_snzprintf(StdOut.strPipeName, sizeof(StdOut.strPipeName), "\\\\.\\pipe\\StdOut%08x", NetRand(0xffffffff));
    StdOut.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_ReadStream, (LPVOID)&StdOut, 0, (LPDWORD)&uPid);
    WaitForSingleObject(StdOut.hEvent, INFINITE);

    // setup "stderr" thread (display in red)
    ds_memclr(&StdErr, sizeof(StdErr));
    StdErr.pMutual = &Critical;
    StdErr.uNotify = GetCurrentThreadId();
    StdErr.hEvent = CreateEvent(NULL, FALSE, 0, NULL);
    StdErr.hShutdown = CreateEvent(NULL, FALSE, 0, NULL);
    StdErr.uColor = COLOR_STDERR;
    StdErr.pStream = stderr;
    // use rand to make sure we don't get collisions when we have other instances running
    ds_snzprintf(StdErr.strPipeName, sizeof(StdErr.strPipeName), "\\\\.\\pipe\\StdErr%08x", NetRand(0xffffffff));
    StdErr.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_ReadStream, (LPVOID)&StdErr, 0, (LPDWORD)&uPid);
    WaitForSingleObject(StdErr.hEvent, INFINITE);

    // set for "search scroll lock off"
    iPause = ((GetKeyState(VK_SCROLL)&1) ? -1 : 0);

    // reset program state
    ds_memclr(&ProgState, sizeof (ProgState));

    // main message loop
    while (GetMessage(&Msg, NULL, 0, 0))
    {
        // run program handler
        _ProgramControl(&ProgState, 0, hEdit, &CharFormat, pCmdLine);

        // check for enter key
        if ((Msg.message == WM_KEYDOWN) && (Msg.wParam == VK_RETURN))
        {
            _ProgramControl(&ProgState, IDM_EXEC, hEdit, &CharFormat, pCmdLine);
        }

        // handle pause key (does not enable until scroll lock is release)
        if (Msg.message == WM_KEYDOWN)
        {
            int32_t iScroll = GetKeyState(VK_SCROLL) & 1;
            if ((iPause < 0) && (iScroll == 0))
            {
                iPause = 0;
            }
            if ((iPause == 0) && (iScroll == 1))
            {
                EnterCriticalSection(&Critical);
                iPause = 1;
            }
            if ((iPause == 1) && (iScroll == 0))
            {
                LeaveCriticalSection(&Critical);
                iPause = 0;
            }
        }

        // check for sighup
        if (((Msg.message == WM_SYSCOMMAND) && (Msg.wParam == IDM_SIGHUP)) ||
            ((Msg.message == WM_KEYDOWN) && (Msg.wParam == VK_F5)))
        {
            _ProgramControl(&ProgState, IDM_SIGHUP, hEdit, &CharFormat, NULL);
        }

        // check for sigterm
        if (((Msg.message == WM_SYSCOMMAND) && (Msg.wParam == IDM_SIGTERM)) ||
            ((Msg.message == WM_KEYDOWN) && (Msg.wParam == VK_F6)))
        {
            _ProgramControl(&ProgState, IDM_SIGTERM, hEdit, &CharFormat, NULL);
        }

        // check for pending output and process
        EnterCriticalSection(&Critical);
        if (StdOut.pText != NULL)
        {
            CharFormat.crTextColor = StdOut.uColor;
            _DialogText(hEdit, &CharFormat, StdOut.pText);
            StdOut.pText = NULL;
            SetEvent(StdOut.hEvent);
        }
        if (StdErr.pText != NULL)
        {
            CharFormat.crTextColor = StdErr.uColor;
            _DialogText(hEdit, &CharFormat, StdErr.pText);
            StdErr.pText = NULL;
            SetEvent(StdErr.hEvent);
        }
        LeaveCriticalSection(&Critical);

        // dispatch window messages to dialog manager
        GetWindowThreadProcessId(GetForegroundWindow(), (LPDWORD)&uPid);
        if (uPid != GetCurrentProcessId())
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
        else if (!IsDialogMessage(hWin, &Msg))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    // allow shutdown of print threads
    SignalObjectAndWait(StdOut.hShutdown, StdOut.hThread, INFINITE, TRUE);
    fclose(StdOut.pStream);
    CloseHandle(StdOut.hThread);
    CloseHandle(StdOut.hShutdown);
    CloseHandle(StdOut.hEvent);
    SignalObjectAndWait(StdErr.hShutdown, StdErr.hThread, INFINITE, TRUE);
    fclose(StdErr.pStream);
    CloseHandle(StdErr.hThread);
    CloseHandle(StdErr.hShutdown);
    CloseHandle(StdErr.hEvent);
    Sleep(5);
    DeleteCriticalSection(&Critical);

    // done with dialog
    DestroyWindow(hWin);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function WinSignal

    \Description
        Simulated signal handler for Windows. Allows *nix style signal handling
        for windows applications. Applications must #include winmain.h in order
        for macros to remap signal() to WinSignal().

    \Input  iSigNum  - Which signal handler to set
    \Input *pSigProc - New signal handler

    \Output WinSignalHandlerT* - Returns previous signal handler

    \Version 12/18/2004 (gschaefer)
*/
/********************************************************************************F*/
WinSignalHandlerT *WinSignal(int32_t iSigNum, WinSignalHandlerT *pSigProc)
{
    WinSignalHandlerT *pOldHandler = NULL;

    // add the handler to the signal table
    if ((iSigNum >= SIG_MIN) && (iSigNum <= SIG_MAX))
    {
        pOldHandler = _SignalTable[iSigNum-SIG_MIN];
        _SignalTable[iSigNum-SIG_MIN] = pSigProc;
    }

    return(pOldHandler);
}
