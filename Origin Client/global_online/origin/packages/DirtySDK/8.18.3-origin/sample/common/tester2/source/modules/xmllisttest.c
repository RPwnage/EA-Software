/*H********************************************************************************/
/*!
    \File xmllisttest.c

    \Description
        Test the XmlList API.

    \Copyright
        Copyright (c) 1999-2006 Electronic Arts Inc.

    \Version 12/07/2006 (kcarpenter) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "xmllist.h"
#include "netconn.h"
#include "lobbytagfield.h"
#include "lobbyhasher.h"
#include "lobbydisplist.h"
#include "testersubcmd.h"
#include "testerregistry.h"
#include "testermodules.h"

#include "zlib.h"

/*** Defines **********************************************************************/
#define DEFAULT_MAX_XML_LENGTH (100*1024)	// 100K Max XML file size

#define MAX_ATTRIB_LENGTH	(256)
#define MAX_URL_LENGTH		(256)
#define MAX_CONTENT_LENGTH	(4096)
#define MAX_LISTS			(5)				// The max number of lists that can be
											// created for testing.

/*** Type Definitions *************************************************************/
typedef struct XmlListAppT
{
    XmlListT *pXmlList;
    
    char strPersona[16];
    char strLKey[128];

} XmlListAppT;

static XmlListAppT _XmlList_App = { NULL, "", "" };

