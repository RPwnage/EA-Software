#undef _WIN32_WINNT //$$TODO - get build to not include this
#define MSC_VER_REQUIRED 1700 //$$TODO - where should this be specified?

#include <config.h>
#include <glib.h>
#include <epan/conversation.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/prefs-int.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"

#include "plugin.h"

//! eatunnel stream data
typedef struct EATunnelStreamT
{
    address src_addr;
    address dst_addr;
    guint16 src_port;
    guint16 dst_port;
    guint32 stream_offset;
    guint32 visited;
} EATunnelStreamT;

//! eatunnel conversation data
typedef struct EATunnelConversationDataT
{
    EATunnelStreamT stream[2];
} EATunnelConversationDataT;


//! per-packet eatunnel packet data
typedef struct EATunnelPacketDataT
{
    uint8_t *frame_data;
    uint32_t frame_size;
    uint32_t stream_offset;
    uint32_t visited;
    uint32_t out_of_order;
    uint32_t stream_skip;
} EATunnelPacketDataT;

#define EATUNNEL_PORT   3659

#define HMAC_SIZE       12
#define HSHK_SIZE       7

static int proto_eatunnel = -1;

static int hf_eatunnel_ident = -1;
static int hf_eatunnel_crypt = -1;
static int hf_eatunnel_hshk = -1;
static int hf_eatunnel_subpacket_head = -1;
static int hf_eatunnel_subpacket_body = -1;
static int hf_eatunnel_hmac = -1;

static gint ett_eatunnel = -1;
static gint ett_hshk = -1;
static gint ett_hmac = -1;
static gint ett_subpacket = -1;

static dissector_handle_t commudp_dissector_handle;
static dissector_handle_t voip_dissector_handle;

static module_t *eatunnel_module;

static char *eatunnel_key = NULL;
static char *eatunnel_pidx_dissector[8] = { NULL };

static packet_info *eatunnel_packetinfo = NULL;

gboolean global_netgamelink_info;

static const uint8_t _ProtoTunnel_aHmacInitVec[64] =
{
    0x6a, 0x09, 0xe6, 0x67, 0xf3, 0xbc, 0xc9, 0x08,
    0xbb, 0x67, 0xae, 0x85, 0x84, 0xca, 0xa7, 0x3b,
    0x3c, 0x6e, 0xf3, 0x72, 0xfe, 0x94, 0xf8, 0x2b,
    0xa5, 0x4f, 0xf5, 0x3a, 0x5f, 0x1d, 0x36, 0xf1,
    0x51, 0x0e, 0x52, 0x7f, 0xad, 0xe6, 0x82, 0xd1,
    0x9b, 0x05, 0x68, 0x8c, 0x2b, 0x3e, 0x6c, 0x1f,
    0x1f, 0x83, 0xd9, 0xab, 0xfb, 0x41, 0xbd, 0x6b,
    0x5b, 0xe0, 0xcd, 0x19, 0x13, 0x7e, 0x21, 0x79
};

static void prep_hmac_key(uint8_t *hmac_key, int32_t hmac_size, const uint8_t *init_vec, int32_t init_vec_size, const char *key)
{
    CryptArc4T hmac_state;
    CryptArc4Init(&hmac_state, (const uint8_t *)key, (int32_t)strlen(key), 12);
    ds_memcpy_s(hmac_key, hmac_size, init_vec, init_vec_size);
    CryptArc4Apply(&hmac_state, hmac_key, hmac_size);
}

static gint validate_hmac(const uint8_t *packet_data, int32_t packet_size, int32_t hmac_offset)
{
    uint8_t aPackHmac[16], aCalcHmac[16];
    uint8_t aHmacKey[64];
    uint8_t aPacketData[SOCKET_MAXUDPRECV];

    // init hmac key
    prep_hmac_key(aHmacKey, sizeof(aHmacKey), _ProtoTunnel_aHmacInitVec, sizeof(_ProtoTunnel_aHmacInitVec), eatunnel_key);
    // copy packet data to temp
    ds_memcpy_s(aPacketData, sizeof(aPacketData), packet_data, packet_size);
    // copy HMAC data to temp
    ds_memcpy_s(aPackHmac, sizeof(aPackHmac), packet_data+hmac_offset, HMAC_SIZE);
    // clear HMAC payload in packet to zero so we can calculate the HMAC ourselves
    memset(aPacketData+hmac_offset, 0, HMAC_SIZE);
    // calculate HMAC
    CryptHmacCalc(aCalcHmac, HMAC_SIZE, aPacketData, packet_size, aHmacKey, sizeof(aHmacKey), CRYPTHASH_MURMUR3);

    // validate
    return(!memcmp(aPackHmac, aCalcHmac, HMAC_SIZE));
}

