#include "stdafx.h"
#include "OriginSDK/OriginSDK.h"
#include "OriginSDK/OriginError.h"
#include "OriginDebugFunctions.h"
#include "OriginErrorFunctions.h"
#include <stdio.h>

static OriginErrorCallback gErrorCallback = NULL;
static void * gErrorContext = NULL;

void ORIGIN_SDK_API OriginRegisterErrorCallback(OriginErrorCallback callback, void * pcontext)
{
	gErrorContext = pcontext;
	gErrorCallback = callback;
}

namespace Origin {

    const char* GetErrorDescription(OriginErrorT err)
    {
        ErrorRecord *pRecord = GetErrorInfo(err);

        if (pRecord)
        {
            return pRecord->description;
        }
        else
        {
            return "No additional error information available";
        }
    }

    OriginErrorT ReportError(OriginErrorT err, const char* msg, const char* file, int line, const char* funcname)
    {
		char buffer[2048];
        int error;

#ifdef ORIGIN_PC
        error = GetLastError();
#elif defined ORIGIN_MAC
        error = 0;
#endif
        sprintf(buffer, "%s(%d) %s Origin Error: 0x%08X; Last OS Error: 0x%08x; %s\n", file, line, funcname, err, error, msg);

        if (err != ORIGIN_SUCCESS)
        {
            if (gErrorCallback != NULL)
                gErrorCallback(gErrorContext, err, msg, file, line);
            REPORTDEBUG(err, buffer);
        }
        else
        {
            REPORTDEBUG(ORIGIN_LEVEL_3, buffer);
        }

        return err;
    }

}; // namespace Origin
