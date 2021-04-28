/*H***************************************************************************/
/*!
    \File weboffer.h

    \Description
        This module implements a simple web driven dynamic offer system.
        It downloads a simple script from a web server and uses that to
        display alerts and data entry screens.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/13/2004 (gschaefer) First Version
    \Version 1.1 01/06/2005 (jfrank)    Moved buttons into aggregate types and added macros to handle them.
                                        Moved buffer sizes into Defines section
*/
/***************************************************************************H*/

#ifndef _weboffer_h
#define _weboffer_h

/*!
\Moduledef WebOffer WebOffer
\Modulemember Misc
*/
//@{

/*** Include files ***************************************************/

#include "DirtySDK/platform.h"

/*** Defines *********************************************************/

#define WEBOFFER_SUGGESTED_MEM      (150*1024)  //!< suggested script size (allow for localized TOS)

#define WEBOFFER_CMD_ALERT          ('alrt')    //!< show an alert
#define WEBOFFER_CMD_PROMO          ('prom')    //!< show promo screen
#define WEBOFFER_CMD_CREDIT         ('card')    //!< show credit card screen
#define WEBOFFER_CMD_HTTP           ('http')    //!< do http transfer
#define WEBOFFER_CMD_NEWS           ('news')    //!< show news screen
#define WEBOFFER_CMD_MENU           ('menu')    //!< show a menu of options
#define WEBOFFER_CMD_ARTICLES       ('arti')    //!< list of articles
#define WEBOFFER_CMD_STORY          ('stor')    //!< show a text based story
#define WEBOFFER_CMD_MEDIA          ('medi')    //!< show a media based story
#define WEBOFFER_CMD_FORM           ('form')    //!< show a web offer form
#define WEBOFFER_CMD_MARKET         ('mplc')    //!< show the platform marketplace UI


#define WEBOFFER_ALERT_NUMBUTTONS       (4)     //!< number of alert buttons
#define WEBOFFER_NEWS_NUMBUTTONS        (4)     //!< number of news buttons
#define WEBOFFER_BUSY_NUMBUTTONS        (1)     //!< number of busy buttons
#define WEBOFFER_CREDIT_NUMBUTTONS      (4)     //!< number of credit buttons
#define WEBOFFER_PROMO_NUMBUTTONS       (4)     //!< number of promo buttons
#define WEBOFFER_MENU_NUMBUTTONS        (8)     //!< number of menu buttons
#define WEBOFFER_ARTICLES_NUMBUTTONS    (4)     //!< number of article buttons
#define WEBOFFER_FORM_NUMBUTTONS        (4)     //!< number of form buttons
#define WEBOFFER_FORM_NUMFIELDS         (16)    //!< number of fields allowed on a web offer form

#define WEBOFFER_ARTICLES_NUMARTICLES   (16)    //!< number of articles in a WebOfferArticlesT

#define WEB_OFFER_FORM_STRING           (0)     //!< Form Style input string
#define WEB_OFFER_FORM_LIST             (1)     //!< Form Style List of possible values
#define WEB_OFFER_FORM_STATIC_STRING    (2)     //!< Form Style Static String
#define WEB_OFFER_FORM_NUMERIC          (3)     //!< Form Style Numeric Value
#define WEB_OFFER_FORM_FLOAT            (4)     //!< Form Style Float Value

#define WEBOFFER_NEWS_NUM_FIXED_IMAGE       (4)     //!< number of auxillary pictures allowed for the news screen
#define WEBOFFER_CREDIT_NUM_FIXED_IMAGE     (4)     //!< number of auxillary pictures allowed for the credit card screen
#define WEBOFFER_PROMO_NUM_FIXED_IMAGE      (4)     //!< number of auxillary pictures allowed for the promotions screen
#define WEBOFFER_ARTICLES_NUM_FIXED_IMAGE   (4)     //!< number of auxillary pictures allowed for the article list screen
#define WEBOFFER_FORM_NUM_FIXED_IMAGE       (4)     //!< number of auxillary pictures allowed for a web offer form screen

//! weboffer actions
enum WEB_OFFER_ACTION
{
    // abort script
    WEBOFFER_ACTION_ABORT = 0,

    // user clicked a button
    WEBOFFER_ACTION_BTN1,
    WEBOFFER_ACTION_BTN2,
    WEBOFFER_ACTION_BTN3,
    WEBOFFER_ACTION_BTN4,
    WEBOFFER_ACTION_BTN5,
    WEBOFFER_ACTION_BTN6,
    WEBOFFER_ACTION_BTN7,
    WEBOFFER_ACTION_BTN8,

