#undef _WIN32_WINNT //$$TODO - get build to not include this
#define MSC_VER_REQUIRED 1700 //$$TODO - where should this be specified?

#include <config.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/prefs-int.h>

#include "DirtySDK/game/netgamepkt.h"

#include "plugin.h"

static int proto_netgamelink = -1;

static dissector_handle_t netgamelink_handle;

static int hf_gamelink_kind = -1;
static int hf_gamelink_data = -1;

static int hf_gamelink_strm = -1;
static int hf_gamelink_strm_idnt = -1;
static int hf_gamelink_strm_kind = -1;
static int hf_gamelink_strm_size = -1;
static int hf_gamelink_strm_data = -1;

static int hf_gamelink_sync = -1;
static int hf_gamelink_sync_repl = -1;
static int hf_gamelink_sync_echo = -1;
static int hf_gamelink_sync_late = -1;
static int hf_gamelink_sync_psnt = -1;
static int hf_gamelink_sync_prcvd = -1;
static int hf_gamelink_sync_plost = -1;
static int hf_gamelink_sync_nsnt = -1;
static int hf_gamelink_sync_flags = -1;
static int hf_gamelink_sync_size = -1;

static gint ett_netgamelink = -1;
static gint ett_netgamelink_stream = -1;
static gint ett_netgamelink_sync = -1;
static gint ett_netgamedist = -1;

static dissector_handle_t netgamedist_dissector_handle;

static char *netgamelink_data_dissector = NULL;

static module_t *netgamelink_module;

static const value_string gamelink_seq_names[] =
{
    { GAME_PACKET_INPUT,            "GAME_PACKET_INPUT"            },
    { GAME_PACKET_INPUT_MULTI,      "GAME_PACKET_INPUT_MULTI"      },
    { GAME_PACKET_STATS,            "GAME_PACKET_STATS"            },
    { GAME_PACKET_USER,             "GAME_PACKET_USER"             },
    { GAME_PACKET_USER_UNRELIABLE,  "GAME_PACKET_USER_UNRELIABLE"  },
    { GAME_PACKET_USER_BROADCAST,   "GAME_PACKET_USER_BROADCAST"   },
    { GAME_PACKET_INPUT_FLOW,       "GAME_PACKET_INPUT_FLOW"       },
    { GAME_PACKET_INPUT_MULTI_FAT,  "GAME_PACKET_INPUT_MULTI_FAT"  },
    { GAME_PACKET_INPUT_META,       "GAME_PACKET_INPUT_META"       },
    { GAME_PACKET_LINK_STRM,        "GAME_PACKET_LINK_STRM"        },
    { GAME_PACKET_SYNC,             "GAME_PACKET_SYNC"             },
    { 0xffffffff, NULL }
};