/*** Function Prototypes ***************************************************************/
static void _XmlListCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListReset(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListFetch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListAbortFetch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListGetLastEvent(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListGetLastError(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListGetServerError(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListRewind(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListNextItem(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListNextProperty(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListGetItemAttrib(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListGetPropertyName(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListGetPropertyAttrib(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListGetPropertyContent(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListSetLoginInfo(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);
static void _XmlListDump(void *_pApp, int32_t argc, char **argv, unsigned char bHelp);

/*** Variables ********************************************************************/
static T2SubCmdT _XmlList_Commands[] =
{
    { "create",        _XmlListCreate             },
    { "destroy",       _XmlListDestroy            },
    { "reset",         _XmlListReset              },
    { "fetch",         _XmlListFetch              },
    { "abort",         _XmlListAbortFetch         },
    { "getlastevent",  _XmlListGetLastEvent       },
    { "getlasterror",  _XmlListGetLastError       },
    { "getservererror",_XmlListGetServerError     },
    { "rewind",        _XmlListRewind             },
    { "nextitem",      _XmlListNextItem           },
    { "nextprop",      _XmlListNextProperty       },
    { "getitemattrib", _XmlListGetItemAttrib      },
    { "getpropname",   _XmlListGetPropertyName    },
    { "getpropattrib", _XmlListGetPropertyAttrib  },
    { "getpropcontent",_XmlListGetPropertyContent },
    { "setlogininfo",  _XmlListSetLoginInfo       },
    { "dump",          _XmlListDump               },
    { "",              NULL                       }
};

uint8_t attribBuffer[MAX_ATTRIB_LENGTH];
uint8_t contentBuffer[MAX_CONTENT_LENGTH];

/*** Private Functions ************************************************************/
/*F*************************************************************************************/
/*!
    \Function _CmdXmlListCb
    
    \Description
        XmlList idle callback.  This callback function exists only to clean up 
        when the tester exits.

    \Input *argz    - unused
    \Input argc     - argument count
    \Input **argv   - argument list

    \Output
        int32_t     - zero

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static int32_t _CmdXmlListCb(ZContext *argz, int32_t argc, char **argv)
{
    if (argc == 0)
    {
		// Shut down
        char *pDestroy = "destroy";

        _XmlListDestroy(&_XmlList_App, 1, &pDestroy, FALSE);
        return(0);
    }

    return(ZCallback(_CmdXmlListCb, 1000));
}

/*F*************************************************************************************/
/*!
    \Function _XmlListCallback
    
    \Description
        XmlList callback
    
    \Input *pXmlList    - pointer to XmlList
    \Input eEvent       - callback event type
    \Input *pEventData  - optional data associated with the event
    \Input *pUserData   - user callback data
    
    \Output None.
            
    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListCallback(XmlListT *pXmlList, XmlListEventE eEvent, uint32_t iEventData, void *pUserData)
{
    #if 0
    XmlListT *pApp = (XmlListT *)pUserData;
    int32_t iAssert = 0;

    switch(eEvent)
    {
        case XMLLIST_EVENT_REQ_COMPLETE_OK:
        {
        }
        break;

        case XMLLIST_EVENT_REQ_COMPLETE_ERROR:
        {
        }
        break;

        case XMLLIST_EVENT_REQ_ABORTED:
        {
        }
        break;

        case XMLLIST_EVENT_REQ_SENT:
        {
        }
        break;

        case XMLLIST_EVENT_ERROR:
        {
        }
        break;

        default:
            assert(iAssert);
    }
    #endif
}

/*F*************************************************************************************/
/*!
    \Function _XmlListCreate
    
    \Description
        XmlList subcommand - Create XmlListT
    
    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.
            
    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListCreate(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
	int32_t iMaxLength = DEFAULT_MAX_XML_LENGTH;		// 100K by default

    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s create [<max_xml_length>]\n", argv[0]);
        ZPrintf("       Create a new XML List\n");
        ZPrintf("       <max_xml_length> defaults to 100K]\n");
        return;
    }

    // Make sure we don't already have a ref
    if (pApp->pXmlList != NULL)
    {
        ZPrintf("   %s: ref already created\n", argv[0]);
        return;
    }

	// Override the maximum XML file size, if given
	if (argc == 3)
	{
		iMaxLength = strtol(argv[2], NULL, 10);
	}

    // Create the list
    pApp->pXmlList = XmlListCreate(iMaxLength, _XmlListCallback, pApp);
	if (pApp->pXmlList == NULL)
	{
		ZPrintf("  %s: unable to create XmlList - XmlListCreate() failed\n");
		return;
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListDestroy
    
    \Description
        XmlList subcommand - Destroy XmlList module
    
    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.
            
    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListDestroy(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if (bHelp == TRUE)
    {
        ZPrintf("   usage: %s destroy\n", argv[0]);
        ZPrintf("       Destroy the previously created XML List\n");
        return;
    }

    if (pApp->pXmlList)
    {
        XmlListDestroy(pApp->pXmlList);
        pApp->pXmlList = NULL;
		ZPrintf("%s: ItemList destroyed\n", argv[0]);
    }
	else
	{
		ZPrintf("%s: No ItemList exists!\n", argv[0]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListReset
    
    \Description
        XmlList subcommand - Reset to default state
    
    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.
            
    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListReset(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s reset\n", argv[0]);
        ZPrintf("       Reset the XML List to it's default state\n");
        return;
    }

    ZPrintf("%s: reseting XmlList to defaults\n", argv[0]);
    XmlListReset(pApp->pXmlList);
}

/*F*************************************************************************************/
/*!
    \Function _XmlListFetch

    \Description
        XmlList subcommand - Fetch an ItemML file from a URL

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListFetch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	char FinalUrl[MAX_URL_LENGTH];

    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s fetch [<absolute_url>|<relative_url>]\n", argv[0]);
        ZPrintf("       Fetch ItemML data from the given URL\n");
        ZPrintf("       (relative URLs are relative to http://sdevbesl03.online.ea.com/xmllist/\n");
        return;
    }

	// Absolute URL's contain a colon, relative ones do not( "http://" )
	if (strchr(argv[2], ':') == 0)
	{
		snzprintf(FinalUrl, MAX_URL_LENGTH, "http://sdevbesl03.online.ea.com/xmllist/%s", argv[2]);
	}
	else
	{
		snzprintf(FinalUrl, MAX_URL_LENGTH, "%s", argv[2]);
	}

	// If user didn't set an LKEY, try to use the one from the lobby logon result
	if (pApp->strLKey == 0)
	{
		// Try and get lobbyapi ref - if we can't get it here, then no login info will be set (which may or may not be OK)
        strnzcpy(pApp->strLKey, "CHANGEME", sizeof(pApp->strLKey));
        strnzcpy(pApp->strPersona, "CHANGEME", sizeof(pApp->strPersona));

		// Set params
		XmlListSetLoginInfo(pApp->pXmlList, pApp->strLKey, pApp->strPersona);
	}

	ZPrintf("%s: fetching ItemML from '%s'\n", argv[0], FinalUrl);
    XmlListFetch(pApp->pXmlList, (unsigned char *)FinalUrl);
}

/*F*************************************************************************************/
/*!
    \Function _XmlListAbortFetch

    \Description
        XmlList subcommand - Abort a previously started fetch

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListAbortFetch(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s abort\n", argv[0]);
        ZPrintf("       Abort a previously issued call to fetch\n");
        return;
    }

    ZPrintf("%s: aborting fetch\n", argv[0]);
    XmlListAbortFetch(pApp->pXmlList);
}

/*F*************************************************************************************/
/*!
    \Function _XmlListGetLastEvent

    \Description
        XmlList subcommand - Get the last event that occurred

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListGetLastEvent(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getlastevent\n", argv[0]);
        ZPrintf("       Get the last event code\n");
        return;
    }

    ZPrintf("%s: last event was %d\n", argv[0], XmlListGetLastEvent(pApp->pXmlList));
}

/*F*************************************************************************************/
/*!
    \Function _XmlListGetLastError

    \Description
        XmlList subcommand - Get the last error that occurred

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListGetLastError(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getlasterror\n", argv[0]);
        ZPrintf("       Get the last error code\n");
        return;
    }

    ZPrintf("%s: last error was %d\n", argv[0], XmlListGetLastError(pApp->pXmlList));
}

/*F*************************************************************************************/
/*!
    \Function _XmlListGetServerError

    \Description
        XmlList subcommand - Get the last server error that was returned

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 01/11/07 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListGetServerError(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getservererror\n", argv[0]);
        ZPrintf("       Get the last server error code (e.g., from err in <items err=\"0\">)\n");
        return;
    }

    ZPrintf("%s: last error was %d\n", argv[0], XmlListGetServerError(pApp->pXmlList));
}

/*F*************************************************************************************/
/*!
    \Function _XmlListRewind

    \Description
        XmlList subcommand - Rewind the parser to the start of the XML

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListRewind(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s rewind\n", argv[0]);
        ZPrintf("       Rewind the parser to the start of the XML\n");
        return;
    }

    ZPrintf("%s: rewinding XML parser to beginning of data\n", argv[0]);
	XmlListRewind(pApp->pXmlList);
}

/*F*************************************************************************************/
/*!
    \Function _XmlListNextItem

    \Description
        XmlList subcommand - Search for the next item tag

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListNextItem(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	int32_t isFound = FALSE;
	XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s nextitem\n", argv[0]);
        ZPrintf("       Search for the next item tag\n");
        return;
    }

	isFound = XmlListNextItem(pApp->pXmlList);
	if (isFound)
	{
		ZPrintf("%s: Found the next item.\n", argv[0]);
	}
	else
	{
		ZPrintf("%s: No more items found!\n", argv[0]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListNextProperty

    \Description
        XmlList subcommand - Search for the next property tag

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListNextProperty(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	int32_t isFound = FALSE;
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s nextprop\n", argv[0]);
        ZPrintf("       Search for the next property tag\n");
        return;
    }

	isFound = XmlListNextProperty(pApp->pXmlList);
	if (isFound)
	{
		ZPrintf("%s: Found the next property.\n", argv[0]);
	}
	else
	{
		ZPrintf("%s: No more properties found!\n", argv[0]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListGetItemAttrib

    \Description
        XmlList subcommand - Get the value of the named attribute from the current item tag

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListGetItemAttrib(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	int32_t isFound = FALSE;
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s getitemattrib <attrib_name>\n", argv[0]);
        ZPrintf("       Get the value of the named attribute from the current item tag\n");
        return;
    }

	isFound = XmlListGetItemAttrib(pApp->pXmlList, (unsigned char *)argv[2], attribBuffer, MAX_ATTRIB_LENGTH);
	if (isFound)
	{
		ZPrintf("%s: attribute value of %s is '%s'\n", argv[0], argv[2], attribBuffer);
	}
	else
	{
		ZPrintf("%s: no attribute named %s was found.\n", argv[0], argv[2]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListGetPropertyName

    \Description
        XmlList subcommand - Get the name of the current property

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListGetPropertyName(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	int32_t isFound = FALSE;
    XmlListAppT *pApp = (XmlListAppT *)_pApp;

    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getpropname\n", argv[0]);
        ZPrintf("       Get the name of the current property\n");
        return;
    }

	isFound = XmlListGetPropertyName(pApp->pXmlList, attribBuffer, MAX_ATTRIB_LENGTH);
	if (isFound)
	{
		ZPrintf("%s: property name is '%s'\n", argv[0], attribBuffer);
	}
	else
	{
		ZPrintf("%s: no 'name' attribute was found!\n", argv[0]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListGetPropertyAttrib

    \Description
        XmlList subcommand - Get the value of the named attribute from the current property tag

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListGetPropertyAttrib(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	int32_t isFound = FALSE;
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 3))
    {
        ZPrintf("   usage: %s getitemattrib <attrib_name>\n", argv[0]);
        ZPrintf("       Get the value of the named attribute from the current property tag\n");
        return;
    }

	isFound = XmlListGetPropertyAttrib(pApp->pXmlList, (unsigned char *)argv[2], attribBuffer, MAX_ATTRIB_LENGTH);
	if (isFound)
	{
		ZPrintf("%s: attribute value of %s is '%s'\n", argv[0], argv[2], attribBuffer);
	}
	else
	{
		ZPrintf("%s: no attribute named %s was found.\n", argv[0], argv[2]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListGetPropertyContent

    \Description
        XmlList subcommand - Get the value of the current property tag's content data

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListGetPropertyContent(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	int32_t isFound = FALSE;
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s getpropcontent\n", argv[0]);
        ZPrintf("       Get the value of the current property tag's content data\n");
        return;
    }

	isFound = XmlListGetPropertyContent(pApp->pXmlList, contentBuffer, MAX_CONTENT_LENGTH);
	if (isFound)
	{
		ZPrintf("%s: property content is '%s'\n", argv[0], contentBuffer);
	}
	else
	{
		ZPrintf("%s: no property content was was found.\n", argv[0]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListSetLoginInfo

    \Description
        XmlList subcommand - Set the login info (LKEY) for the fetch to use.

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListSetLoginInfo(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc < 3))
    {
        ZPrintf("   usage: %s setlogininfo <lkey> [<persona>]\n", argv[0]);
        ZPrintf("       Set the login info (LKEY) for the fetch to use\n");
        return;
    }

	if (argc == 3)
	{
        XmlListSetLoginInfo(pApp->pXmlList, argv[2], 0);
	}
	else // 4 or greater
	{
        XmlListSetLoginInfo(pApp->pXmlList, argv[2], argv[3]);
	}
}

/*F*************************************************************************************/
/*!
    \Function _XmlListDump

    \Description
        XmlList subcommand - Print out the full raw XML

    \Input *pApp    - pointer to app
    \Input argc     - argument count
    \Input **argv   - argument list
	\Input bHelp    - TRUE if help is requested

    \Output None.

    \Version 1.0 12/13/06 (kcarpenter) First Version
*/
/**************************************************************************************F*/
static void _XmlListDump(void *_pApp, int32_t argc, char **argv, unsigned char bHelp)
{
	uint8_t *pXml = NULL;
    XmlListAppT *pApp = (XmlListAppT *)_pApp;
    
    if ((bHelp == TRUE) || (argc != 2))
    {
        ZPrintf("   usage: %s dump\n", argv[0]);
        ZPrintf("       Print out the full raw XML that was retrieved with fetch\n");
        return;
    }

	pXml = XmlListGetRawXml(pApp->pXmlList);
	if (pXml)
	{
		ZPrintf("%s: XML data follows:\n%s\n", argv[0], pXml);
	}
	else
	{
		ZPrintf("%s: No XML data available!\n", argv[0]);
	}
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdXmlList

    \Description
        Test the XmlList API

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list
    
    \Output
		int32_t		- standard return value

    \Version 12/07/2006 (kcarpenter)
*/
/********************************************************************************F*/
int32_t CmdXmlList(ZContext *argz, int32_t argc, char *argv[])
{
    T2SubCmdT *pCmd;
    unsigned char bHelp, bCreate = FALSE;
    XmlListAppT *pApp = &_XmlList_App;

    // Handle basic help
    if ((argc <= 1) || (((pCmd = T2SubCmdParse(_XmlList_Commands, argc, argv, &bHelp)) == NULL)))
    {
        ZPrintf("   Fetch and parse ItemML data from a remote server\n");
        T2SubCmdUsage(argv[0], _XmlList_Commands);
        return(0);
    }

    // If no ref yet, make one, unless it is a create or destroy call
    if ((pApp->pXmlList == NULL) && ((strcmp(pCmd->strName, "create") != 0)
		                             && (strcmp(pCmd->strName, "destroy") != 0)))

    {
        char *pCreate = "create";
        ZPrintf("   %s: ref has not been created - creating\n", argv[0]);
        _XmlListCreate(pApp, 1, &pCreate, bHelp);
        bCreate = TRUE;
    }

	// Hand off to command
    pCmd->pFunc(pApp, argc, argv, bHelp);

    // If we executed create, remember
    if (pCmd->pFunc == _XmlListCreate)
    {
        bCreate = TRUE;
    }

    // If we executed create, install periodic callback
    return((bCreate == TRUE) ? ZCallback(_CmdXmlListCb, 1000) : 0);
}
