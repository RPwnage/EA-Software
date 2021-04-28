/*H********************************************************************************/
/*!
    \File T2Host.cpp

    \Description
        Main file for Tester2 Host Application.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/21/2005 (jfrank) First Version
    \Version 08/05/2005 (jfrank) Moved to ATG sample shell
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

// system includes
#include <xtl.h>
#include <xbdm.h>
#include <d3d9.h>
#include <xbox.h>
#include <xonline.h>
#include <xam.h>

#include <stdlib.h>
#include <stdio.h>
#include <crtdbg.h>
#include <stdarg.h>

// atg framework includes
#include "AtgApp.h"
#include "AtgFont.h"
#include "AtgHelp.h"
#include "AtgInput.h"
#include "AtgResource.h"
#include "AtgUtil.h"

// dirtysock includes
#include "dirtysock.h"
#include "netconn.h"
#include "lobbytagfield.h"

#include "zlib.h"
#include "zmemtrack.h"

#include "testerhostcore.h"
#include "testerregistry.h"
#include "testercomm.h"

#define XN_SECURE               (TRUE)

/*** Defines **********************************************************************/

#define INFO_COLOR          0xffffff00          // information display color
#define SEP_COLOR           0xffffffff          // split screen separator color
#define PRESENCE_COLOR      0xff00ffff          // presence string color

#define TOP_BACK_COLOR      0xff00ccbb          // background gradient colors
#define BOTTOM_BACK_COLOR   0xff003322

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

class CT2HostSample : public ATG::Application
{
    public:
        virtual ~CT2HostSample();

    private:
        ATG::Font               m_Font;         // font for displaying text
        HANDLE                  m_Listener;     // listener for xdk notifications
        LPCWSTR                 m_LiveStatus;   // xbox live status string
        wchar_t                 m_users[4*(XUSER_NAME_SIZE+3)];// list of users online, 3 is padding for commas, etc...

        void                    Process_Live_ConnectionChanged( ULONG_PTR ulParam );
        void                    Process_Sys_SignInChanged( );

    private:
        virtual HRESULT Initialize();
        virtual HRESULT Update();
        virtual HRESULT Render();
};

TesterHostCoreT *g_pHostCore;   //!< global host core pointer

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _T2HostCmdExit

    \Description
        Quit

    \Input  *argz   - environment
    \Input   argc   - num args
    \Input **argv   - arg list
    
    \Output  int32_t    - standard return code

    \Version 05/03/2005 (jfrank)
