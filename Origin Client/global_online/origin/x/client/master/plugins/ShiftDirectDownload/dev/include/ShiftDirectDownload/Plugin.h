//  Plugin.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef SHIFT_DIRECT_DOWNLOAD_PLUGIN_H
#define SHIFT_DIRECT_DOWNLOAD_PLUGIN_H

#ifdef ORIGIN_PC
#pragma comment(lib, "shell32.lib")
#endif

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

extern "C" Q_DECL_EXPORT int init();
extern "C" Q_DECL_EXPORT int run();
extern "C" Q_DECL_EXPORT int shutdown();

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // SHIFT_DIRECT_DOWNLOAD_PLUGIN_H
