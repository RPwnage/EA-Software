/*H********************************************************************************/
/*!
    \File configfile.h

    \Description
        Configuration file processor which supports simple substitution and conditional
        actions as well as auto-reload and file/http include.

    \Copyright
        Copyright (c) Electronic Arts 4003. ALL RIGHTS RESERVED.

    \Version 1.0 01/20/2004 (gschaefer) First Version
*/
/********************************************************************************H*/

#ifndef _configfile_h
#define _configfile_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define CONFIGFILE_STAT_BUSY         1           // busy loading file
#define CONFIGFILE_STAT_DONE         0           // no error
#define CONFIGFILE_STAT_OPENERR     -1           // file open error
#define CONFIGFILE_STAT_READERR     -2           // file read error

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct ConfigFileT ConfigFileT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Setup state for config file module
ConfigFileT *ConfigFileCreate(void);

// Free resources and destroy module
ConfigFileT *ConfigFileDestroy(ConfigFileT *pConfig);

// Reset to base state (no config data loaded)
void ConfigFileClear(ConfigFileT *pConfig);

// Reset to base state (no config data loaded)
void ConfigFileLoad(ConfigFileT *pConfig, const char *pUrl, const char *pEnv);

// Reset to base state (no config data loaded)
void ConfigFileUpdate(ConfigFileT *pConfig);

// Blocking wait for completion
void ConfigFileWait(ConfigFileT *pConfig);

// Return error string
const char *ConfigFileError(ConfigFileT *pConfig);

// Perform control/status updated on configfile channel
int32_t ConfigFileControl(ConfigFileT *pConfig, int32_t iSelect, int32_t iParm, void *pData);

// Return current loading status (legacy -- now implemented via ConfigFileControl)
#define ConfigFileStatus(pConfig) ConfigFileControl(pConfig, 'stat', 0, NULL)

// Return loaded data
const char *ConfigFileData(ConfigFileT *pConfig, const char *pSection);

// Do immediate load (blocking until completion)
char *ConfigFileImmed(const char *pUrl, const char *pEnv);

// Import data into a config file section
void ConfigFileImport(ConfigFileT *pConfig, const char *pName, const char *pData);

// Return the data that was loaded by section index (0=main)
const char *ConfigFileIndex(ConfigFileT *pConfig, int32_t iIndex, const char **pName);

#ifdef __cplusplus
};
#endif

#endif // _configfile_h