    // user selected an article
    WEBOFFER_ACTION_ARTICLE1,
    WEBOFFER_ACTION_ARTICLE2,
    WEBOFFER_ACTION_ARTICLE3,
    WEBOFFER_ACTION_ARTICLE4,
    WEBOFFER_ACTION_ARTICLE5,
    WEBOFFER_ACTION_ARTICLE6,
    WEBOFFER_ACTION_ARTICLE7,
    WEBOFFER_ACTION_ARTICLE8,
    WEBOFFER_ACTION_ARTICLE9,
    WEBOFFER_ACTION_ARTICLE10,
    WEBOFFER_ACTION_ARTICLE11,
    WEBOFFER_ACTION_ARTICLE12,
    WEBOFFER_ACTION_ARTICLE13,
    WEBOFFER_ACTION_ARTICLE14,
    WEBOFFER_ACTION_ARTICLE15,
    WEBOFFER_ACTION_ARTICLE16,

    // last action
    WEBOFFER_ACTION_LAST
};

#define WEBOFFER_TYPE_NORMAL        (' ')       //!< button type is normal
#define WEBOFFER_TYPE_SUBMIT        ('^')       //!< button type is submit

//! Display sizes (how many characters to show on the screen)
#define WEBOFFER_BUTTON_DISPMAX     (20)        //!< display chars for button name
#define WEBOFFER_TITLE_DISPMAX      (64)        //!< display chars for title
#define WEBOFFER_BUSYMSG_DISPMAX    (512)       //!< display chars for busy message
#define WEBOFFER_ALERTMSG_DISPMAX   (512)       //!< display chars for alert message
#define WEBOFFER_CCMSG_DISPMAX      (512)       //!< display chars for credit card message
#define WEBOFFER_CCNAME_DISPMAX     (50)        //!< display chars for first/last name
#define WEBOFFER_CCADDR_DISPMAX     (50)        //!< display chars per address line
#define WEBOFFER_CCCITY_DISPMAX     (50)        //!< display character for city
#define WEBOFFER_CCSTATE_DISPMAX    (20)        //!< display chars for state
#define WEBOFFER_CCPOST_DISPMAX     (20)        //!< display chars for post/zip
#define WEBOFFER_CCCOUNTRY_DISPMAX  (25)        //!< display chars for country
#define WEBOFFER_CCEMAIL_DISPMAX    (50)        //!< display chars for email
#define WEBOFFER_CCCARDTYPE_DISPMAX (40)        //!< display chars for credit card type
#define WEBOFFER_CCCARDNUM_DISPMAX  (20)        //!< display chars for credit card number
#define WEBOFFER_PROMOMSG_DISPMAX   (512)       //!< display chars for promotion message
#define WEBOFFER_PROMOENTRY_DISPMAX (100)       //!< display chars for promotion entry
#define WEBOFFER_SUMMARY_DISPMAX    (2048)      //!< display chars for summary in Articles
#define WEBOFFER_FORMMSG_DISPMAX    (512)       //!< display chars for form message
#define WEBOFFER_FORMVALUE_DISPMAX  (64)        //!< display chars for form values

