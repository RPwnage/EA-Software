#undef _WIN32_WINNT //$$TODO - get build to not include this
#define MSC_VER_REQUIRED 1700 //$$TODO - where should this be specified?

#include <config.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/prefs-int.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/game/netgamedist.h"
#include "DirtySDK/game/netgamepkt.h"

#include "plugin.h"

static int proto_netgamedist = -1;

static dissector_handle_t netgamedist_handle;

static int hf_gamedist_input = -1;
static int hf_gamedist_input_type = -1;
static int hf_gamedist_input_data = -1;

static int hf_gamedist_minput = -1;
static int hf_gamedist_minput_data = -1;
static int hf_gamedist_minput_delta = -1;
static int hf_gamedist_minput_vers = -1;
static int hf_gamedist_minput_count = -1;
static int hf_gamedist_minput_inputs = -1;
static int hf_gamedist_minput_inputs_data = -1;

static int hf_gamedist_meta = -1;
static int hf_gamedist_meta_vers = -1;
static int hf_gamedist_meta_mask = -1;

static int hf_gamedist_flow = -1;
static int hf_gamedist_flow_sendremote = -1;
static int hf_gamedist_flow_recvremote = -1;
static int hf_gamedist_flow_crcvalid = -1;
static int hf_gamedist_flow_crcdata = -1;

static int hf_gamedist_stat = -1;
static int hf_gamedist_stat_ping = -1;
static int hf_gamedist_stat_late = -1;
static int hf_gamedist_stat_bps = -1;
static int hf_gamedist_stat_pps = -1;

static gint ett_netgamedist = -1;
static gint ett_netgamedist_meta = -1;
static gint ett_netgamedist_flow = -1;
static gint ett_netgamedist_stat = -1;

static gint ett_netgamedist_input = -1;
static gint ett_netgamedist_minput = -1;
static gint ett_netgamedist_minput_inputs = -1;

static module_t *netgamedist_module;

static const value_string gamedist_data_names[] =
{
    { GMDIST_DATA_NONE,             "GMDIST_DATA_NONE"              },
    { GMDIST_DATA_INPUT,            "GMDIST_DATA_INPUT"             },
    { GMDIST_DATA_INPUT_DROPPABLE,  "GMDIST_DATA_INPUT_DROPPABLE"   },
    { GMDIST_DATA_DISCONNECT,       "GMDIST_DATA_DISCONNECT"        },
    { GMDIST_DATA_NODATA,           "GMDIST_DATA_NODATA"            },
    { 0xffffffff, NULL }
};

//! info from netgamedist.c used to decode input multipackets

#define GMDIST_META_ARRAY_SIZE          (32)    // how many past versions of sparse multipacket to keep

//! Describes one version of multipacket.
typedef struct NetGameDistMetaInfoT
{
    //! which entries are used
    uint32_t uMask;
    //! number of players used
    uint8_t uPlayerCount;
    //! version number
    uint8_t uVer;
} NetGameDistMetaInfoT;

    //! Received meta information about sparse multi-packets (wraps around)
static NetGameDistMetaInfoT meta_info[GMDIST_META_ARRAY_SIZE] = { 0 };
static gint                 meta_vers = 0;

static uint32_t _NetGameDistCountBits(uint32_t uMask)
{
    uint32_t uCount = 0;

    while(uMask)
    {
        uCount += (uMask & 1);
        uMask >>= 1;
    };

    return(uCount);
}

