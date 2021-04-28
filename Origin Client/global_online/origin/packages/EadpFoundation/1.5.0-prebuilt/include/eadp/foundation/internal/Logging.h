// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/LoggingService.h>

// !DEPRECATED as bulk build conflict with TAG macro usage.
// define a group of little convenient macros for EADP-SDK internal usage
// ensure you have getHub() and TAG defined: getHub() should return IHub*, TAG is the string for logging name
// #define LOGV(...) EADPSDK_LOGV(getHub(), TAG, __VA_ARGS__)
// #define LOGD(...) EADPSDK_LOGD(getHub(), TAG, __VA_ARGS__)
// #define LOGI(...) EADPSDK_LOGI(getHub(), TAG, __VA_ARGS__)
// #define LOGW(...) EADPSDK_LOGW(getHub(), TAG, __VA_ARGS__)
// #define LOGE(...) EADPSDK_LOGE(getHub(), TAG, __VA_ARGS__)
// #define LOGF(...) EADPSDK_LOGF(getHub(), TAG, __VA_ARGS__)