//! Memory storage sizes (number of bytes used to store the above displayed strings)
//!   based on display size (above defines) and storage size conversion macro (above)
#define WEBOFFER_BUTTON_MEMSIZE     (WEBOFFER_DispToMemSize(WEBOFFER_BUTTON_DISPMAX))
#define WEBOFFER_TITLE_MEMSIZE      (WEBOFFER_DispToMemSize(WEBOFFER_TITLE_DISPMAX))
#define WEBOFFER_BUSYMSG_MEMSIZE    (WEBOFFER_DispToMemSize(WEBOFFER_BUSYMSG_DISPMAX))
#define WEBOFFER_ALERTMSG_MEMSIZE   (WEBOFFER_DispToMemSize(WEBOFFER_ALERTMSG_DISPMAX))
#define WEBOFFER_CCMSG_MEMSIZE      (WEBOFFER_DispToMemSize(WEBOFFER_CCMSG_DISPMAX))
#define WEBOFFER_CCNAME_MEMSIZE     (WEBOFFER_DispToMemSize(WEBOFFER_CCNAME_DISPMAX))
#define WEBOFFER_CCADDR_MEMSIZE     (WEBOFFER_DispToMemSize(WEBOFFER_CCADDR_DISPMAX))
#define WEBOFFER_CCCITY_MEMSIZE     (WEBOFFER_DispToMemSize(WEBOFFER_CCCITY_DISPMAX))
#define WEBOFFER_CCSTATE_MEMSIZE    (WEBOFFER_DispToMemSize(WEBOFFER_CCSTATE_DISPMAX))
#define WEBOFFER_CCPOST_MEMSIZE     (WEBOFFER_DispToMemSize(WEBOFFER_CCPOST_DISPMAX))
#define WEBOFFER_CCCOUNTRY_MEMSIZE  (WEBOFFER_DispToMemSize(WEBOFFER_CCCOUNTRY_DISPMAX))
#define WEBOFFER_CCEMAIL_MEMSIZE    (WEBOFFER_DispToMemSize(WEBOFFER_CCEMAIL_DISPMAX))
#define WEBOFFER_CCCARDTYPE_MEMSIZE (WEBOFFER_DispToMemSize(WEBOFFER_CCCARDTYPE_DISPMAX))
#define WEBOFFER_CCCARDNUM_MEMSIZE  (WEBOFFER_DispToMemSize(WEBOFFER_CCCARDNUM_DISPMAX))
#define WEBOFFER_PROMOMSG_MEMSIZE   (WEBOFFER_DispToMemSize(WEBOFFER_PROMOMSG_DISPMAX))
#define WEBOFFER_PROMOENTRY_MEMSIZE (WEBOFFER_DispToMemSize(WEBOFFER_PROMOENTRY_DISPMAX))
#define WEBOFFER_SUMMARY_MEMSIZE    (WEBOFFER_DispToMemSize(WEBOFFER_SUMMARY_DISPMAX))
#define WEBOFFER_FORMMSG_MEMSIZE    (WEBOFFER_DispToMemSize(WEBOFFER_FORMMSG_DISPMAX))
#define WEBOFFER_FORMVALUE_MEMSIZE  (WEBOFFER_DispToMemSize(WEBOFFER_FORMVALUE_DISPMAX))

#define WEBOFFER_IMAGE_MEMSIZE      32          //!< fixed size size not a display name

/*** Macros **********************************************************/

/*!
    Games that support Greek and/or Asian languages encode each
    user-visible character using UTF-8 which results in a single
    character being represented by 1-4 bytes.  These games therefore
    need larger buffers to hold strings.

    The following macro is used to convert the suggested display
    length to a storage size.  If space is at a premium and the game
    is English only, the calculation can be reduced to simply equate
    the two sizes.  Otherwise, this formula should be adjusted to take
    into account the average display size and average storage size of
    the strings to be used.
*/
#define WEBOFFER_DispToMemSize(nDispChars)  ((nDispChars * 100) / 50)

/*** Type Definitions ************************************************/

typedef struct WebOfferT WebOfferT;

typedef struct WebOfferSetupT
{
    char strPersona[20];        //!< user persona
    char strFavTeam[32];        //!< favorite team (recommended but not required)
    char strFavPlyr[32];        //!< favorite player (recommended but not required)
    char strProduct[32];        //!< product identifier (i.e., MADDEN-2005)
    char strPlatform[32];       //!< platform identifier (i.e., PS2, XBOX, PC)
    char strPrivileges[16];     //!< user privileges: P=able to purchase content
    char strLanguage[4];        //!< 2-char ISO language identifier
    char strCountry[4];         //!< 2-char ISO country identifier
    char strGameRegion[8];      //!< 4-char Game Region locality token
    char strSlusCode[32];       //!< product SLUS code
    char strLKey[40];           //!< LKEY from lobby login
    char strSkuCode[32];        //!< product SKU code
    uint32_t uColor;            //!< the Default Color to use
    char strUID[24];            //!< the user's platform UID
    uint32_t uMemSize;          //!< buffer size allocated for weboffer by the game
} WebOfferSetupT;

typedef struct WebOfferCommandT
{
    int32_t iCommand;           //!< WEBOFFER_CMD_xxxx
    char strParms[8192];        //!< command parameters (app does not normally use directly)
    struct  {
        uint32_t uTokenModule;   //!< the module token to send to Telemetry
        uint32_t uTokenGroup;    //!< the group token to send to Telemetry
        char strTelemetry[17];   //!< the string to send to Telemetry API
    } Telemetry ;
} WebOfferCommandT;

