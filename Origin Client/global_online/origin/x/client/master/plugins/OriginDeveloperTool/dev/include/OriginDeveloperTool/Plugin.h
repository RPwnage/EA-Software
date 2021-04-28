//  Plugin.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef ORIGIN_DEVELOPER_TOOL_PLUGIN_H
#define ORIGIN_DEVELOPER_TOOL_PLUGIN_H

#include "services/plugin/PluginAPI.h"

#ifdef ORIGIN_PC
#pragma comment(lib, "shell32.lib")
#endif

namespace Origin
{

namespace Plugins
{

namespace DeveloperTool
{

extern "C" Q_DECL_EXPORT int init();
extern "C" Q_DECL_EXPORT int run();
extern "C" Q_DECL_EXPORT int shutdown();

} // namespace DeveloperTool

} // namespace Plugins

} // namespace Origin

#endif // ORIGIN_DEVELOPER_TOOL_PLUGIN_H
