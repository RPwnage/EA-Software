#undef _WIN32_WINNT //$$TODO - get build to not include this
#define MSC_VER_REQUIRED 1700 //$$TODO - where should this be specified?

#include <config.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/prefs-int.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"

#include "plugin.h"

#define COMMUDP_PORT   7000

static int proto_commudp = -1;

static dissector_handle_t commudp_handle;

static int hf_commudp_seq = -1;
static int hf_commudp_ack = -1;
static int hf_commudp_clid = -1;
static int hf_commudp_meta = -1;
static int hf_commudp_meta_clid = -1;
static int hf_commudp_meta_rclid = -1;
static int hf_commudp_dat = -1;
static int hf_commudp_sub = -1;

static gint ett_commudp = -1;
static gint ett_meta = -1;
static gint ett_data = -1;

static dissector_handle_t netgamelink_dissector_handle;

static module_t *commudp_module;

#define SEQ_MASK (0x00ffffff)
#define SEQ_MULTI_SHIFT (28)
#define SEQ_META_SHIFT  (28 - 4)
#define SEQ_MULTI_INC (1 << SEQ_MULTI_SHIFT)

//! define protocol packet types
enum {
    RAW_PACKET_INIT = 1,        // initiate a connection
    RAW_PACKET_CONN,            // confirm a connection
    RAW_PACKET_DISC,            // terminate a connection
    RAW_PACKET_NAK,             // force resend of lost data
    RAW_PACKET_POKE,            // try and poke through firewall

    RAW_PACKET_UNREL = 128,     // unreliable packet send (must be power of two)
                                // 128-255 reserved for unreliable packet sequence
    RAW_PACKET_DATA = 256,      // initial data packet sequence number (must be power of two)

    /*  Width of the sequence window, can be anything provided
        RAW_PACKET_DATA + RAW_PACKET_DATA_WINDOW
        doesn't overlap the meta-data bits.
    */
    RAW_PACKET_DATA_WINDOW = (1 << 24) - 256
};

static const value_string commudp_seq_names[] =
{
    { RAW_PACKET_INIT, "RAW_PACKET_INIT" },
    { RAW_PACKET_CONN, "RAW_PACKET_CONN" },
    { RAW_PACKET_DISC, "RAW_PACKET_DISC" },
    { RAW_PACKET_NAK,  "RAW_PACKET_NAK"  },
    { RAW_PACKET_POKE, "RAW_PACKET_POKE" },
    { 0, NULL }
};

static const char *commudp_get_seq_name(guint32 seq)
{
    const char *seq_name;
    if (seq < RAW_PACKET_UNREL)
    {
        seq_name = val_to_str(seq, commudp_seq_names, "unknown");
    }
    else if (seq < RAW_PACKET_DATA)
    {
        seq_name = "RAW_PACKET_DATA_UNRELIABLE";
    }
    else
    {
        seq_name = "RAW_PACKET_DATA";
    }
    return(seq_name);
}