typedef struct WebOfferColorT
{
    unsigned char uRed;         //!< 0-255 red
    unsigned char uGreen;       //!< 0-255 green
    unsigned char uBlue;        //!< 0-255 blue
    unsigned char uAlpha;       //!< 1-255 alpha (0=color is not valid)
} WebOfferColorT;

typedef struct WebOfferResourceT
{
    int32_t iType;              //!< resource type
    int32_t iSize;              //!< resource size
    const char *pParm;          //!< pointer to tagfield parms
    const char *pData;          //!< pointer to resource data
} WebOfferResourceT;

typedef struct WebOfferButtonT
{
    char strType[1];                        //!< button type flag (normal or submit)
    char strText[WEBOFFER_BUTTON_MEMSIZE];  //!< button text buffer
    WebOfferColorT Color;                   //!< button color
} WebOfferButtonT;

typedef struct WebOfferFixedImageT
{
    char strImage[WEBOFFER_IMAGE_MEMSIZE];     //!< picture name
} WebOfferFixedImageT;

typedef struct WebOfferBusyT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];      //!< busy dialog title
    char strMessage[WEBOFFER_BUSYMSG_MEMSIZE];  //!< e.g. "Please Wait"
    WebOfferColorT MsgColor;                    //!< message color
    WebOfferButtonT Button[WEBOFFER_BUSY_NUMBUTTONS]; //!< buttons
    //!< Type is always ' '
    //!< Button[0].strText should be blank if no button
} WebOfferBusyT;

typedef struct WebOfferAlertT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];      //!< alert title
    char strImage[WEBOFFER_IMAGE_MEMSIZE];      //!< image name (optional)
    char strMessage[WEBOFFER_ALERTMSG_MEMSIZE]; //!< alert text
    WebOfferColorT MsgColor;                    //!< text color
    WebOfferButtonT Button[WEBOFFER_ALERT_NUMBUTTONS]; //!< buttons
    //!< Type is always ' '
} WebOfferAlertT;

typedef struct WebOfferCreditT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];              //!< dialog title
    char strMessage[WEBOFFER_CCMSG_MEMSIZE];            //!< create card offer message
    WebOfferColorT MsgColor;                            //!< message color
    WebOfferButtonT Button[WEBOFFER_CREDIT_NUMBUTTONS]; //!< buttons
    //!< Type is '^' for submit, '_' for normal
    //!< example: Button[0].strText might be "Submit"
    //!< example: Button[1].strText might be "Cancel"
    char strFirstName[WEBOFFER_CCNAME_MEMSIZE];         //!< e.g. "Wayne"
    char strLastName[WEBOFFER_CCNAME_MEMSIZE];          //!< e.g. "Newton"
    char strAddress[2][WEBOFFER_CCADDR_MEMSIZE];        //!< e.g. "3000 Las Vegas Blvd. South"
    char strCity[WEBOFFER_CCCITY_MEMSIZE];              //!< e.g. "Las Vegas"
    char strState[WEBOFFER_CCSTATE_MEMSIZE];            //!< e.g. "Nevada"
    char strPostCode[WEBOFFER_CCPOST_MEMSIZE];          //!< e.g. "89109"
    const char *pCountryList;                           //!< list of countries (decode with WebOfferParamList)
    char strCountry[WEBOFFER_CCCOUNTRY_MEMSIZE];        //!< e.g. "United States"
    char strEmail[WEBOFFER_CCEMAIL_MEMSIZE];            //!< e.g. "wayne@stardust.com"
    const char *pCardList;                              //!< list of cards (decode with WebOfferParamList)
    char strCardType[WEBOFFER_CCCARDTYPE_MEMSIZE];      //!< "Visa", "MasterCard" or "American Express"
    char strCardNumber[WEBOFFER_CCCARDNUM_MEMSIZE];     //!< 16 digits + 3 dividers
    int32_t iExpiryMonth;                               //!< 1 .. 12 inclusive
    int32_t iExpiryYear;                                //!< 2003 ...
    int32_t iYearStartExpiry;                           //!< what year start the expiration combo box from
    int32_t iYearEndExpiry;                             //!< The year the expiry combo box end with
    WebOfferFixedImageT FixedImage[WEBOFFER_CREDIT_NUM_FIXED_IMAGE];  //!< maximum number of pictures

} WebOfferCreditT;