// netgamelink dissector
static void dissect_netgamelink(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 1))
    {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "NetGameLink");
    }

    // are we being asked for details?
    if (tree != NULL)
    {
        proto_tree *netgamelink_tree = NULL;
        proto_item *ti = NULL;
        guint32 decode_offset = 0;
        guint32 packet_length, payload_length;
        guint8 sync, kind;
        NetGamePacketSyncT SyncData;

        // add netgamelink tree item
        ti = proto_tree_add_item(tree, proto_netgamelink, tvb, decode_offset, -1, ENC_NA);
        packet_length = payload_length = ti->finfo->length;
        proto_item_append_text(ti, " (size=%d)", packet_length);

        // add subtree for dissected data
        netgamelink_tree = proto_item_add_subtree(ti, ett_netgamelink);

        // get sync/kind
        kind = tvb_get_guint8(tvb, packet_length-1);
        sync = kind & GAME_PACKET_SYNC;
        if (kind != sync)
        {
            kind &= ~GAME_PACKET_SYNC;
        }

        // subtract size of sync data from payload
        if (sync != 0)
        {
            payload_length -= sizeof(SyncData);
        }
        // subtract size of kind from payload
        payload_length -= 1;

        // add subtree item for payload
        if (payload_length > 0)
        {
            // process payload
            if (kind == GAME_PACKET_LINK_STRM)
            {
                proto_tree *netgamestream_tree;
                guint32 fourcc_code;
                char fourcc_str[5];

                // add subtree item and tree for stream packet
                proto_item *ti2 = proto_tree_add_item(netgamelink_tree, hf_gamelink_strm, tvb, decode_offset, 0, ENC_NA);
                netgamestream_tree = proto_item_add_subtree(ti2, ett_netgamelink_stream);

                // add subtree item for stream ident
                ti2 = proto_tree_add_item(netgamestream_tree, hf_gamelink_strm_idnt, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                fourcc_code = tvb_get_ntohl(tvb, decode_offset);
                ds_snzprintf(fourcc_str, sizeof(fourcc_str), "%C", fourcc_code);
                proto_item_append_text(ti2, ", ('%s')", fourcc_str);
                decode_offset += 4;

                // add subtree item for stream kind
                ti2 = proto_tree_add_item(netgamestream_tree, hf_gamelink_strm_kind, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                fourcc_code = tvb_get_ntohl(tvb, decode_offset);
                ds_snzprintf(fourcc_str, sizeof(fourcc_str), "%C", fourcc_code);
                proto_item_append_text(ti2, ", ('%s')", fourcc_str);
                decode_offset += 4;

                // add subtree item for stream size
                proto_tree_add_item(netgamestream_tree, hf_gamelink_strm_size, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                decode_offset += 4;

                // add subtree item for stream data
                proto_tree_add_item(netgamelink_tree, hf_gamelink_strm_data, tvb, decode_offset, payload_length-decode_offset, ENC_NA);
            }
            else if ((kind == GAME_PACKET_USER) || (kind == GAME_PACKET_USER_UNRELIABLE) || (kind == GAME_PACKET_USER_BROADCAST))
            {
                // create new tvb buffer to hold netgamelink data
                tvbuff_t *netgamecust_tvb = tvb_new_subset(tvb, decode_offset, payload_length, payload_length);
                // get custom dissector or use
                dissector_handle_t netgamelink_dissector = find_dissector((*netgamelink_data_dissector != '\0') ? netgamelink_data_dissector : "data");
                // store packet kind in pinfo
                pinfo->private_data = (void *)(uintptr_t)kind;
                // call netgamelink dissector
                call_dissector(netgamelink_dissector, netgamecust_tvb, pinfo, tree);
            }
            else if ((kind == GAME_PACKET_INPUT) || (kind == GAME_PACKET_INPUT_MULTI) || (kind == GAME_PACKET_INPUT_FLOW) ||
                     (kind == GAME_PACKET_INPUT_MULTI_FAT) || (kind == GAME_PACKET_INPUT_META) || (kind == GAME_PACKET_STATS))
            {
                tvbuff_t *netgamedist_tvb;
                proto_tree *data_tree;
                // add tree for packet data
                data_tree = proto_item_add_subtree(netgamelink_tree, ett_netgamedist);
                // add subtree item for data header
                ti = proto_tree_add_item(data_tree, hf_gamelink_data, tvb, decode_offset, payload_length, ENC_NA);
                // create new tvb buffer to hold netgamedist data
                netgamedist_tvb = tvb_new_subset(tvb, decode_offset, payload_length, payload_length);
                // store packet kind in pinfo
                pinfo->private_data = (void *)(uintptr_t)kind;
                // call netgamedist dissector
                call_dissector(netgamedist_dissector_handle, netgamedist_tvb, pinfo, tree);
            }
            else
            {
                ti = proto_tree_add_item(netgamelink_tree, hf_gamelink_data, tvb, decode_offset, payload_length, ENC_BIG_ENDIAN);
            }
        }

        // add subtree for sync
        if (sync != 0)
        {
            guint32 sync_offset = packet_length-sizeof(SyncData)-1;
            proto_tree *sync_tree = NULL;
            proto_item *ti2 = NULL;

            // add sync subtree
            ti2 = proto_tree_add_item(netgamelink_tree, hf_gamelink_sync, tvb, sync_offset, sizeof(SyncData), ENC_BIG_ENDIAN);
            tvb_memcpy(tvb, &SyncData, sync_offset, sizeof(SyncData));

            // add subtree for dissected data
            sync_tree = proto_item_add_subtree(ti2, ett_netgamelink_sync);

            // add repl
            proto_tree_add_item(ti2, hf_gamelink_sync_repl, tvb, sync_offset, 4, ENC_BIG_ENDIAN);
            sync_offset += 4;
            proto_item_append_text(ti2, ", (repl=%u)", g_ntohl(SyncData.repl));

            // add echo
            proto_tree_add_item(ti2, hf_gamelink_sync_echo, tvb, sync_offset, 4, ENC_BIG_ENDIAN);
            sync_offset += 4;
            proto_item_append_text(ti2, ", (echo=%u)", g_ntohl(SyncData.echo));

            // add late
            proto_tree_add_item(ti2, hf_gamelink_sync_late, tvb, sync_offset, 2, ENC_BIG_ENDIAN);
            sync_offset += 2;
            proto_item_append_text(ti2, ", (late=%d)", g_ntohs(SyncData.late));

            // add psnt
            proto_tree_add_item(ti2, hf_gamelink_sync_psnt, tvb, sync_offset, 1, ENC_BIG_ENDIAN);
            sync_offset += 1;
            proto_item_append_text(ti2, ", (psnt=%d)", SyncData.psnt);

            // add prcvd
            proto_tree_add_item(ti2, hf_gamelink_sync_prcvd, tvb, sync_offset, 1, ENC_BIG_ENDIAN);
            sync_offset += 1;
            proto_item_append_text(ti2, ", (prcvd=%d)", SyncData.prcvd);

            // add plost
            proto_tree_add_item(ti2, hf_gamelink_sync_plost, tvb, sync_offset, 1, ENC_BIG_ENDIAN);
            sync_offset += 1;
            proto_item_append_text(ti2, ", (plost=%d)", SyncData.plost);

            // add nsnt
            proto_tree_add_item(ti2, hf_gamelink_sync_nsnt, tvb, sync_offset, 1, ENC_BIG_ENDIAN);
            sync_offset += 1;
            proto_item_append_text(ti2, ", (nsnt=%d)", SyncData.nsnt);

            // add flags
            proto_tree_add_item(ti2, hf_gamelink_sync_flags, tvb, sync_offset, 1, ENC_BIG_ENDIAN);
            sync_offset += 1;
            proto_item_append_text(ti2, ", (flags=%d)", SyncData.flags);

            // add size
            proto_tree_add_item(ti2, hf_gamelink_sync_size, tvb, sync_offset, 1, ENC_BIG_ENDIAN);
            sync_offset += 1;
             proto_item_append_text(ti2, ", (size=%d)", SyncData.size);
        }

        // add subtree item for kind
        ti = proto_tree_add_item(netgamelink_tree, hf_gamelink_kind, tvb, packet_length-1, 1, ENC_BIG_ENDIAN);
        // add kind name
        proto_item_append_text(ti, ", (%s)", val_to_str(kind, gamelink_seq_names, "unknown"));

        // add summary info
        if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 1))
        {
            col_add_fstr(pinfo->cinfo, COL_INFO, "[%s] data=%d", val_to_str(kind, gamelink_seq_names, "unknown"), payload_length);
            global_eatunnel_colinfo_set = TRUE;
        }
    }
}