static uint16_t sync_cipher(EATunnelPacketDataT *eatunnel_packet_data, uint32_t uRecvOffset, uint32_t uRecvOffsetState)
{
    // lost data?  sync the cipher
    if (uRecvOffset != uRecvOffsetState)
    {
        // calc how many data units we've missed
        int16_t iRecvOffset = (signed)uRecvOffset;
        int16_t iTunlOffset = (signed)uRecvOffsetState;
        // calc 15bit offset
        int16_t iRecvDiff = (iRecvOffset - iTunlOffset) & 0x7fff;
        /* sign extend our 15bit offset - we do it this way because the simpler way of shifting left and
           back again right relies on the right shift sign-extending the value, however in the C standard
           it is implementation-dependent whether the shift is treated as signed or unsigned */
        iRecvDiff |= (iRecvDiff & 16384) << 1;

        // check to see if packet is previous to our current window offset
        if (iRecvDiff < 0)
        {
            eatunnel_debug_printf("packet-eatunnel: out-of-order packet (off=%d)\n", iRecvDiff);
            eatunnel_packet_data->out_of_order = -iRecvDiff;
            return(uRecvOffset);
        }

        // advance the cipher to resync
        eatunnel_debug_printf("packet-eatunnel: stream skip %d bytes\n", iRecvDiff/*3PROTOTUNNEL_CRYPTBITS*/);
        eatunnel_packet_data->stream_skip = abs(iRecvDiff);
        //CryptArc4Advance(&CryptRecvState, iRecvDiff<<PROTOTUNNEL_CRYPTBITS);
        uRecvOffsetState = uRecvOffset;
    }
    return(uRecvOffsetState);
}

static EATunnelStreamT *eatunnel_get_stream_data(conversation_t *eatunnel_convo, packet_info *pinfo, proto_item *ti, EATunnelPacketDataT **tunnel_packet_data, guint8 *packet_info_new)
{
    EATunnelConversationDataT *data_ptr;
    EATunnelStreamT *eatunnel_stream = NULL;
    PluginPacketDataT *packet_data;

    if ((data_ptr = (EATunnelConversationDataT *)conversation_get_proto_data(eatunnel_convo, proto_eatunnel)) == NULL)
    {
        data_ptr = (EATunnelConversationDataT *)wmem_new(wmem_file_scope(), EATunnelConversationDataT);
        memset(data_ptr, 0, sizeof(*data_ptr));
        copy_address(&data_ptr->stream[0].src_addr, &pinfo->src);
        copy_address(&data_ptr->stream[0].dst_addr, &pinfo->dst);
        data_ptr->stream[0].src_port = pinfo->srcport;
        data_ptr->stream[0].dst_port = pinfo->destport;
        conversation_add_proto_data(eatunnel_convo, proto_eatunnel, (void *)data_ptr);
        eatunnel_stream = &data_ptr->stream[0];
        eatunnel_debug_printf("packet-eatunnel: creating conversation0 data\n");
    }
    else
    {
        if (!(addresses_equal(&data_ptr->stream[0].src_addr, &pinfo->src) && (data_ptr->stream[0].src_port == pinfo->srcport)))
        {
            if (data_ptr->stream[1].src_port == 0)
            {
                copy_address(&data_ptr->stream[1].src_addr, &pinfo->src);
                copy_address(&data_ptr->stream[1].dst_addr, &pinfo->dst);
                data_ptr->stream[1].src_port = pinfo->srcport;
                data_ptr->stream[1].dst_port = pinfo->destport;
                eatunnel_debug_printf("packet-eatunnel: creating conversation1 data\n");
            }
            eatunnel_stream = &data_ptr->stream[1];
        }
        else
        {
            eatunnel_stream = &data_ptr->stream[0];
        }
    }

    // if first time, allocate and set up a packet info structure
    if ((packet_data = (PluginPacketDataT *)p_get_proto_data(wmem_file_scope(), pinfo, proto_eatunnel, 0)) == NULL)
    {
        packet_data = (PluginPacketDataT *)wmem_new(wmem_file_scope(), PluginPacketDataT);
        memset(packet_data, 0, sizeof(packet_data));

        packet_data->pEATunnelData = (EATunnelPacketDataT *)wmem_new(wmem_file_scope(), EATunnelPacketDataT);
        memset(packet_data->pEATunnelData, 0, sizeof(*(packet_data->pEATunnelData)));

        packet_data->prev = eatunnel_packetinfo;
#if 0
        if (packet_data->prev != NULL)
        {
            PluginPacketDataT *prev_packet_info = (PluginPacketDataT *)packet_data->prev->private_data;
            if (prev_packet_info != NULL)
            {
                prev_packet_info->next = pinfo;
            }
        }
#endif
        eatunnel_packetinfo = pinfo;

        p_add_proto_data(wmem_file_scope(), pinfo, proto_eatunnel, 0, packet_data);
        pinfo->private_data = (void *)packet_data;

        eatunnel_debug_printf("packet-eatunnel: [%5d] creating per-packet data for packet (visited=%d, offset=%d)\n",
            pinfo->fd->num, eatunnel_stream->visited, eatunnel_stream->stream_offset);

        packet_data->pEATunnelData->stream_offset = eatunnel_stream->stream_offset;
        packet_data->pEATunnelData->visited = eatunnel_stream->visited++;

        *packet_info_new = TRUE;
    }
    else
    {
        eatunnel_debug_printf("packet-eatunnel: [%5d] visiting packet\n", pinfo->fd->num);
        *packet_info_new = FALSE;
    }

    *tunnel_packet_data = packet_data->pEATunnelData;
    return(eatunnel_stream);
}

