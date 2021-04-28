#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginDebugFunctions.h"

static	enumDebugLevel	gDebugSeverityLevel = ORIGIN_LEVEL_MINOR;	///< Severity of debug messages reported
static	PrintfHook		gDebugPrintHook		= NULL;					///< Hook to debug reporting callback function
static	void *			gDebugContext		= NULL;					///< Debug context.

void ORIGIN_SDK_API OriginEnableDebug(enumDebugLevel severity)
{
	gDebugSeverityLevel = severity;
}

void ORIGIN_SDK_API OriginHookDebug(PrintfHook pPrintHook, void *pContext)
{
	gDebugPrintHook	= pPrintHook;
	gDebugContext	= pContext;
}

namespace Origin
{

OriginErrorT ReportDebug (OriginErrorT err, const char* pMsg)
{
	// This compares two different enumerations. OriginErrorT uses a left shifted severity level of enumDebugLevel.
	int level = (err & ORIGIN_ERROR_LEVEL_MASK)>>ORIGIN_ERROR_LEVEL_SHIFT;
	if (gDebugPrintHook && (level < gDebugSeverityLevel))
	{
		gDebugPrintHook(gDebugContext, pMsg);
	}
	return ORIGIN_SUCCESS;
}

}; // namespace Origin