// netgamedist dissector
static void dissect_netgamedist(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 1))
    {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "NetGameDist");
    }

    // are we being asked for details?
    if (tree != NULL)
    {
        proto_tree *netgamedist_tree, *netgamedist_tree2, *netgamedist_tree3;
        proto_item *ti, *ti2;
        guint32 decode_offset = 0;
        guint32 packet_length, payload_length;
        guint32 kind, type;

        // add netgamedist tree item
        ti = proto_tree_add_item(tree, proto_netgamedist, tvb, decode_offset, -1, ENC_NA);
        packet_length = payload_length = ti->finfo->length;
        proto_item_append_text(ti, " (size=%d)", packet_length);
        netgamedist_tree = proto_item_add_subtree(ti, ett_netgamedist);

        // get packet kind from private data
        kind = (guint32)pinfo->private_data;        
        
        // process based on kind
        switch(kind)
        {
            case GAME_PACKET_INPUT:
            {
                // add subtree item and tree for input packet
                ti = proto_tree_add_item(netgamedist_tree, hf_gamedist_input, tvb, decode_offset, 0, ENC_NA);
                netgamedist_tree2 = proto_item_add_subtree(ti, ett_netgamedist_input);

                // add subtree item for input type
                ti = proto_tree_add_item(netgamedist_tree2, hf_gamedist_input_type, tvb, decode_offset, 1, ENC_BIG_ENDIAN);
                type = tvb_get_guint8(tvb, decode_offset);
                proto_item_append_text(ti, " (%s)", val_to_str(type, gamedist_data_names, "INVALID"));
                decode_offset += 1;

                // add subtree item for input data
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_input_data, tvb, decode_offset, payload_length-decode_offset, ENC_BIG_ENDIAN);
                decode_offset += payload_length;
            }
            break;

            case GAME_PACKET_INPUT_MULTI:
            case GAME_PACKET_INPUT_MULTI_FAT:
            {
                guint32 off_types, off_lengths, off_buffer;
                guint32 input_decode_offset, input_count;
                guint32 count, input;
                gint32 length, length_size;
                guint8 delta, vers, type;
                guint8 input_buf[SOCKET_MAXUDPRECV]; // way more than we need

                // add subtree item and tree for input packet
                ti2 = proto_tree_add_item(netgamedist_tree, hf_gamedist_minput, tvb, decode_offset, 0, ENC_NA);
                netgamedist_tree2 = proto_item_add_subtree(ti2, ett_netgamedist_minput);
                // add subtree item for input data
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_minput_data, tvb, decode_offset, payload_length, ENC_BIG_ENDIAN);
                tvb_memcpy(tvb, input_buf, decode_offset, payload_length);

                // get length size based on input type
                length_size = (kind == GAME_PACKET_INPUT_MULTI_FAT) ? 2 : 1;

                // add subtree item for input delta
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_minput_delta, tvb, decode_offset, 1, ENC_BIG_ENDIAN);
                delta = input_buf[0];
                // add subtree item for input version
                ti = proto_tree_add_item(netgamedist_tree2, hf_gamedist_minput_vers, tvb, decode_offset+1, 1, ENC_BIG_ENDIAN);
                vers = input_buf[1];
                // wrap version and ensure minimum count of 2 (see netgamedist)
                vers %= GMDIST_META_ARRAY_SIZE;
                proto_item_append_text(ti, " (%d)", vers); 
                count = DS_MAX(meta_info[vers].uPlayerCount, 2);
                proto_item_append_text(ti, " (count=%d)", count);

                // compute offsets to various parts
                off_types = 2;
                off_lengths = off_types + count/2;
                off_buffer = off_lengths + (length_size * (count-2));
                proto_item_append_text(ti, " (off_types=%d, off_lengths=%d, off_buffer=%d, mask=0x%08x)", off_types, off_lengths, off_buffer, meta_info[vers].uMask);

                // add inputs subtree
                ti = proto_tree_add_item(netgamedist_tree2, hf_gamedist_minput_inputs, tvb, decode_offset, 0, ENC_NA);
                netgamedist_tree3 = proto_item_add_subtree(ti, ett_netgamedist_minput_inputs);

                // decode the data
                for (input = 0, input_decode_offset = 0, input_count = 0; (input < 32) && (input_count < (count-1)); input += 1)
                {
                    if (meta_info[vers].uMask & (1 << input))
                    {
                        // compute the type of packet for player i (nybble-packed)
                        type = input_buf[off_types+input_count/2];
                        type = (input_count % 2) ? type>>4 : type&0x0f;

                        // get input type $$TODO support CRC decoding
                        type &= ~GMDIST_DATA_CRC_REQUEST;

                        // calculate input length, where appropriate
                        if ((type == GMDIST_DATA_INPUT) || (type == GMDIST_DATA_INPUT_DROPPABLE))
                        {
                            /* compute the length of this input packet; the last input length is unknown
                               (it is derived locally from the local player's input), the second to last
                               is implicit (total - known ones) */
                            if (input < (count-2))
                            {
                                length = input_buf[off_lengths+(input_count*length_size)];
                                if (kind == GAME_PACKET_INPUT_MULTI_FAT)
                                {
                                    length = (length<<8) | input_buf[off_lengths+(input_count*length_size)];
                                }
                            }
                            else
                            {
                                length = payload_length - input_decode_offset - off_buffer;
                            }

                            // prevent invalid sizes if bogus data is received
                            if (length < 0)
                            {
                                length = 0;
                            }
                        }
                        else
                        {
                            length = 0;
                        }

                        // add input item
                        ti = proto_tree_add_item(netgamedist_tree3, hf_gamedist_minput_inputs_data, tvb, decode_offset+off_buffer, length, ENC_BIG_ENDIAN);
                        proto_item_append_text(ti, " ([%d] length=%d, type=%d)", input, length, type);
                        // append input type name
                        proto_item_append_text(ti, " (%s)", val_to_str(type, gamedist_data_names, "INVALID"));
                        
                        // skip past input data
                        if ((type == GMDIST_DATA_INPUT) || (type == GMDIST_DATA_INPUT_DROPPABLE))
                        {
                            off_buffer += length;
                        }

                        // count the input
                        input_count += 1;
                    }
                }
            }
            break;

            case GAME_PACKET_INPUT_META:
            {
                guint32 meta_mask;
                // add subtree item and tree for meta packet
                ti2 = proto_tree_add_item(netgamedist_tree, hf_gamedist_meta, tvb, decode_offset, 0, ENC_NA);
                netgamedist_tree2 = proto_item_add_subtree(ti2, ett_netgamedist_meta);
                // add subtree item for meta version
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_meta_vers, tvb, decode_offset, 1, ENC_BIG_ENDIAN);
                meta_vers = tvb_get_guint8(tvb, decode_offset);
                decode_offset += 1;
                // add subtree item for meta mask
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_meta_mask, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                // process meta flag and add to list
                meta_mask = tvb_get_ntohl(tvb, decode_offset);
                meta_info[meta_vers].uMask = meta_mask;
                meta_info[meta_vers].uPlayerCount = _NetGameDistCountBits(meta_mask);
                meta_info[meta_vers].uVer = meta_vers;
                decode_offset += 1;
            }
            break;

            case GAME_PACKET_INPUT_FLOW:
            {
                // add subtree item and tree for flow packet
                ti2 = proto_tree_add_item(netgamedist_tree, hf_gamedist_flow, tvb, decode_offset, 0, ENC_NA);
                netgamedist_tree2 = proto_item_add_subtree(ti2, ett_netgamedist_flow);

                // add subtree item for sndremote
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_flow_sendremote, tvb, decode_offset, 1, ENC_BIG_ENDIAN);
                decode_offset += 1;
                // add subtree item for meta flags
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_flow_recvremote, tvb, decode_offset, 1, ENC_BIG_ENDIAN);
                decode_offset += 1;

                // add subtree item for crc_valid
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_flow_crcvalid, tvb, decode_offset, 1, ENC_BIG_ENDIAN);
                decode_offset += 1;
                // add subtree item for crc
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_flow_crcdata, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                decode_offset += 4;
            }
            break;

            case GAME_PACKET_STATS:
            {
                // add subtree item and tree for stats packet
                ti2 = proto_tree_add_item(netgamedist_tree, hf_gamedist_stat, tvb, decode_offset, 0, ENC_NA);
                netgamedist_tree2 = proto_item_add_subtree(ti2, ett_netgamedist_stat);

                // add subtree item for ping
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_stat_ping, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                decode_offset += 4;
                // add subtree item for late
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_stat_late, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                decode_offset += 4;
                // add subtree item for bps
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_stat_bps, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                decode_offset += 4;
                // add subtree item for pps
                proto_tree_add_item(netgamedist_tree2, hf_gamedist_stat_pps, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
                decode_offset += 4;
            }
            break;

            default:
            {
                proto_item_append_text(ti, " (kind=%d (unknown))", kind);
            }
            break;
        }


        // add summary info
        if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 1))
        {
            col_add_fstr(pinfo->cinfo, COL_INFO, "[%s] data=%d", "TODO", payload_length);
            global_eatunnel_colinfo_set = TRUE;
        }
    }
}

