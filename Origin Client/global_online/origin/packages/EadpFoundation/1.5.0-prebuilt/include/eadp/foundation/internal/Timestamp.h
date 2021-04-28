// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/protobuf/Timestamp.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

EADPSDK_API Timestamp getEmptyTimestamp();
EADPSDK_API Timestamp getTimestampForNow(); // UTC
EADPSDK_API String formatTimestamp(Allocator& allocator, Timestamp timestamp); // "yyyy-MM-dd HH:mm:ss"
EADPSDK_API String formatTimestampUTC(Allocator& allocator, Timestamp timestamp); // "yyyy-MM-dd HH:mm:ss"
EADPSDK_API Timestamp convertCtimeToTimestamp(time_t ctime);
EADPSDK_API SharedPtr<protobuf::Timestamp> getProtobufTimestampFromTimestamp(Allocator& allocator, const Timestamp& timestamp);

}
}
}