static void dissect_eatunnel(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    int subpacket, nsubpackets, packetlen, packetoff, cryptsize;
    CryptArc4T Arc4State;
    tvbuff_t *next_tvb, *sub_tvb;
    char *tunnel_key;

    // set proto column; we typically expect sub-packet dissectors to overwrite this
    col_set_str(pinfo->cinfo, COL_PROTOCOL, "EATunnel");
    // force it to stick if we are preferring tunnel info
    global_eatunnel_colinfo_set = global_eatunnel_info == 0 ? TRUE : FALSE;
    // clear info column
    col_clear(pinfo->cinfo, COL_INFO);

    // set up debug output
    eatunnel_debug_setup();

    if (1) //$$todo - clean this up - we don't want to exclude all of this when the tree is null
    {
        proto_tree *eatunnel_tree = NULL, /**hshk_tree,*/ *hmac_tree = NULL, *subpacket_tree;
        proto_item *ti = NULL;
        guint8 subpacket_idx[8];
        guint16 ident_header, encrypt_header, subpacket_len[8];
        guint32 decode_offset = 0, decrypt_offset, crypt_offset;
        guint8 packet_bytes[1264], *packet_decrypt;
        guint32 packet_length;
        guint8 active=1;
        char str_ports[16], *pidx_dissector;
        dissector_handle_t sub_dissector_handle;
        conversation_t *eatunnel_convo;
        EATunnelStreamT *eatunnel_stream = NULL;
        EATunnelPacketDataT *eatunnel_packet_data = NULL;
        guint8 packet_info_new = FALSE;

        // find conversation
        eatunnel_convo = find_conversation(pinfo->fd->num, &pinfo->src, &pinfo->dst, pinfo->ptype, pinfo->srcport, pinfo->destport, 0);

        // get packet length
        packetlen = tvb_reported_length(tvb);

        // add tree item for eatunnel packet -- we have to guard just this one call for no tree
        if (tree != NULL)
        {
            ti = proto_tree_add_item(tree, proto_eatunnel, tvb, decode_offset, -1, ENC_NA);
        }
        proto_item_append_text(ti, " (size=%d)", packetlen);

        // make a copy of the data
        packet_length = (packetlen < sizeof(packet_bytes)) ? packetlen : sizeof(packet_bytes);
        tvb_memcpy(tvb, packet_bytes, 0, packet_length);
        // make a copy for decryption
        packet_decrypt = (guint8 *)g_malloc(packet_length);
        ds_memcpy(packet_decrypt, packet_bytes, packet_length);

        // add subtree for dissected data
        eatunnel_tree = proto_item_add_subtree(ti, ett_eatunnel);

        // add subtree item for ident header
        ti = proto_tree_add_item(eatunnel_tree, hf_eatunnel_ident, tvb, decode_offset, 2, ENC_BIG_ENDIAN);
        // get header
        ident_header = tvb_get_ntohs(tvb, decode_offset);
        decode_offset += 2;

        // add ident info
        if (ident_header & 0x8000)
        {
            proto_item_append_text(ti, " (ident=0x%04x, version=%d.%d)", ident_header, (ident_header & ~0x8000) >> 8, ident_header & 0xff);
        }
        else
        {
            proto_item_append_text(ti, " (ident=%d)", ident_header);
        }

        // add subtree item for encryption header
        ti = proto_tree_add_item(eatunnel_tree, hf_eatunnel_crypt, tvb, decode_offset, 2, ENC_BIG_ENDIAN);
        // get header
        encrypt_header = tvb_get_ntohs(tvb, decode_offset);
        decode_offset += 2;

        // get offset
        crypt_offset = (encrypt_header & 0x7fff) << 3;
        // add header info
        proto_item_append_text(ti, " (header=0x%04x, offset=%d)", encrypt_header, crypt_offset);

        // are we in a conversation?
        if (eatunnel_convo != NULL)
        {
            eatunnel_stream = eatunnel_get_stream_data(eatunnel_convo, pinfo, ti, &eatunnel_packet_data, &packet_info_new);
            proto_item_append_text(ti, " (stream_offset=%d)", eatunnel_packet_data->stream_offset);
            // lost data?
            if (crypt_offset != (eatunnel_packet_data->stream_offset&0x3ffff))
            {
                crypt_offset = sync_cipher(eatunnel_packet_data, crypt_offset, eatunnel_packet_data->stream_offset);
                eatunnel_packet_data->stream_offset = crypt_offset;
                eatunnel_stream->stream_offset = crypt_offset;
            }
            else
            {
                crypt_offset = eatunnel_packet_data->stream_offset;
            }

            if (eatunnel_packet_data->out_of_order)
            {
                proto_item_append_text(ti, " (out-of-order packet %d bytes))", eatunnel_packet_data->out_of_order);
            }
            if (eatunnel_packet_data->stream_skip)
            {
                proto_item_append_text(ti, " (stream skip %d bytes)", eatunnel_packet_data->stream_skip);
            }
        }

        // handshake?
        if (ident_header & 0x8000)
        {
            guint8 handshake_header[7];
            // get handshake data
            tvb_memcpy(tvb, handshake_header, decode_offset, sizeof(handshake_header));
            // add item for handshake
            ti = proto_tree_add_item(eatunnel_tree, hf_eatunnel_hshk, tvb, decode_offset, HSHK_SIZE, ENC_BIG_ENDIAN);
            proto_item_append_text(ti, " (clientident=0x%02x%02x%02x%02x, connident=%d, active=%s)",
                handshake_header[0], handshake_header[1], handshake_header[2], handshake_header[3],
                (int)handshake_header[4] << 8 | handshake_header[5], handshake_header[6] ? "yes" : "no");
            active = handshake_header[6];
            decode_offset += HSHK_SIZE;
        }

        // get tunnel key from preferences -- first clean it of any cr/lf characters
        if ((tunnel_key = strchr(eatunnel_key, '\r')) != NULL)
        {
            *tunnel_key = '\0';
        }
        if ((tunnel_key = strchr(eatunnel_key, '\n')) != NULL)
        {
            *tunnel_key = '\0';
        }
        tunnel_key = eatunnel_key;
        // init crypt state
        CryptArc4Init(&Arc4State, (const unsigned char *)tunnel_key, (int32_t)strlen(tunnel_key), 1);
        // advance the cipher 3k to initial state
        CryptArc4Advance(&Arc4State, 3*1024+crypt_offset);

        // add subtree items for subpacket headers
        for (nsubpackets = 0, packetoff = decode_offset+HMAC_SIZE; (nsubpackets < 8) && (packetoff < packetlen); nsubpackets += 1)
        {
            decrypt_offset = decode_offset+(nsubpackets*2);
            CryptArc4Apply(&Arc4State, packet_decrypt+decrypt_offset, 2);
            subpacket_len[nsubpackets] = ((int)packet_decrypt[decrypt_offset+0]<<4)|(packet_decrypt[decrypt_offset+1]>>4);
            subpacket_idx[nsubpackets] = packet_decrypt[decrypt_offset+1]&0xf;
            packetoff += 2+subpacket_len[nsubpackets];
            ti = proto_tree_add_item(eatunnel_tree, hf_eatunnel_subpacket_head, tvb, decode_offset+(nsubpackets*2), 2, ENC_BIG_ENDIAN);
            proto_item_append_text(ti, " (data=0x%02x%02x)", packet_decrypt[decrypt_offset+0], packet_decrypt[decrypt_offset+1]);
            proto_item_append_text(ti, " (port=%d,size=%d)", subpacket_idx[nsubpackets], subpacket_len[nsubpackets]);
            proto_item_append_text(ti, " (packetoff=%d)", packetoff);
        }
        proto_item_append_text(ti, " (stream_incr=%d)", ((packetlen-decode_offset+7) & ~7));
        // validate packet length (if not correct, it means our key is likely wrong, or this isn't a prototunnel packet)
        if (packetoff != packetlen)
        {
            proto_item_append_text(ti, " (packetoff != packetlen)", packetoff, packetlen);
            return;
        }

        // update stream offset we expect for next packet
        if (packet_info_new)
        {
            eatunnel_stream->stream_offset += ((packetlen-decode_offset+7) & ~7);
        }

        // decrypt the rest of the packet
        packetoff = decode_offset+(nsubpackets*2);
        cryptsize = packetlen-packetoff;
        CryptArc4Apply(&Arc4State, packet_decrypt+packetoff, cryptsize);

        // create new tvb buffer to hold decrypted data; this allows us to display it
        next_tvb = tvb_new_child_real_data(tvb, packet_decrypt, packet_length, packet_length);
        tvb_set_free_cb(next_tvb, g_free);
        add_new_data_source(pinfo, next_tvb, "Decrypted Data");

        // add HMAC
        ti = proto_tree_add_item(eatunnel_tree, hf_eatunnel_hmac, tvb, packetoff, HMAC_SIZE, ENC_BIG_ENDIAN);
        hmac_tree = proto_item_add_subtree(ti, ett_hmac);
        ti = proto_tree_add_item(hmac_tree, hf_eatunnel_hmac, next_tvb, packetoff, HMAC_SIZE, ENC_BIG_ENDIAN);

        // validate hmac
        if (!validate_hmac(packet_decrypt, packet_length, packetoff))
        {
            proto_item_append_text(ti, " (validation failed)");
            return;
        }

        // add subtree items for subpackets
        for (subpacket = 0, packetoff = decode_offset+(nsubpackets*2)+HMAC_SIZE; subpacket < nsubpackets; subpacket += 1)
        {
            // add encrypted packet data
            ti = proto_tree_add_item(eatunnel_tree, hf_eatunnel_subpacket_body, tvb, packetoff, subpacket_len[subpacket], ENC_BIG_ENDIAN);
            // add subtree for decrypted packet data
            subpacket_tree = proto_item_add_subtree(ti, ett_subpacket);

            // reference dissector for this pidx from preferences
            pidx_dissector = eatunnel_pidx_dissector[subpacket_idx[subpacket]];
            // if no dissector and pidx zero, default to commudp
            if (*pidx_dissector == '\0' && subpacket_idx[subpacket] == 0)
            {
                pidx_dissector = "commudp";
            }
            // if no dissector and pidx one, default to voip
            if (*pidx_dissector == '\0' && subpacket_idx[subpacket] == 1)
            {
                pidx_dissector = "voip";
            }

            // hand off to commudp dissector
            if (!strcmp(pidx_dissector, "commudp"))
            {
                sub_dissector_handle = commudp_dissector_handle;
            }
            // hand off to voip dissector
            else if (!strcmp(pidx_dissector, "voip"))
            {
                sub_dissector_handle = voip_dissector_handle;
            }
            // hand off to custom dissector
            else if (*pidx_dissector != '\0')
            {
                sub_dissector_handle = find_dissector(pidx_dissector);
            }
            else
            {
                sub_dissector_handle = find_dissector("data");
            }

            // create new tvb buffer to hold protocol data
            sub_tvb = tvb_new_subset(next_tvb, packetoff, subpacket_len[subpacket], subpacket_len[subpacket]);
            // call dissector
            call_dissector(sub_dissector_handle, sub_tvb, pinfo, tree);

            // increment to next subpacket
            packetoff += subpacket_len[subpacket];
        }

        // format info field, for example: 3659->3659 ident=0x8101 offset=0x0000 active=0 subpacket=2
        if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 0))
        {
            ds_snzprintf(str_ports, sizeof(str_ports), "%d->%d", pinfo->srcport, pinfo->destport);
            col_add_fstr(pinfo->cinfo, COL_INFO, "%12s ident=0x%04x offset=0x%04x active=%d subpacket=%d",
                str_ports, ident_header, crypt_offset, active, nsubpackets);
        }
    }
}

