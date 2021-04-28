/*H********************************************************************************/
/*!
    \File logger.h

    \Description
        Logger for DirtyCast server

    \Copyright
        Copyright (c) 2007-2010 Electronic Arts Inc.

    \Version 08/03/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _logger_h
#define _logger_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/
#define LOGGER_EXIT_ERROR 2
#define LOGGER_EXIT_OK 0

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

//! opaque module state
typedef struct LoggerRefT LoggerRefT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the singleton logger module
LoggerRefT *LoggerCreate(int32_t iArgc, const char *pArgv[], uint8_t bBlocking);

// destroy the logger module
void LoggerDestroy(LoggerRefT *pLoggerRef);

// write specified output to log file (iTextLen may be -1 for auto-calc)
int32_t LoggerWrite(LoggerRefT *pLoggerRef, const char *pText, int32_t iTextLen);

#ifdef __cplusplus
}
#endif

#endif // _logger_h

