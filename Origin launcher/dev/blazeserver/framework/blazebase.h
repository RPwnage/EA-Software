///*************************************************************************************************/
///*!
//    \file blazebase.h
//
//    Common header file used by the Blaze framework and all Blaze components.
//
//    \attention
//        (c) Electronic Arts. All Rights Reserved.
//*/
///*************************************************************************************************/
//
#pragma once

#include "framework/blazedefines.h"

//These next two lines HAVE to come before everything else to ensure any TDFs have the right 
//memory group built in.
#include "framework/system/allocation.h"

#include "EATDF/tdfobjectid.h"
#include "EATDF/time.h"
#include "EATDF/tdfbase.h"

#include "blazerpcerrors.h"

namespace Blaze
{
    typedef EA::TDF::EntityId BlazeId;
    using EA::TDF::ComponentId;
    using EA::TDF::EntityType;
    using EA::TDF::EntityId;
    using EA::TDF::TimeValue;
}
//
//#include "framework/logger.h"
//#include "framework/system/timerqueue.h"
//
//// Replaced blanket inclusion of remotereplicatedmap.h with just the
//// selector, blazerpc, and fibermanager, which were previously included indirectly from 
//// remoteReplicatedMap.h.  As of this witting, we can technically
//// build without these includes, but we are leaving these in
//// to prevent custom code build failures which will likely result from removal.
//#include "framework/connection/selector.h"
//#include "framework/component/blazerpc.h"
//#include "framework/system/fibermanager.h"
//
//#include "framework/util/shared/blazestring.h"
//#include "framework/usersessions/usersession.h"
//#include "framework/connection/session.h"
//
//
//#include "framework/component/command.h"
//#include "framework/component/component.h"
//
//#include "blazerpcerrors.h"
//#include "framework/component/blazerpc.h"
//
namespace Blaze
{
    // This struct acts as a BlazeRpcError, but has the added benefit of being able to get passed
    // to the BLAZE_LOG macros directly, and the actual error name is logged - rather than the ordinal
    // value of the BlazeRpcError int32_t.
    struct BlazeError
    {
        BlazeRpcError code;

        inline BlazeError() : code(ERR_OK) {}
        inline BlazeError(const BlazeError& be) : code(be.code) {}
        inline BlazeError(const BlazeRpcError& bre) : code(bre) {}

        inline bool operator==(const BlazeError& be) const { return (code == be.code); }
        inline bool operator==(const BlazeRpcError& bre) const { return (code == bre); }
        inline bool operator!=(const BlazeError& be) const { return (code != be.code); }
        inline bool operator!=(const BlazeRpcError& bre) const { return (code != bre); }

        inline operator const BlazeRpcError&() const { return code; }

        const char8_t* c_str() const { return ErrorHelp::getErrorName(code); }
    };
    
}

