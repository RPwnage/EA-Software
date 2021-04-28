#ifndef __ORIGIN_ERROR_FUNCTIONS_H__
#define __ORIGIN_ERROR_FUNCTIONS_H__

#include "OriginSDK/OriginTypes.h"
#include "OriginSDK/OriginSDK.h"

namespace Origin {
    template <typename T> static void deleteIfHeap(T& /*var*/) { }
    template <typename T> static void deleteIfHeap(T* var) { delete var; }

    struct ErrorRecord
    {
        const char * name;
        const char * area;
        const char * description;
    };



#define REPORTERROR(ERR) (Origin::ReportError(ERR, Origin::GetErrorDescription(ERR), __FILE__, __LINE__, __FUNCTION__))
#define REPORTERRORMESSAGE(ERR, MSG) Origin::ReportError(ERR, MSG, __FILE__, __LINE__)
#define FILL_REQUEST_CHECK(FUNCTION_TO_CHECK) OriginErrorT fillError = (FUNCTION_TO_CHECK); if(fillError != ORIGIN_SUCCESS) {Origin::deleteIfHeap(req);return REPORTERROR(fillError);}

    const char* GetErrorDescription(OriginErrorT err);

    OriginErrorT ReportError (OriginErrorT err, const char* msg, const char* file, int line, const char* funcname);


    ErrorRecord * GetErrorInfo(OriginErrorT err);

}; // namespace Origin

#endif //__ORIGIN_ERROR_FUNCTIONS_H__