typedef struct WebOfferPromoT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];      //!< promo title
    char strMessage[WEBOFFER_PROMOMSG_MEMSIZE]; //!< promo text
    WebOfferColorT MsgColor;                    //!< message color
    WebOfferButtonT Button[WEBOFFER_PROMO_NUMBUTTONS]; //!< buttons
    //!< Type is '^' for submit, '_' for normal
    //!< example: Button[0].strText might be "Submit"
    //!< example: Button[1].strText might be "Cancel"
    char strPromo[WEBOFFER_PROMOENTRY_MEMSIZE]; //!< promo entry string
    char strPromoKPopTitle[WEBOFFER_TITLE_MEMSIZE];//!< promo kpop title string
    int32_t iStyle;                             //!< promo style to suggest screen implementation
    WebOfferFixedImageT FixedImage[WEBOFFER_PROMO_NUM_FIXED_IMAGE];  //!< maximum number of pictures

} WebOfferPromoT;

typedef struct WebOfferNewsT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];      //!< promo title
    char strImage[WEBOFFER_IMAGE_MEMSIZE];      //!< image name (optional)
    WebOfferButtonT Button[WEBOFFER_NEWS_NUMBUTTONS]; //!< buttons
    //!< Type is always ' '
    //!< example: Button[0].strText might be "Submit"
    //!< example: Button[1].strText might be "Cancel"
    WebOfferColorT NewsColor;                   //!< news color
    const char *pNews;                          //!< pointer to news data
    int32_t iStyle;                             //!< news style to suggest screen implementation
    WebOfferFixedImageT FixedImage[WEBOFFER_NEWS_NUM_FIXED_IMAGE];  //!< maximum number of pictures
} WebOfferNewsT;

typedef struct WebOfferMenuT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];      //!< menu title
    WebOfferButtonT Button[WEBOFFER_MENU_NUMBUTTONS]; //!< buttons
    //!< Type is always ' '
} WebOfferMenuT;

typedef struct WebOfferArticlesT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];          //!< title of screen
    struct {
        char strArticle[WEBOFFER_TITLE_MEMSIZE];    //!< name of the article
        char strImage[WEBOFFER_IMAGE_MEMSIZE];      //!< optional resource name of icon (use WebOfferResource to fetch)
        char strType[4];                            //!< The type of the article
        char strDesc[WEBOFFER_SUMMARY_MEMSIZE];     //!< the description of the article
    } Article[WEBOFFER_ARTICLES_NUMARTICLES];       //!< list of articles
    WebOfferButtonT Button[WEBOFFER_ARTICLES_NUMBUTTONS]; //!< buttons
    WebOfferFixedImageT FixedImage[WEBOFFER_ARTICLES_NUM_FIXED_IMAGE];  //!< maximum number of pictures
} WebOfferArticlesT;

typedef struct WebOfferStoryT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];      //!< promo title
    char strImage[WEBOFFER_IMAGE_MEMSIZE];      //!< image name (optional)
    const char *pText;                          //!< pointer to story text
} WebOfferStoryT;

typedef struct WebOfferMediaT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];      //!< title of article
    char strImage[WEBOFFER_IMAGE_MEMSIZE];      //!< image name (optional)
    const char *pMedia;                         //!< list of media URLs
} WebOfferMediaT;


typedef struct WebOfferFormT
{
    char strTitle[WEBOFFER_TITLE_MEMSIZE];              //!< title of form
    char strMessage[WEBOFFER_FORMMSG_MEMSIZE];          //!< Text to display
    struct {
        char strName[WEBOFFER_TITLE_MEMSIZE];           //!< name of field
        const char *pValues;                            //!< the possible values for field
        char strValue[WEBOFFER_FORMVALUE_MEMSIZE];      //!< the value chosen for the field
        int32_t iMaxWidth;                              //!< the max width of the field
        int32_t iLabelWidth;                            //!< the width of the display label for the field
        int32_t iStyle;                                 //!< the field style
        int8_t bMandatory;                              //!< Is the field Mandatory
    } Fields[WEBOFFER_FORM_NUMFIELDS];                  //!< the fields used in this form
    int32_t iNumFields ;                                //!< num fields used in this form
    int32_t iNumRows;                                   //!< num of rows to render each entry
    WebOfferButtonT Button[WEBOFFER_FORM_NUMBUTTONS];   //!< number of buttons in form
    //!< Type is '^' for submit, '_' for normal
    //!< example: Button[0].strText might be "Submit"
    //!< example: Button[1].strText might be "Cancel"
    WebOfferFixedImageT FixedImage[WEBOFFER_FORM_NUM_FIXED_IMAGE];  //!< maximum number of pictures
} WebOfferFormT;

