#ifndef __WEBWIDGET_API_H_INCLUDED_
#define __WEBWIDGET_API_H_INCLUDED_

#include <qcompilerdetection.h>

#if !defined(WEBWIDGET_PLUGIN_API)
#if defined(ORIGIN_PLUGIN)
#define WEBWIDGET_PLUGIN_API Q_DECL_IMPORT
#else
#define WEBWIDGET_PLUGIN_API Q_DECL_EXPORT
#endif
#endif

#endif // __WEBWIDGET_API_H_INCLUDED_