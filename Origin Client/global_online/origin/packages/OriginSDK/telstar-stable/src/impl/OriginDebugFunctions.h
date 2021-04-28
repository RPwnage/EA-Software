#ifndef __ORIGIN_DEBUG_FUNCTIONS_H__
#define __ORIGIN_DEBUG_FUNCTIONS_H__

#include "OriginSDK/OriginTypes.h"

#define REPORTDEBUG(ERR, MSG) Origin::ReportDebug(ERR, MSG)

namespace Origin {

OriginErrorT ReportDebug (OriginErrorT err, const char* pMsg);

}; // namespace Origin

#endif // __ORIGIN_DEBUG_FUNCTIONS_H__