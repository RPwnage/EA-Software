/*H********************************************************************************/
/*!
    \File mbtest.c

    \Description
        Tester routine for displaying a dialog box on PS3

    \Copyright
        Copyright (c) 2003-2005 Electronic Arts Inc.

    \Version 05/24/2006 (tdills) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/memory.h>
#include <sysutil/sysutil_msgdialog.h>
#include <sysutil/sysutil_oskdialog.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "zlib.h"

#include "testersubcmd.h"
#include "testermodules.h"

/*** Defines **********************************************************************/

typedef struct MbTestAppT
{
    int32_t                 bIsKeyboardOn; //!< is the onscreen keyboard currently displayed?
    int32_t                 bZCallback;    //!< have we initialized the period callback?
} MbTestAppT;

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

static void _MbTestBox(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);
static void _MbTestKBrd(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables ********************************************************************/

static T2SubCmdT _MbTest_Commands[] =
{
    { "box",            _MbTestBox     },
    { "kbrd",           _MbTestKBrd    },
};

static MbTestAppT _MbTest_App = { 0, 0 };

// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function   _MbTestMessageBoxCallback
    
    \Description
        Callback function for receiving message box input.
    
    \Input iButtonType - the button the user pressed
    \Input *pUserData  - the pointer provided to cellMsgDialogOpen
    
    \Output void
            
    \Version 05/25/2006 (tdills) First Version
*/
/********************************************************************************F*/
static void _MbTestMessageBoxCallback(int32_t iButtonType, void *pUserData)
{
    ZPrintf("MbTest: iButtonType = %d\n", iButtonType);
    switch (iButtonType)
    {
    case CELL_MSGDIALOG_BUTTON_YES:
        ZPrintf("MbTest: Dialog pressed Yes\n");
        break;
    case CELL_MSGDIALOG_BUTTON_NO:
        ZPrintf("MbTest: Dialog pressed No\n");
        break;
    }
}

/*F********************************************************************************/
/*!
    \Function   _MbTestKeyboardCallback
    
    \Description
        Callback function for receiving message box input.
    
    \Input uStatus   - status of the onscreen dialog
    \Input uParam    - not used
    \Input pUserData - not used
    
    \Output void
            
    \Version 05/26/2006 (tdills) First Version
*/
/********************************************************************************F*/
static void _MbTestKeyboardCallback(uint64_t uStatus, uint64_t uParam, void* pUserData)
{
	static uint16_t strResultTextBuffer[CELL_OSKDIALOG_STRING_SIZE];

    static CellOskDialogCallbackReturnParam OutputInfo;

    OutputInfo.result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;
    OutputInfo.numCharsResultString = CELL_OSKDIALOG_STRING_SIZE;
    OutputInfo.pResultString = strResultTextBuffer;

    switch (uStatus)
    {
    case CELL_SYSUTIL_OSKDIALOG_LOADED:
        break;
    case CELL_SYSUTIL_OSKDIALOG_FINISHED:
        {
            cellOskDialogUnloadAsync(&OutputInfo);

            ZPrintf("MbTest: Keyboard Status: %d\n", OutputInfo.result);
            ZPrintf("MbTest: Keyboard Input:  %s\n", OutputInfo.pResultString);
        }
        break;
    case CELL_SYSUTIL_OSKDIALOG_UNLOADED:
        break;
    }
}

/*F********************************************************************************/
/*!
    \Function   _MbTestBox
    
    \Description
        Message-box tester
    
    \Input *pCmdRef - pointer to module reference (NULL)
    \Input argc     - number of command arguments
    \Input **argv   - array of command arguments
    \Input bHelp    - should we display help
    
    \Output void
            
    \Version 05/26/2006 (tdills) First Version
*/
/********************************************************************************F*/
static void _MbTestBox(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
    char strText[256];
    uint32_t uFlags = 0;

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s %s [hide buttons] [error] [display text]\n", argv[0], argv[1]);
        ZPrintf("          hide buttons = 1 if you want to hide yes/no buttons\n");
        ZPrintf("          error        = 1 if you want the dialog to be an error dialog\n");
        return;
    }

    if (argc > 2)
    {
        int32_t iHideButtons = atoi(argv[2]);
        if (iHideButtons)
        {
            uFlags = CELL_MSGDIALOG_BUTTON_TYPE_NONE;
        }
        else
        {
            uFlags = CELL_MSGDIALOG_BUTTON_TYPE_YESNO | CELL_MSGDIALOG_DEFAULT_CURSOR_YES;
        }
    }

    if (argc > 3)
    {
        int32_t iError = atoi(argv[3]);
        if (iError)
        {
            uFlags |= CELL_MSGDIALOG_DIALOG_TYPE_ERROR;
        }
        else
        {
            uFlags |= CELL_MSGDIALOG_DIALOG_TYPE_NORMAL;
        }
    }

    sprintf(strText, "Test Text. The documentation says we can make this 8 lines long.\n Can we put carriage\n returns in it?");
    if (argc > 4)
    {
        int32_t iWord;

        strcpy(strText, argv[4]);
        for (iWord = 5; iWord < argc; iWord++)
        {
            strcat(strText, argv[iWord]);
        }
    }

    _MbTest_App.bIsKeyboardOn = TRUE;

    cellMsgDialogOpen(uFlags, strText, _MbTestMessageBoxCallback, NULL, NULL);
}

