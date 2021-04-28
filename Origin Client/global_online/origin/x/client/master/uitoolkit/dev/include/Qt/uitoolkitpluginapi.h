#ifndef __UITOOLKIT_API_H_INCLUDED_
#define __UITOOLKIT_API_H_INCLUDED_

#include <qcompilerdetection.h>

#if !defined(UITOOLKIT_PLUGIN_API)
#if defined(ORIGIN_PLUGIN)
#define UITOOLKIT_PLUGIN_API Q_DECL_IMPORT
#else
#define UITOOLKIT_PLUGIN_API Q_DECL_EXPORT
#endif
#endif

#endif // __UITOOLKIT_API_H_INCLUDED_