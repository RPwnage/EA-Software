/*H********************************************************************************/
/*!
    \File SampleCore.h

    \Description
        A simple test harness for exercising high level modal screen code

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 03/14/2004 (gschaefer) First Version
*/

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

HWND DialogCreate(const char *pResource);
HWND DialogDestroy(HWND pHandle);
int32_t DialogCheckButton(HWND pHandle);
int32_t DialogWaitButton(HWND pHandle);
void DialogComboInit(HWND pHandle, int32_t iItem, const char *pList);
void DialogComboAdd(HWND pHandle, int32_t iItem, const char *pAdd);
void DialogComboSelect(HWND pHandle, int32_t iItem, const char *pSelect);
void DialogComboQuery(HWND pHandle, int32_t iItem, const char *pDataBuf, int32_t iDataLen);
void DialogQuit(void);
void DialogColor(HWND pHandle, int32_t iItem, uint32_t uRGB);
void DialogButtonCenter(HWND pHandle, int32_t *pButtons, int32_t iCount);
void DialogImageSet(HWND pHandle, int32_t iItem, int32_t iWidth, int32_t iHeight, unsigned char *pPalette, unsigned char *pBitmap);
void DialogImageSet2(HWND pHandle, int32_t iItem, unsigned char *pBitmap, int32_t iWidth, int32_t iHeight);