typedef struct WebOfferMarketplaceT
{
    uint32_t uOfferId;    //!< which marketplace item to display
} WebOfferMarketplaceT;

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create client with passed in buffer.
DIRTYCODE_API WebOfferT *WebOfferCreate(char *pBuffer, int32_t iLength);

// Destroy the instance.
DIRTYCODE_API WebOfferT *WebOfferDestroy(WebOfferT *pOffer);

// Do credit-card related background tasks.
DIRTYCODE_API void WebOfferUpdate(WebOfferT *pOffer);

// Clear the script and reset to idle
DIRTYCODE_API void WebOfferClear(WebOfferT *pOffer);

// Setup information about the user/product
DIRTYCODE_API void WebOfferSetup(WebOfferT *pOffer, WebOfferSetupT *pSetup);

// Execute an offer script
DIRTYCODE_API void WebOfferExecute(WebOfferT *pOffer, const char *pScript);

// Get script result
#define WebOfferResult(pOffer) WebOfferResultData(pOffer, NULL, 0)
DIRTYCODE_API int32_t WebOfferResultData(WebOfferT *pOffer, char *pBuffer, int32_t iLength);

// Return the next command from the script
DIRTYCODE_API WebOfferCommandT *WebOfferCommand(WebOfferT *pOffer);

// Extract an item from a parameter list
DIRTYCODE_API int32_t WebOfferParamList(WebOfferT *pOffer, char *pBuffer, int32_t iLength, const char *pParam, int32_t iIndex);

// Report user input from last command
DIRTYCODE_API void WebOfferAction(WebOfferT *pOffer, int32_t iAction);

// Transfer data to web server
DIRTYCODE_API void WebOfferHttp(WebOfferT *pOffer);

// Check on completion status of transfer
DIRTYCODE_API int32_t WebOfferHttpComplete(WebOfferT *pOffer);

// Return resource header
DIRTYCODE_API int32_t WebOfferResource(WebOfferT *pOffer, WebOfferResourceT *pRsrc, const char *pName);

// Extract the alert data from the current command packet
DIRTYCODE_API void WebOfferGetAlert(WebOfferT *pOffer, WebOfferAlertT *pAlert);

// Extract the busy message for a web transfer
DIRTYCODE_API void WebOfferGetBusy(WebOfferT *pOffer, WebOfferBusyT *pBusy);

// Extract the busy message for a web transfer, providing a default message to use
DIRTYCODE_API void WebOfferGetBusy2(WebOfferT *pOffer, WebOfferBusyT *pBusy, char *strDefaultMessage);

// Extract credit card parms
DIRTYCODE_API void WebOfferGetCredit(WebOfferT *pOffer, WebOfferCreditT *pInfo);

// Store updated credit card data form user
DIRTYCODE_API void WebOfferSetCredit(WebOfferT *pOffer, WebOfferCreditT *pInfo);

// Extract promo screen parms
DIRTYCODE_API void WebOfferGetPromo(WebOfferT *pOffer, WebOfferPromoT *pInfo);

// Store updated promo data form user
DIRTYCODE_API void WebOfferSetPromo(WebOfferT *pOffer, WebOfferPromoT *pInfo);

// Extract menu screen parms
DIRTYCODE_API void WebOfferGetMenu(WebOfferT *pOffer, WebOfferMenuT *pInfo);

// Extract news screen parms
DIRTYCODE_API const char *WebOfferGetNews(WebOfferT *pOffer, WebOfferNewsT *pInfo);

// Extract article list
DIRTYCODE_API void WebOfferGetArticles(WebOfferT *pOffer, WebOfferArticlesT *pInfo);

// Extract textual story
DIRTYCODE_API const char *WebOfferGetStory(WebOfferT *pOffer, WebOfferStoryT *pInfo);

// Extract media story
DIRTYCODE_API const char *WebOfferGetMedia(WebOfferT *pOffer, WebOfferMediaT *pInfo);

// Extract the form data
DIRTYCODE_API void WebOfferGetForm(WebOfferT* pOffer, WebOfferFormT* pInfo);

// Extract the marketplace data
DIRTYCODE_API void WebOfferGetMarketplace(WebOfferT* pOffer, WebOfferMarketplaceT* pInfo);

// Store updated information about FORM
DIRTYCODE_API void WebOfferSetForm(WebOfferT*   pOffer, WebOfferFormT* pInfo);

#ifdef __cplusplus
}
#endif

//@}

#endif  // _weboffer_h_
