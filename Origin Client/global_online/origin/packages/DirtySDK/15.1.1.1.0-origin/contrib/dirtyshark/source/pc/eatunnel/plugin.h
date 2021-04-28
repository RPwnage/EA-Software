// opaque type for commudp packet data
typedef struct EATunnelPacketDataT EATunnelPacketDataT;
// opaque type for commudp packet data
typedef struct CommUDPPacketDataT CommUDPPacketDataT;
// opaque type for netgamelink packet data
typedef struct NetGameLinkPacketDataT NetGameLinkPacketDataT;
// opaque type for netgamedist packet data
typedef struct NetGameDistPacketDataT NetGameDistPacketDataT;
// opaque type for voip packet data
typedef struct VoIPPacketDataT VoIPPacketDataT;

// per-packet data structure for EATunnel packets
typedef struct PluginPacketDataT
{
    EATunnelPacketDataT *pEATunnelData;
    CommUDPPacketDataT *pCommUDPData;
    NetGameLinkPacketDataT *pNetGameLinkData;
    NetGameDistPacketDataT *pNetGameDistData;
    VoIPPacketDataT *pVoIPPacketData;

    packet_info *prev;
    packet_info *next;
} PluginPacketDataT;

// external definitions for plugin.c module
extern gint global_eatunnel_info;
extern gboolean global_eatunnel_colinfo_set;
extern gint global_voip_platform;

// file debugging
extern char *eatunnel_debugfilename;
void eatunnel_debug_printf(const gchar *fmt, ...);
void eatunnel_debug_setup(void);