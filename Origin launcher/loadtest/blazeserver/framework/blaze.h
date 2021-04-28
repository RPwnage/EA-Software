/*************************************************************************************************/
/*!
    \file blaze.h

    Common header file used by the Blaze framework and all Blaze components.

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#pragma once

#include "framework/blazebase.h"

#include <math.h>
#include <regex>

#include "EAJson/JsonDomReader.h"
#include "EAJson/JsonWriter.h"

#include "EAStdC/EACType.h"
#include "EAStdC/EAFixedPoint.h"
#include "EAStdC/EAHashCRC.h"
#include "EAStdC/EAProcess.h"
#include "EAStdC/EATextUtil.h"

#include "EASTL/bitvector.h"
#include "EASTL/numeric.h"
#include "EASTL/safe_ptr.h"
#include "EASTL/scoped_array.h"
#include "EASTL/scoped_ptr.h"
#include "EASTL/stack.h"

#include "EASTL/vector_multimap.h"

#include "EATDF/tdf.h"

#include "eathread/eathread_callstack.h"
#include "eathread/eathread_rwmutex.h"
#include "eathread/shared_ptr_mt.h"



#include "hiredis/async.h"
#include "hiredis/hiredis.h"

#include "framework/logger.h"
#include "framework/system/timerqueue.h"

#include "framework/connection/selector.h"
#include "framework/component/blazerpc.h"
#include "framework/system/fibermanager.h"

#include "framework/util/shared/blazestring.h"
#include "framework/usersessions/usersession.h"
#include "framework/connection/session.h"


#include "framework/component/command.h"
#include "framework/component/component.h"

#ifdef BLAZE_PCH_FORCE_INCLUDE_HEADER
  #include BLAZE_PCH_FORCE_INCLUDE_HEADER
#endif

