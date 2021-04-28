///////////////////////////////////////////////////////////////////////////////
// PluginAPI.h
//
// Copyright (c) 2014 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __PLUGIN_API_H_INCLUDED_
#define __PLUGIN_API_H_INCLUDED_

#include <qcompilerdetection.h>

#if !defined(ORIGIN_PLUGIN_API)
#if defined(ORIGIN_PLUGIN)
#define ORIGIN_PLUGIN_API Q_DECL_IMPORT
#else
#define ORIGIN_PLUGIN_API Q_DECL_EXPORT
#endif
#endif

#endif // __PLUGIN_API_H_INCLUDED_