void proto_register_netgamedist(void)
{
    static hf_register_info hf[] =
    {
        { &hf_gamedist_input,           { "input",      "gamedist.input",           FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_input_type,      { "type",       "gamedist.input.type",      FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_input_data,      { "data",       "gamedist.input.data",      FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },

        { &hf_gamedist_minput,          { "minput",     "gamedist.minput",          FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_minput_data,     { "data",       "gamedist.minput.data",     FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_minput_delta,    { "delta",      "gamedist.minput.delta",    FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_minput_vers,     { "vers",       "gamedist.minput.vers",     FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_minput_count,    { "count",      "gamedist.minput.count",    FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_minput_inputs,   { "inputs",     "gamedist.minput.inputs",   FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_minput_inputs_data, { "data",    "gamedist.minput.inputs.data", FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },

        { &hf_gamedist_meta,            { "meta",       "gamedist.meta",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_meta_vers,       { "vers",       "gamedist.meta.vers",       FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_meta_mask,       { "mask",       "gamedist.meta.mask",       FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },

        { &hf_gamedist_flow,            { "flow",       "gamedist.flow",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_flow_sendremote, { "sendremote", "gamedist.flow.sendremote", FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_flow_recvremote, { "recvremote", "gamedist.flow.recvremote", FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_flow_crcvalid,   { "crcvalid",   "gamedist.flow.crcvalid",   FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_flow_crcdata,    { "crcdata",    "gamedist.flow.crcdata",    FT_UINT32, BASE_DEC,   NULL, 0x0, NULL, HFILL } },

        { &hf_gamedist_stat,            { "stat",       "gamedist.stat",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_stat_ping,       { "ping",       "gamedist.stat.ping",       FT_UINT32, BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_stat_late,       { "late",       "gamedist.stat.late",       FT_UINT32, BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_stat_bps,        { "bps",        "gamedist.stat.bps",        FT_UINT32, BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_gamedist_stat_pps,        { "pps",        "gamedist.stat.pps",        FT_UINT32, BASE_DEC,   NULL, 0x0, NULL, HFILL } },
    };
    static gint *ett[] =
    {
        &ett_netgamedist,
        &ett_netgamedist_input,
        &ett_netgamedist_minput,
        &ett_netgamedist_minput_inputs,
        &ett_netgamedist_meta,
        &ett_netgamedist_flow,
        &ett_netgamedist_stat,
    };

    // register our protocol
    proto_netgamedist = proto_register_protocol("NetGameDist Protocol", "NetGameDist", "gamedist");
    // register protocol preferences (options)
    netgamedist_module = prefs_register_protocol_subtree("eatunnel", proto_netgamedist, NULL);
    // register dissector (this allows commudp to call us)
    netgamedist_handle = register_dissector("netgamedist", dissect_netgamedist, proto_netgamedist);
    // register hf/ett
    proto_register_field_array(proto_netgamedist, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void proto_reg_handoff_netgamedist(void)
{
    static dissector_handle_t netgamedist_handle;
    netgamedist_handle = create_dissector_handle(dissect_netgamedist, proto_netgamedist);
}
