#undef _WIN32_WINNT //$$TODO - get build to not include this
#define MSC_VER_REQUIRED 1700 //$$TODO - where should this be specified?

#include <stdio.h>

#include "config.h"

#include <gmodule.h>

#include "moduleinfo.h"

/* plugins are DLLs */
#define WS_BUILD_DLL
#include "ws_symbol_export.h"

#ifndef ENABLE_STATIC
WS_DLL_PUBLIC_DEF void plugin_register (void);
WS_DLL_PUBLIC_DEF const gchar version[] = VERSION;

// prototunnel dissector in packet-eatunnel.c
extern void proto_register_eatunnel(void);
extern void proto_reg_handoff_eatunnel(void);

// commudp dissector in packet-commudp.c
extern void proto_register_commudp(void);
extern void proto_reg_handoff_commudp(void);

// netgamelink dissector in packet-netgamelink.c
extern void proto_register_netgamelink(void);
extern void proto_reg_handoff_netgamelink(void);

// netgamedist dissector in packet-netgamedist.c
extern void proto_register_netgamedist(void);
extern void proto_reg_handoff_netgamedist(void);

// voip dissector in packet-voip.c
extern void proto_register_voip(void);
extern void proto_reg_handoff_voip(void);

// global preference variables
gint global_eatunnel_info;
gboolean global_eatunnel_colinfo_set;
gint global_voip_platform;

static FILE *eatunnel_debug_file = NULL;

char *eatunnel_debugfilename = NULL;

/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_DEF void plugin_register(void)
{
    { proto_register_eatunnel(); }
    { proto_register_commudp(); }
    { proto_register_netgamelink(); }
    { proto_register_netgamedist(); }
    { proto_register_voip(); }
}

WS_DLL_PUBLIC_DEF void plugin_reg_handoff(void)
{
    { proto_reg_handoff_eatunnel(); }
    { proto_reg_handoff_commudp(); }
    { proto_reg_handoff_netgamelink(); }
    { proto_reg_handoff_netgamedist(); }
    { proto_reg_handoff_voip(); }
}
#endif


void eatunnel_debug_printf(const gchar *fmt, ...)
{
    va_list ap;

    if (!eatunnel_debug_file)
    {
        return;
    }

    va_start(ap, fmt);
    vfprintf(eatunnel_debug_file, fmt, ap);
    va_end(ap);

    // always flush, figuring out when to do it smartly in the future
    fflush(eatunnel_debug_file);
}

void eatunnel_debug_setup(void)
{
    if (eatunnel_debug_file != NULL)
    {
        return;
    }
    if ((eatunnel_debugfilename == NULL) || (*eatunnel_debugfilename == ' '))
    {
        return;
    }
    eatunnel_debug_file = fopen(eatunnel_debugfilename, "w");

    eatunnel_debug_printf("Wireshark EATunnel debug log\n\n");
}



