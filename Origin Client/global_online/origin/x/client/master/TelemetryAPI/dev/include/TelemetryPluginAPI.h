#ifndef __TELEMETRY_API_H_INCLUDED_
#define __TELEMETRY_API_H_INCLUDED_

#if !defined(TELEMETRY_PLUGIN_API)
#  if defined(WIN32)
#    if defined(ORIGIN_PLUGIN)
#      define TELEMETRY_PLUGIN_API __declspec(dllimport)
#    else
#      define TELEMETRY_PLUGIN_API __declspec(dllexport)
#    endif
#  else
#    define TELEMETRY_PLUGIN_API __attribute__((visibility("default")))
#  endif
#endif

#endif // __TELEMETRY_API_H_INCLUDED_