void proto_register_netgamelink(void)
{
    static hf_register_info hf[] =
    {
        { &hf_gamelink_strm,        { "strm",  "gamelink.strm",       FT_NONE,   BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_strm_idnt,   { "idnt",  "gamelink.strm.idnt",  FT_UINT32, BASE_HEX,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_strm_kind,   { "kind",  "gamelink.strm.kind",  FT_UINT32, BASE_HEX,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_strm_size,   { "size",  "gamelink.strm.size",  FT_UINT32, BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_strm_data,   { "data",  "gamelink.strm.data",  FT_BYTES,  BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync,        { "sync",  "gamelink.sync",       FT_NONE,   BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_repl,   { "repl",  "gamelink.sync.repl",  FT_UINT32, BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_echo,   { "echo",  "gamelink.sync.echo",  FT_UINT32, BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_late,   { "late",  "gamelink.sync.late",  FT_UINT16, BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_psnt,   { "psnt",  "gamelink.sync.psnt",  FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_prcvd,  { "prcvd", "gamelink.sync.prcvd", FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_plost,  { "plost", "gamelink.sync.plost", FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_nsnt,   { "nsnt",  "gamelink.sync.nsnt",  FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_flags,  { "flags", "gamelink.sync.flags", FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_sync_size,   { "size",  "gamelink.sync.size",  FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_data,        { "data",  "gamelink.data",       FT_BYTES,  BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_gamelink_kind,        { "kind",  "gamelink.kind",       FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
    };
    static gint *ett[] =
    {
        &ett_netgamelink,
        &ett_netgamelink_stream,
        &ett_netgamelink_sync,
        &ett_netgamedist,
    };

    // register our protocol
    proto_netgamelink = proto_register_protocol("NetGameLink Protocol", "NetGameLink", "gamelink");
    // register protocol preferences (options)
    netgamelink_module = prefs_register_protocol_subtree("eatunnel", proto_netgamelink, NULL);
    // register dissector (this allows commudp to call us)
    netgamelink_handle = register_dissector("netgamelink", dissect_netgamelink, proto_netgamelink);
    // register sub-dissector preference
    prefs_register_string_preference(netgamelink_module, "data_dissector", "NetGameLink Data Dissector",
         "Set dissector for NetGameLink data", &netgamelink_data_dissector);
    // register hf/ett
    proto_register_field_array(proto_netgamelink, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void proto_reg_handoff_netgamelink(void)
{
    static dissector_handle_t netgamelink_handle;

    netgamelink_handle = create_dissector_handle(dissect_netgamelink, proto_netgamelink);
    
    netgamedist_dissector_handle = find_dissector("netgamedist");
}
