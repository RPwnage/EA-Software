/*H********************************************************************************/
/*!
    \File xmllist.h

    \Description
        The XmlList API is used to retrieve and parse XML data in the ItemML
        format.  It provies the ability to retrieve the data from an EA server
        URL.  An iteration-oriented API allows the caller to walk through the
        returned items and properties to access the data.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2006. ALL RIGHTS RESERVED.

    \Version 1.0 12/05/2006 (kcarpenter) First Version
*/
/********************************************************************************H*/

#ifndef _xmllist_h
#define _xmllist_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/
#define XMLLIST_MAX_URL_LENGTH          (256)
#define XMLLIST_MAX_LKEY_LENGTH         (128)
#define XMLLIST_MAX_PERSONA_LENGTH      (32)

#define XMLLIST_ITEMS_TAG           "itms"
#define XMLLIST_ITEM_TAG            "itm"
#define XMLLIST_PROP_TAG            "prp"

#define XMLLIST_NAME_ATTRIB         "name"
#define XMLLIST_TYPE_ATTRIB         "type"
#define XMLLIST_ERROR_ATTRIB        "err"

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef enum
{
    XMLLIST_EVENT_NONE,                     //!< Nothing going on
    XMLLIST_EVENT_ERROR,                    //!< An error occurred - See error code for more details
    XMLLIST_EVENT_REQ_SENT,                 //!< Request has been made and we are waiting for a response
    XMLLIST_EVENT_REQ_COMPLETE_OK,          //!< Request completed successfully
    XMLLIST_EVENT_REQ_COMPLETE_ERROR,       //!< Request completed with an error
    XMLLIST_EVENT_REQ_ABORTED,              //!< Caller aborted the request
} XmlListEventE;    //!< Event types

typedef enum
{
    XMLLIST_ERROR_OK,                       //!< No error occurred.
    XMLLIST_ERROR_PARSER,                   //!< XML Parser encountered an error
    XMLLIST_ERROR_NO_ITEMS_RETURNED,        //!< The server returned 0 in the xmllist "num" attribute
    XMLLIST_ERROR_UNAUTHORIZED,             //!< The requested URL cannot be accessed do to security permissions
    XMLLIST_ERROR_NOT_FOUND,                //!< The requested URL cannot be found
    XMLLIST_ERROR_SERVER,                   //!< Generic "server" error
    XMLLIST_ERROR_TIMEOUT,                  //!< Timeout while waiting for the HTTP data
} XmlListErrorE;    //!< Error types

//! Opaque module ref
typedef struct XmlListT XmlListT;

//!< User Event Callback
typedef void (XmlListCallbackT)(XmlListT *pXmlList, XmlListEventE eEvent, uint32_t iEventData, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create an XmlListT
XmlListT *XmlListCreate(uint32_t iMaxXmlLength, XmlListCallbackT *pCallback, void *pUserData);

// Destroy an XmlListT
void XmlListDestroy(XmlListT *pXmlList);

// Reset an XmlListT to defaults
void XmlListReset(XmlListT *pXmlList);

// Fetch data from the specified URL
void XmlListFetch(XmlListT *pXmlList, const uint8_t *pUrl);

// Set the authentication parameters
void XmlListSetLoginInfo(XmlListT *pXmlList, const char *pLKey, const char *pPersona);

// Abort a previously started Fetch
void XmlListAbortFetch(XmlListT *pXmlList);

// Get the last event that occurred
XmlListEventE XmlListGetLastEvent(XmlListT *pXmlList);

// Get the last error that occurred
XmlListErrorE XmlListGetLastError(XmlListT *pXmlList);

// Get the last error code reported by the web server.
// This is the value of the "err" attribute in the <items> tag.
int32_t XmlListGetServerError(XmlListT *pXmlList);

// Rewind the parser to the start of the XML
void XmlListRewind(XmlListT *pXmlList);

// Search for the next item tag
int32_t XmlListNextItem(XmlListT *pXmlList);

// Search for the next property tag
int32_t XmlListNextProperty(XmlListT *pXmlList);

// Get the value of the named attribute from the current item tag
int32_t XmlListGetItemAttrib(XmlListT *pXmlList, uint8_t *pAttribName, uint8_t *pBuffer, int32_t iLength);

// Get the name of the current property
int32_t XmlListGetPropertyName(XmlListT *pXmlList, uint8_t *pBuffer, int32_t iLength);

// Get the value of the named attribute from the current property tag
int32_t XmlListGetPropertyAttrib(XmlListT *pXmlList, uint8_t *pAttribName, uint8_t *pBuffer, int32_t iLength);

// Get the value of the current property tag's content data
int32_t XmlListGetPropertyContent(XmlListT *pXmlList, uint8_t *pBuffer, int32_t iLength);

// Return a pointer to the internal XML buffer
uint8_t *XmlListGetRawXml(XmlListT *pXmlList);

#ifdef __cplusplus
};
#endif

#endif // _xmllist_h