void proto_register_eatunnel(void)
{
    static hf_register_info hf[] =
    {
        { &hf_eatunnel_ident,  { "ident", "eat.ident", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_eatunnel_crypt,  { "crypt", "eat.crypt", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_eatunnel_hshk,   { "handshake", "eat.handshake", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_eatunnel_subpacket_head, { "subpacket_header", "eat.subpacket_header", FT_UINT16, BASE_DEC, NULL, 0x0, NULL, HFILL } },
        { &hf_eatunnel_subpacket_body, { "subpacket_body", "eat.subpacket_body", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } },
        { &hf_eatunnel_hmac, { "hmac", "eat.hmac", FT_BYTES, BASE_NONE, NULL, 0x0, NULL, HFILL } }
    };
    static gint *ett[] =
    {
        &ett_eatunnel,
        &ett_hshk,
        &ett_hmac,
        &ett_subpacket
    };
    static const enum_val_t infoprefs[] =
    {
        { "Tunnel", "Prefer Tunnel info", 0 },
        { "Game",   "Prefer Game info",   1 },
        { "Voip",   "Prefer VoIP info",   2 },
        { NULL,     NULL,                -1 }
    };

    // register our protocol
    proto_eatunnel = proto_register_protocol("EATunnel Protocol", "EATunnel", "eat");

    // register protocol preferences (options)
    eatunnel_module = prefs_register_protocol_subtree("eatunnel", proto_eatunnel, NULL);

    // prototunnel key
    prefs_register_string_preference(eatunnel_module, "tunnel_key", "Tunnel Key",
         "Set the key used to decrypt packet data", &eatunnel_key);

    // info summary preference
    prefs_register_enum_preference(eatunnel_module, "info_summary", "Info Summary",
        "Choose the protocol to be displayed in the info summary", &global_eatunnel_info,
        infoprefs, TRUE);

    // portindex dissector preferences
    prefs_register_string_preference(eatunnel_module, "pidx0_dissector", "PIdx0 Dissector",
         "Set the dissector for Port Index 0", &eatunnel_pidx_dissector[0]);
    prefs_register_string_preference(eatunnel_module, "pidx1_dissector", "PIdx1 Dissector",
         "Set the dissector for Port Index 1", &eatunnel_pidx_dissector[1]);
    prefs_register_string_preference(eatunnel_module, "pidx2_dissector", "PIdx2 Dissector",
         "Set the dissector for Port Index 2", &eatunnel_pidx_dissector[2]);
    prefs_register_string_preference(eatunnel_module, "pidx3_dissector", "PIdx3 Dissector",
         "Set the dissector for Port Index 3", &eatunnel_pidx_dissector[3]);
    prefs_register_string_preference(eatunnel_module, "pidx4_dissector", "PIdx4 Dissector",
         "Set the dissector for Port Index 4", &eatunnel_pidx_dissector[4]);
    prefs_register_string_preference(eatunnel_module, "pidx5_dissector", "PIdx5 Dissector",
         "Set the dissector for Port Index 5", &eatunnel_pidx_dissector[5]);
    prefs_register_string_preference(eatunnel_module, "pidx6_dissector", "PIdx6 Dissector",
         "Set the dissector for Port Index 6", &eatunnel_pidx_dissector[6]);
    prefs_register_string_preference(eatunnel_module, "pidx7_dissector", "PIdx7 Dissector",
         "Set the dissector for Port Index 7", &eatunnel_pidx_dissector[7]);

    // dirtyshark debug file preference
    prefs_register_filename_preference(eatunnel_module, "debug_filename", "Debug File", "Filename for debugging information", &eatunnel_debugfilename);

    proto_register_field_array(proto_eatunnel, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void proto_reg_handoff_eatunnel(void)
{
    static dissector_handle_t eatunnel_handle;

    eatunnel_handle = create_dissector_handle(dissect_eatunnel, proto_eatunnel);
    dissector_add_uint("udp.port", EATUNNEL_PORT, eatunnel_handle);

    commudp_dissector_handle = find_dissector("commudp");
    voip_dissector_handle = find_dissector("voip");
}