/*F********************************************************************************/
/*!
    \Function   _MbTestKBrd
    
    \Description
        Onscreen keyboard tester
    
    \Input *pCmdRef - pointer to module reference (NULL)
    \Input argc     - number of command arguments
    \Input **argv   - array of command arguments
    \Input bHelp    - should we display help
    
    \Output void
            
    \Version 05/26/2006 (tdills) First Version
*/
/********************************************************************************F*/
static void _MbTestKBrd(void *pCmdRef, int32_t argc, char **argv, unsigned char bHelp)
{
	static uint16_t strMessageBuffer[CELL_OSKDIALOG_STRING_SIZE];
    static uint16_t strInitTextBuffer[CELL_OSKDIALOG_STRING_SIZE];
    static CellOskDialogInputFieldInfo InputFieldInfo;
	static CellOskDialogParam DialogParam;
    static CellOskDialogPoint Position ={
		0.0,
		0.0
	};
    sys_memory_container_t  iMemoryContainer;
    int32_t iResult;
    
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s %s [display text]\n", argv[0], argv[1]);
        return;
    }

    DialogParam.allowOskPanelFlg = CELL_OSKDIALOG_PANELMODE_ALPHABET      |
        //CELL_OSKDIALOG_PANELMODE_ALPHABET_HALF |
        CELL_OSKDIALOG_PANELMODE_NUMERAL       |
        //CELL_OSKDIALOG_PANELMODE_NUMERAL_HALF  |
        CELL_OSKDIALOG_PANELMODE_JAPANESE      |
        CELL_OSKDIALOG_PANELMODE_JAPANESE_HIRAGANA |
        CELL_OSKDIALOG_PANELMODE_JAPANESE_KATAKANA |
        CELL_OSKDIALOG_PANELMODE_ENGLISH;

    DialogParam.firstViewPanel = CELL_OSKDIALOG_PANELMODE_ALPHABET;//_HALF;
    DialogParam.controlPoint = Position;
    DialogParam.prohibitFlgs = 0;

	ds_strnzcpy((char*)strMessageBuffer, "This is the message buffer", sizeof(strMessageBuffer));
    if (argc > 2)
    {
        int32_t iWord;

        ds_strnzcpy((char*)strMessageBuffer, argv[2], sizeof(strMessageBuffer));
        for (iWord = 3; iWord < argc; iWord++)
        {
            ds_strnzcat((char*)strMessageBuffer, argv[iWord], sizeof(strMessageBuffer));
        }
    }

	ds_strnzcpy((char*)strInitTextBuffer, "This is the initial text", sizeof(strInitTextBuffer));

	InputFieldInfo.message = strMessageBuffer;
    InputFieldInfo.init_text = strInitTextBuffer;
    InputFieldInfo.limit_length = CELL_OSKDIALOG_STRING_SIZE;

	if (0 != cellSysutilRegisterCallback( 0, _MbTestKeyboardCallback, NULL ))
    {
        ZPrintf("cellSysutilRegisterCallback failed\n");
    }

    // create a "memory container" which must be "at least 7mb (!)" according to the OSK docs
    if ((iResult = sys_memory_container_create(&iMemoryContainer, 8*1024*1024)) != CELL_OK)
    {
        NetPrintf(("mbtest: sys_memory_container_create() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return;
    }

    cellOskDialogLoadAsync(iMemoryContainer, &DialogParam, &InputFieldInfo);
}

/*F*************************************************************************************/
/*!
    \Function _CmdMbTestCb
    
    \Description
        MbTest idle callback.
    
    \Input *argz    - unused
    \Input argc     - argument count
    \Input *argv[]  - argument list
    
    \Output
        int32_t         - zero
            
    \Version 1.0 05/26/2006 (tdills) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdMbTestCb(ZContext *argz, int32_t argc, char *argv[])
{
    if (_MbTest_App.bIsKeyboardOn)
    {
        cellSysutilCheckCallback();
    }

    // update
    return(ZCallback(_CmdMbTestCb, 16));
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function   CmdMbTest
    
    \Description
        Native UI test harness
    
    \Input *argz    -
    \Input argc     -
    \Input **argv   -
    
    \Output
        int32_t     -
            
    \Version 05/25/2006 (tdills) First Version
*/
/********************************************************************************F*/
int32_t CmdMbTest(ZContext *argz, int32_t argc, char **argv)
{
    T2SubCmdT *pCmd;
    unsigned char bHelp;

    // handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_MbTest_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   %s: test native UI functions on PS3\n", argv[0]);
        T2SubCmdUsage(argv[0], _MbTest_Commands);
        return(0);
    }

    // hand off to command
    pCmd->pFunc(NULL, argc, argv, bHelp);


        // one-time install of periodic callback
    if (_MbTest_App.bZCallback == FALSE)
    {
        _MbTest_App.bZCallback = TRUE;
        return(ZCallback(_CmdMbTestCb, 16));
    }
    else
    {
        return(0);
    }
}