*/
/********************************************************************************F*/
static int32_t _T2HostCmdExit(ZContext *argz, int32_t argc, char **argv)
{
    if (argc >= 1)
    {
        DmReboot(DMBOOT_TITLE);
    }
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _T2HostRegisterModules

    \Description
        Register client commands (local commands, like exit, history, etc.)

    \Input None
    
    \Output None

    \Version 05/03/2005 (jfrank)
*/
/********************************************************************************F*/
static void _T2HostRegisterModules(void)
{
    TesterHostCoreRegister(g_pHostCore, "exit",     &_T2HostCmdExit);
}


/*F********************************************************************************/
/*!
    \Function Initialize

    \Description
        Startup procedures for the ATG sample shell

    \Input  None
    
    \Output None

    \Version 08/05/2005 (jfrank)
*/
/********************************************************************************F*/
HRESULT CT2HostSample::Initialize()
{
    char strParams[512];

    ZPrintf("\nStarting T2Host.\n\n");

    // Create the font
    if( FAILED( m_Font.Create( "d:\\Media\\Fonts\\Arial_16.xpr" ) ) )
        return ATGAPPERR_MEDIANOTFOUND;
    // Confine text drawing to the title safe area
    m_Font.SetWindow( ATG::GetTitleSafeArea() );

    ZMemtrackStartup();

    // create the module
    TagFieldClear(strParams, sizeof(strParams));
    TagFieldSetNumber(strParams, sizeof(strParams), "LOCALECHO", 1);
    TagFieldSetNumber(strParams, sizeof(strParams), "PLATFORM", TESTER_PLATFORM_XENON);
    g_pHostCore = TesterHostCoreCreate(strParams);

    // register local modules
    _T2HostRegisterModules();

    m_Listener = XNotifyCreateListener( XNOTIFY_SYSTEM | XNOTIFY_LIVE );

    m_LiveStatus = L"Undefined Xbox live status";
    wcscpy( m_users, L"Undefined" );

    return(S_OK);
}


/*F********************************************************************************/
/*!
    \Function ~CT2HostSample

    \Description
        Destructor for the T2Host/ATG sample shell

    \Input  None
    
    \Output None

    \Version 08/05/2005 (jfrank)
*/
/********************************************************************************F*/
CT2HostSample::~CT2HostSample()
{
    // kill all active processes
    ZShutdown();

    // kill the host core module
    TesterHostCoreDestroy(g_pHostCore);

    ZMemtrackShutdown();

    _CrtDumpMemoryLeaks();

    ZPrintf("\nQuitting T2Host.\n\n");
}

/*F********************************************************************************/
/*!
    \Function Process_Sys_SignInChanged

    \Description
        Process sign-in and sign-out notifications from the xbox360, and updates 
        member variables for displaying

    \Input  None
    
    \Output None

    \Version 12/20/2006 (jrainy)
*/
/********************************************************************************F*/
void CT2HostSample::Process_Sys_SignInChanged( )
{
    DWORD i,j;
    char buffer0[XUSER_NAME_SIZE];
    wchar_t buffer[XUSER_NAME_SIZE];

    wcscpy( m_users, L"" );

    for(i=0;i<4;i++)
    {
        if ( XUserGetSigninState( i ) == eXUserSigninState_SignedInToLive )
        {
            XUserGetName( i, buffer0, XUSER_NAME_SIZE );

            for( j=0; j<XUSER_NAME_SIZE; j++ )
            {
                buffer[j] = (wchar_t)buffer0[j];
            }
            wcscat( m_users, buffer );
            wcscat( m_users, L", " );
        }
    }
}

/*F********************************************************************************/
/*!
    \Function Process_Live_ConnectionChanged

    \Description
        Process live status notifications from the xbox360, and updates 
        member variables for displaying

    \Input  None
    
    \Output None

    \Version 12/20/2006 (jrainy)
*/
/********************************************************************************F*/
void CT2HostSample::Process_Live_ConnectionChanged( ULONG_PTR ulParam )
{
    switch( ulParam )
    {
    case XONLINE_S_LOGON_CONNECTION_ESTABLISHED :
        m_LiveStatus = L"Xbox live connection established.";
        break;
    case XONLINE_S_LOGON_DISCONNECTED  :
        m_LiveStatus = L"Xbox live disconnected.";
        break;
    case XONLINE_E_LOGON_NO_NETWORK_CONNECTION :
    case XONLINE_E_LOGON_CANNOT_ACCESS_SERVICE :
    case XONLINE_E_LOGON_UPDATE_REQUIRED :
    case XONLINE_E_LOGON_SERVERS_TOO_BUSY :
    case XONLINE_E_LOGON_CONNECTION_LOST :
    case XONLINE_E_LOGON_KICKED_BY_DUPLICATE_LOGON :
    case XONLINE_E_LOGON_INVALID_USER :
    default :
        m_LiveStatus = L"Xbox live error status.";
        break;
    }
}


/*F********************************************************************************/
/*!
    \Function Update

    \Description
        Update procedures for the ATG sample shell
        Added notifications processing

    \Input  None
    
    \Output None

    \Version 12/20/2006 (jrainy)
*/
/********************************************************************************F*/
HRESULT CT2HostSample::Update()
{
    DWORD dwNotificationID;
    ULONG_PTR ulParam;

    if( XNotifyGetNext( m_Listener, 0, &dwNotificationID, &ulParam ) )
    {
        switch( dwNotificationID )
        {
            case XN_SYS_SIGNINCHANGED:
                Process_Sys_SignInChanged( );
                break;

            case XN_LIVE_CONNECTIONCHANGED:
                Process_Live_ConnectionChanged( ulParam );
                break;
        }
    }
    // pump the host core module
    TesterHostCoreUpdate(g_pHostCore, 1);

    // give time to zlib
    ZTask();
    ZCleanup();

    // give time to network
    NetConnIdle();

    return(S_OK);
}


/*F********************************************************************************/
/*!
    \Function Render

    \Description
        Render procedures for the ATG sample shell

    \Input  None
    
    \Output None

    \Version 08/05/2005 (jfrank)
*/
/********************************************************************************F*/
HRESULT CT2HostSample::Render()
{
    // Draw a gradient filled background
    ATG::RenderBackground( TOP_BACK_COLOR, BOTTOM_BACK_COLOR );

    // Draw title text
    m_Font.Begin();

    m_Font.SetScaleFactors( 1.2f, 1.2f );
    m_Font.DrawText( 0, 0, 0xffffffff, L"T2Host" );
    m_Font.DrawText( 0, 60, 0xffffffff, m_LiveStatus );
    m_Font.DrawText( 0, 100, 0xffffffff, m_users );

    m_Font.End();

    // Present the scene
    m_pd3dDevice->Present( NULL, NULL, NULL, NULL );

    return(S_OK);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function WinMain

    \Description
        Main windows entry point.

    \Input Standard windows startup params
    
    \Output Process exit code

    \Version 03/22/2005 (jfrank)
*/
/********************************************************************************F*/
int main(int32_t argc, char **argv)
{
    CT2HostSample atgApp;
    ATG::GetVideoSettings( &atgApp.m_d3dpp.BackBufferWidth, &atgApp.m_d3dpp.BackBufferHeight );
    atgApp.Run();

}