static void dissect_commudp(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 1))
    {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "CommUDP");
    }

    // are we being asked for details?
    if (tree != NULL)
    {
        proto_tree *commudp_tree = NULL;
        proto_item *ti = NULL;
        guint32 decode_offset = 0;
        guint32 packet_length, payload_length;
        guint32 seq, ack, multi, meta;

        // add commudp tree item
        ti = proto_tree_add_item(tree, proto_commudp, tvb, decode_offset, -1, ENC_NA);
        packet_length = ti->finfo->length;
        proto_item_append_text(ti, " (size=%d)", packet_length);

        // add subtree for dissected data
        commudp_tree = proto_item_add_subtree(ti, ett_commudp);

        // add subtree item for sequence header
        ti = proto_tree_add_item(commudp_tree, hf_commudp_seq, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
        seq = tvb_get_ntohl(tvb, decode_offset);
        decode_offset += 4;

        // split up seq into seq/multi/meta
        multi = seq >> SEQ_MULTI_SHIFT;
        meta = (seq >> SEQ_META_SHIFT) & 0xf;
        seq &= SEQ_MASK;
        
        // append seq name
        proto_item_append_text(ti, ", (%s)", commudp_get_seq_name(seq));
        // append if there is meta info
        if (meta != 0)
        {
            proto_item_append_text(ti, ", (meta)");
        }
        // append multipacket count
        if (multi != 0)
        {
            proto_item_append_text(ti, ", (multi=%d)", multi);
        }

        // add subtree item for acknowledge header
        ti = proto_tree_add_item(commudp_tree, hf_commudp_ack, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
        ack = tvb_get_ntohl(tvb, decode_offset);
        decode_offset += 4;

        // decode ack
        if ((seq == RAW_PACKET_INIT) || (seq == RAW_PACKET_POKE))
        {
            // ack==connident
            proto_item_append_text(ti, ", (connident)");
            // add subtree item for client identifier
            ti = proto_tree_add_item(commudp_tree, hf_commudp_clid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
            decode_offset += 4;
            proto_item_append_text(ti, ", (clientident)");
        }
        else if ((seq == RAW_PACKET_CONN) || (seq == RAW_PACKET_DISC))
        {
            proto_item_append_text(ti, ", (connident)");
        }
        else if (seq == RAW_PACKET_NAK)
        {
            proto_item_append_text(ti, ", (recvseqn)");
        }
        else
        {
            proto_item_append_text(ti, ", (ack)");
        }

        // handle metadata
        if (meta != 0)
        {
            proto_tree *meta_tree;
            // add subtree item for metadata
            ti = proto_tree_add_item(commudp_tree, hf_commudp_meta, tvb, decode_offset, 0, ENC_NA);
            // add tree for metadata
            meta_tree = proto_item_add_subtree(ti, ett_meta);
            // add subtree item for client identifier
            ti = proto_tree_add_item(meta_tree, hf_commudp_meta_clid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
            decode_offset += 4;
            // add subtree item for remote client identifier
            ti = proto_tree_add_item(meta_tree, hf_commudp_meta_rclid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
            decode_offset += 4;
        }

        // add netgamelink payload if present
        if ((payload_length = (packet_length-decode_offset)) > 0)
        {
            tvbuff_t *netgamelink_tvb;
            proto_tree *data_tree;
            guint32 subpacket_length, subpacket_offset;
            guint16 subpacket_size[8];
            gint32 count;

            // figure out subpacket offset/size (decoded in reverse order due to how data is encapsulated)
            for (count = multi; count >= 0; count -= 1)
            {
                if (count > 0)
                {
                    payload_length -= 1;
                    subpacket_length = tvb_get_guint8(tvb, decode_offset+payload_length);
                    subpacket_offset = decode_offset+payload_length-subpacket_length;
                }
                else
                {
                    subpacket_length = payload_length;
                    subpacket_offset = decode_offset;
                }
                subpacket_size[count] = subpacket_length;
                payload_length -= subpacket_length;
            }

            // add subpackets to tree
            for (count = 0; count <= (signed)multi; count += 1)
            {
                subpacket_length = subpacket_size[count];
                // add tree for packet data
                data_tree = proto_item_add_subtree(commudp_tree, ett_data);
                // add subtree item for data header
                ti = proto_tree_add_item(data_tree, hf_commudp_dat, tvb, decode_offset, subpacket_length, ENC_NA);
                // create new tvb buffer to hold netgamelink data
                netgamelink_tvb = tvb_new_subset(tvb, decode_offset, subpacket_length, subpacket_length);
                // call netgamelink dissector
                call_dissector(netgamelink_dissector_handle, netgamelink_tvb, pinfo, tree);

                // increment past sub-packet
                decode_offset += subpacket_length;

                // if this isn't the first sub-packet, add length
                if (count > 0)
                {
                    // add tree for packet data
                    proto_tree_add_item(data_tree, hf_commudp_sub, tvb, decode_offset, 1, ENC_NA);
                    decode_offset += 1;
                }
            }
        }
        else
        {
            // add summary info if no netgamelink data
            if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 1))
            {
                col_add_fstr(pinfo->cinfo, COL_INFO, "[%s][0x%08x] data=%d", commudp_get_seq_name(seq), ack, packet_length-decode_offset);
                global_eatunnel_colinfo_set = TRUE;
            }
        }
    }
}

void proto_register_commudp(void)
{
    static hf_register_info hf[] =
    {
        { &hf_commudp_seq,          { "seq",   "cudp.seq",        FT_UINT32, BASE_HEX,  NULL, 0x0, NULL, HFILL } },
        { &hf_commudp_ack,          { "ack",   "cudp.ack",        FT_UINT32, BASE_HEX,  NULL, 0x0, NULL, HFILL } },
        { &hf_commudp_clid,         { "clid",  "cudp.clid",       FT_UINT32, BASE_HEX,  NULL, 0x0, NULL, HFILL } },
        { &hf_commudp_meta,         { "meta",  "cudp.meta",       FT_NONE,   BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_commudp_meta_clid,    { "clid",  "cudp.meta.clid",  FT_UINT32, BASE_HEX,  NULL, 0x0, NULL, HFILL } },
        { &hf_commudp_meta_rclid,   { "rclid", "cudp.meta.rclid", FT_UINT32, BASE_HEX,  NULL, 0x0, NULL, HFILL } },
        { &hf_commudp_dat,          { "data",  "cudp.data",       FT_BYTES,  BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_commudp_sub,          { "subp",  "cudp.subp",       FT_UINT8,  BASE_DEC,  NULL, 0x0, NULL, HFILL } },
    };
    static gint *ett[] =
    {
        &ett_commudp,
        &ett_meta,
        &ett_data,
    };

    // register our protocol
    proto_commudp = proto_register_protocol("CommUDP Protocol", "CommUDP", "cudp");
    // register protocol preferences (options)
    commudp_module = prefs_register_protocol_subtree("eatunnel", proto_commudp, NULL);
    // register dissecetor (this allows eatunnel to call us)
    commudp_handle = register_dissector("commudp", dissect_commudp, proto_commudp);
    // register hf/ett
    proto_register_field_array(proto_commudp, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void proto_reg_handoff_commudp(void)
{
    static dissector_handle_t commudp_handle;

    commudp_handle = create_dissector_handle(dissect_commudp, proto_commudp);
    dissector_add_uint("udp.port", COMMUDP_PORT, commudp_handle);

    netgamelink_dissector_handle = find_dissector("netgamelink");
}
