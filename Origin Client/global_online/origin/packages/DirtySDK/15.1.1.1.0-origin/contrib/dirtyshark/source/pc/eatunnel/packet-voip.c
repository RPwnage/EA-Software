#undef _WIN32_WINNT //$$TODO - get build to not include this
#define MSC_VER_REQUIRED 1700 //$$TODO - where should this be specified?

#include <config.h>
#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/prefs-int.h>

#include "DirtySDK/dirtysock/dirtynet.h"

// from voippacket.h

//! VoIP Flags bit field included in VoIP packets
#define VOIP_PACKET_STATUS_FLAG_METADATA     (1)     //!< packet contains platform-specific metadata (unreliable)
#define VOIP_PACKET_RELIABLE_FLAG_FIRST      (32)    //!< first reliable data entry on connection - to be used by consuming end to initialize expected seq number accordingly
#define VOIP_PACKET_RELIABLE_FLAG_ACK_ACKCNF (64)    //!< packet contains ACK+ACKCNF protocol components (reliability mechanism)
#define VOIP_PACKET_RELIABLE_FLAG_DATA       (128)   //!< packet contains DATA protocol component (reliability mechanism)

//! range of supported reliable data types
#define VOIP_RELIABLE_TYPE_USERADD          (0)     //!< reliable data contains newly added local user on the originating console (used for voip join-in-progress)
#define VOIP_RELIABLE_TYPE_USERREM          (1)     //!< reliable data contains newly removed local user on the originating console (used for voip leave-in-progress)
#define VOIP_RELIABLE_TYPE_OPAQUE           (2)     //!< reliable data contains platform-specific data blob that needs to be transmitted reliably

//! mask used to manipulate ACKCNF bit for uAckedSeq field of ReliableAckT type
#define VOIP_RELIABLE_ACKCNF_MASK           (0x80)


#include "plugin.h"

static int proto_voip = -1;

static dissector_handle_t voip_handle;

static int hf_voip_head = -1;
static int hf_voip_head_type = -1;
static int hf_voip_head_flags = -1;
static int hf_voip_head_clid = -1;
static int hf_voip_head_sess = -1;
static int hf_voip_head_stat = -1;

static int hf_voip_conn = -1;
static int hf_voip_conn_rclid = -1;
static int hf_voip_conn_connected = -1;
static int hf_voip_conn_pad = -1;
static int hf_voip_conn_lusers = -1;
static int hf_voip_conn_lusers_user = -1;
static int hf_voip_conn_lusers_userblob = -1;

static int hf_voip_disc = -1;
static int hf_voip_disc_rclid = -1;

static int hf_voip_ping = -1;
static int hf_voip_ping_rclid = -1;
static int hf_voip_ping_channels = -1;
static int hf_voip_ping_channels_data = -1;
static int hf_voip_ping_data = -1;

static int hf_voip_micr = -1;
static int hf_voip_micr_sendmask = -1;
static int hf_voip_micr_channels = -1;
static int hf_voip_micr_seqn = -1;
static int hf_voip_micr_subpacknum = -1;
static int hf_voip_micr_subpacklen = -1;
static int hf_voip_micr_userindex = -1;
static int hf_voip_micr_meta = -1;
static int hf_voip_micr_meta_coid = -1;
static int hf_voip_micr_meta_adev = -1;
static int hf_voip_micr_subpackets = -1;
static int hf_voip_micr_subpackets_data = -1;

static gint ett_voip = -1;
static gint ett_head = -1;
static gint ett_conn = -1;
static gint ett_conn_lusers = -1;
static gint ett_disc = -1;
static gint ett_ping = -1;
static gint ett_ping_channels = -1;
static gint ett_micr = -1;
static gint ett_meta = -1;
static gint ett_micr_subpackets = -1;

static module_t *voip_module;

enum
{
    VOIP_PLATFORM_PC = 1,
    VOIP_PLATFORM_PS4,
    VOIP_PLATFORM_XB1
};

/*
    VoIP module constants and types
*/

#define VOIP_PORT   6000

// ps4 localuser constants
#define VOIP_MAXLOCALUSERS_PS4  (4)
#define VOIP_MAXLOCALUSERS_PS4_EXTENDED (VOIP_MAXLOCALUSERS_PS4 + 1)
// xb1 localuser constants
#define VOIP_MAXLOCALUSERS_XB1  (16)
#define VOIP_MAXLOCALUSERS_XB1_EXTENDED (VOIP_MAXLOCALUSERS_XB1 + 1)
// all other platforms
#define VOIP_MAXLOCALUSERS      (1)
#define VOIP_MAXLOCALUSERS_EXTENDED     (VOIP_MAXLOCALUSERS)

//! VoIP data packet header
typedef struct VoipPacketHeadT
{
    uint8_t         aType[3];        //!< type/version info
    uint8_t         uFlags;          //!< flags
    uint8_t         aClientId[4];    //!< source clientId
    uint8_t         aSessionId[4];   //!< session ID
    uint8_t         aHeadsetStat[2]; //!< headset status bitfield (1 bit per local user)
} VoipPacketHeadT;

typedef struct VoipUserT
{
    char strUserId[20];
} VoipUserT;

typedef struct VoipUserExT
{
    VoipUserT user;                       // important:  must remain first field in this struct.
    uint8_t aSerializedUserBlob[48];      // data blob that can be sent over the network for recreating the user object on other consoles
} VoipUserExT;

typedef union VoipConnPacketUsersT
{
    VoipUserExT     LocalUsersXB1[VOIP_MAXLOCALUSERS_XB1_EXTENDED];
    VoipUserT       LocalUsersPS4[VOIP_MAXLOCALUSERS_PS4_EXTENDED];
    VoipUserT       LocalUsers[VOIP_MAXLOCALUSERS_EXTENDED];
} VoipConnPacketUsersT;

typedef struct VoipPacketChanT
{
    uint8_t         aChannelId[4];  //!< packet channelId
} VoipPacketChanT;

/* packet type names, used in detailed description.  these exactly match the packet payload type,
   which is what you want to use when filtering */
 static const value_string voip_type_names[] =
{
    { '\0COg', "COg" },
    { '\0DSC', "DSC" },
    { '\0MIC', "MIC" },
    { '\0PNG', "PNG" },
    { 0, NULL }
};

 /* packet type names, used in info description.  we don't use the exact packet values as these
    are easier to read when looking at the info field */
static const value_string voip_info_names[] =
{
    { '\0COg', "CONN" },
    { '\0DSC', "DISC" },
    { '\0MIC', "MICR" },
    { '\0PNG', "PING" },
    { 0, NULL }
};

guint32 voip_decode_head(tvbuff_t *tvb, proto_tree *voip_tree, guint32 decode_offset, guint32 *type, guint32 *clid, guint8 *flags)
{
    proto_tree *voip_head_tree;
    proto_item *ti;

    // add subtree for packet head data
    ti = proto_tree_add_item(voip_tree, hf_voip_head, tvb, decode_offset, sizeof(VoipPacketHeadT), ENC_NA);
    voip_head_tree = proto_item_add_subtree(ti, ett_head);

    // add subtree item for type header
    ti = proto_tree_add_item(voip_head_tree, hf_voip_head_type, tvb, decode_offset, 3, ENC_ASCII);
    *type = tvb_get_ntoh24(tvb, decode_offset);
    decode_offset += 3;
    proto_item_append_text(ti, " (%s)", val_to_str(*type, voip_type_names, "UNK"));

    // add subtree item for flags
    proto_tree_add_item(voip_head_tree, hf_voip_head_flags, tvb, decode_offset, 1, ENC_NA);
    *flags = tvb_get_guint8(tvb, decode_offset);
    decode_offset += 1;

    // add subtree item for client id
    proto_tree_add_item(voip_head_tree, hf_voip_head_clid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
    *clid = tvb_get_ntohl(tvb, decode_offset);
    decode_offset += 4;

    // add subtree item for session
    proto_tree_add_item(voip_head_tree, hf_voip_head_sess, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
    decode_offset += 4;

    // add subtree item for stat
    proto_tree_add_item(voip_head_tree, hf_voip_head_stat, tvb, decode_offset, 2, ENC_BIG_ENDIAN);
    decode_offset += 2;

    return(decode_offset);
}

void voip_decode_conn(tvbuff_t *tvb, proto_tree *voip_tree, guint32 decode_offset, guint32 packet_length)
{
    proto_tree *voip_conn_tree;
    proto_item *ti;
    guint32 conn_length = packet_length-decode_offset, lusers_length;
    VoipConnPacketUsersT LocalUsers;

    // add subtree for conn data
    ti = proto_tree_add_item(voip_tree, hf_voip_conn, tvb, decode_offset, conn_length, ENC_NA);
    voip_conn_tree = proto_item_add_subtree(ti, ett_conn);

    // add subtree item for remote client id header
    proto_tree_add_item(voip_conn_tree, hf_voip_conn_rclid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
    decode_offset += 4;

    // add subtree item for bConnected
    proto_tree_add_item(voip_conn_tree, hf_voip_conn_connected, tvb, decode_offset, 1, ENC_NA);
    decode_offset += 1;

    // add subtree item for pad
    proto_tree_add_item(voip_conn_tree, hf_voip_conn_pad, tvb, decode_offset, 1, ENC_NA);
    decode_offset += 1;

    // add subtree item for lusers
    lusers_length = packet_length-decode_offset;
    ti = proto_tree_add_item(voip_conn_tree, hf_voip_conn_lusers, tvb, decode_offset, lusers_length, ENC_NA);

    /* Here we detect what platform this is based on size of connection data, so we can dissect
       LocalUsers more accurately.  The LocalUsers format varies based on platform, so what we
       do here is calculate the expected size to determine the platform and therefore the format
       of the data. */

    // calculate size of localuser data and validate against our expected sizes
    if (lusers_length <= sizeof(LocalUsers))
    {
        proto_tree *voip_lusers_tree;
        guint32 user_count;
        if (lusers_length == sizeof(LocalUsers.LocalUsersXB1))
        {
            VoipUserExT *pUserData;
            voip_lusers_tree = proto_item_add_subtree(ti, ett_conn_lusers);
            for (user_count = 0; user_count < VOIP_MAXLOCALUSERS_XB1_EXTENDED; user_count += 1)
            {
                proto_tree_add_item(voip_lusers_tree, hf_voip_conn_lusers_user, tvb, decode_offset, sizeof(pUserData->user), ENC_NA);
                proto_tree_add_item(voip_lusers_tree, hf_voip_conn_lusers_userblob, tvb, decode_offset+sizeof(pUserData->user), sizeof(pUserData->aSerializedUserBlob), ENC_NA);
                decode_offset += sizeof(*pUserData);
            }
        }
        else if ((lusers_length == sizeof(LocalUsers.LocalUsersPS4)) || (lusers_length == sizeof(LocalUsers.LocalUsers)))
        {
            VoipUserT *pUserData;
            guint32 user_count_max = (lusers_length == sizeof(LocalUsers.LocalUsers)) ? VOIP_MAXLOCALUSERS_EXTENDED : VOIP_MAXLOCALUSERS_PS4_EXTENDED;
            voip_lusers_tree = proto_item_add_subtree(ti, ett_conn_lusers);
            for (user_count = 0; user_count < user_count_max; user_count += 1)
            {
                proto_tree_add_item(voip_lusers_tree, hf_voip_conn_lusers, tvb, decode_offset, sizeof(*pUserData), ENC_NA);
                decode_offset += sizeof(*pUserData);
            }
        }
    }
}

void voip_decode_disc(tvbuff_t *tvb, proto_tree *voip_tree, guint32 decode_offset, guint32 packet_length)
{
    proto_tree *voip_disc_tree;
    proto_item *ti;

    // add subtree for disc data
    ti = proto_tree_add_item(voip_tree, hf_voip_disc, tvb, decode_offset, packet_length-decode_offset, ENC_NA);
    voip_disc_tree = proto_item_add_subtree(ti, ett_disc);

    // add subtree item for remote client id header
    proto_tree_add_item(voip_disc_tree, hf_voip_disc_rclid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
}

void voip_decode_ping(tvbuff_t *tvb, proto_tree *voip_tree, guint32 decode_offset, guint32 packet_length)
{
    proto_tree *voip_ping_tree;
    proto_item *ti;
    guint channel_num, pingdata_len;

    // add subtree for ping data
    ti = proto_tree_add_item(voip_tree, hf_voip_ping, tvb, decode_offset, packet_length-decode_offset, ENC_NA);
    voip_ping_tree = proto_item_add_subtree(ti, ett_ping);

    // add subtree item for remote client id header
    proto_tree_add_item(voip_ping_tree, hf_voip_conn_rclid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
    decode_offset += 4;

    // add channel subtree
    switch(global_voip_platform)
    {
        case VOIP_PLATFORM_PC:
            channel_num = VOIP_MAXLOCALUSERS_EXTENDED;
            break;
        case VOIP_PLATFORM_PS4:
            channel_num = VOIP_MAXLOCALUSERS_PS4_EXTENDED;
            break;
        case VOIP_PLATFORM_XB1:
            channel_num = VOIP_MAXLOCALUSERS_XB1_EXTENDED;
            break;
        default:
            // unknown, so we say zero and let it all fall under data
            channel_num = 0;
            break;
    }
    // add channel data
    if (channel_num > 0)
    {
        guint channel_idx;
        proto_tree *voip_chan_tree;

        ti = proto_tree_add_item(voip_ping_tree, hf_voip_ping_channels, tvb, decode_offset, channel_num*sizeof(VoipPacketChanT), ENC_NA);
        voip_chan_tree = proto_item_add_subtree(ti, ett_ping_channels);

        for (channel_idx = 0; channel_idx < channel_num; channel_idx += 1)
        {
            proto_tree_add_item(voip_chan_tree, hf_voip_ping_channels_data, tvb, decode_offset, sizeof(VoipPacketChanT), ENC_NA);
            decode_offset += sizeof(VoipPacketChanT);
        }
    }
    // add ping data, if present
    if ((pingdata_len = packet_length-decode_offset) > 0)
    {
        proto_tree_add_item(voip_ping_tree, hf_voip_ping_data, tvb, decode_offset, pingdata_len, ENC_NA);
    }
}

void voip_decode_micr(tvbuff_t *tvb, proto_tree *voip_tree, guint32 decode_offset, guint32 packet_length, guint8 flags)
{
    proto_tree *voip_micr_tree, *voip_subp_tree;
    proto_item *ti;
    guint8 subpack_idx, subpack_num, subpack_len;

    ti = proto_tree_add_item(voip_tree, hf_voip_micr, tvb, decode_offset, packet_length-decode_offset, ENC_NA);
    voip_micr_tree = proto_item_add_subtree(ti, ett_micr);

    // add subtree item for sendmask
    proto_tree_add_item(voip_micr_tree, hf_voip_micr_sendmask, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
    decode_offset += 4;

    // add subtree item for channels
    proto_tree_add_item(voip_micr_tree, hf_voip_micr_channels, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
    decode_offset += 4;

    // add subtree item for sequence
    proto_tree_add_item(voip_micr_tree, hf_voip_micr_seqn, tvb, decode_offset, 1, ENC_NA);
    decode_offset += 1;

    // add subtree item for subpacknum
    proto_tree_add_item(voip_micr_tree, hf_voip_micr_subpacknum, tvb, decode_offset, 1, ENC_NA);
    subpack_num = tvb_get_guint8(tvb, decode_offset);
    decode_offset += 1;

    // add subtree item for subpacklen
    ti = proto_tree_add_item(voip_micr_tree, hf_voip_micr_subpacklen, tvb, decode_offset, 1, ENC_NA);
    subpack_len = tvb_get_guint8(tvb, decode_offset);
    decode_offset += 1;
    if (subpack_len == 255)
    {
        proto_item_append_text(ti, " (variable-length)");
    }

    // add subtree item for user index
    proto_tree_add_item(voip_micr_tree, hf_voip_micr_userindex, tvb, decode_offset, 1, ENC_NA);
    decode_offset += 1;

    // add meta info if present
    if ((global_voip_platform == VOIP_PLATFORM_XB1) && (flags & VOIP_PACKET_STATUS_FLAG_METADATA))
    {
        guint8 meta_len = tvb_get_guint8(tvb, decode_offset);
        decode_offset += 1;
        if (meta_len == 8)
        {
            proto_tree *voip_meta_tree;
            ti = proto_tree_add_item(voip_micr_tree, hf_voip_micr_meta, tvb, decode_offset, meta_len, ENC_NA);
            voip_meta_tree = proto_item_add_subtree(ti, ett_meta);
            proto_item_append_text(ti, " (len=%d)", meta_len);

            // add subtree item for console identifier
            proto_tree_add_item(voip_meta_tree, hf_voip_micr_meta_coid, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
            decode_offset += 4;
            // add subtree item for audio device info
            proto_tree_add_item(voip_meta_tree, hf_voip_micr_meta_adev, tvb, decode_offset, 4, ENC_BIG_ENDIAN);
            decode_offset += 4;
        }
    }

    // add subtree for subpackets
    ti = proto_tree_add_item(voip_micr_tree, hf_voip_micr_subpackets, tvb, decode_offset, packet_length-decode_offset, ENC_NA);
    voip_subp_tree = proto_item_add_subtree(ti, ett_micr_subpackets);

    // add subpackets
    if (subpack_len != 255)
    {
        for (subpack_idx = 0; subpack_idx < subpack_num; subpack_idx += 1)
        {
            proto_tree_add_item(voip_subp_tree, hf_voip_micr_subpackets_data, tvb, decode_offset, subpack_len, ENC_NA);
            decode_offset += subpack_len;
        }
    }
    else
    {
        for (subpack_idx = 0; subpack_idx < subpack_num; subpack_idx += 1)
        {
            // get subpacket length
            subpack_len = tvb_get_guint8(tvb, decode_offset);
            decode_offset += 1;
            // get subpacket data
            ti = proto_tree_add_item(voip_subp_tree, hf_voip_micr_subpackets_data, tvb, decode_offset, subpack_len, ENC_NA);
            decode_offset += subpack_len;
            // add length
            proto_item_append_text(ti, " (len=%d)", subpack_len);
        }
    }
}

static void dissect_voip(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
    // set info field if it hasn't already been set or we are set to prefer voip
    if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 2))
    {
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "VoIP");
    }

    // are we being asked for details?
    if (tree != NULL)
    {
        proto_tree *voip_tree;
        proto_item *ti;
        guint32 decode_offset = 0;
        guint32 packet_length, voip_type, voip_clid;
        guint8 packet_bytes[SOCKET_MAXUDPRECV];
        guint8 voip_flags;

        // add voip tree item
        ti = proto_tree_add_item(tree, proto_voip, tvb, decode_offset, -1, ENC_NA);
        packet_length = ti->finfo->length < sizeof(packet_bytes) ? ti->finfo->length : sizeof(packet_bytes);
        proto_item_append_text(ti, " (size=%d)", packet_length);

        // add subtree for dissected data
        voip_tree = proto_item_add_subtree(ti, ett_voip);

        // decode VoipPacketHeadT
        decode_offset = voip_decode_head(tvb, voip_tree, decode_offset, &voip_type, &voip_clid, &voip_flags);

        // append output type
        proto_item_append_text(ti, " (%s)", val_to_str(voip_type, voip_type_names, "(UNK)"));

        // decode packet
        switch(voip_type)
        {
            case '\0COg':
                voip_decode_conn(tvb, voip_tree, decode_offset, packet_length);
                break;
            case '\0DSC':
                voip_decode_disc(tvb, voip_tree, decode_offset, packet_length);
                break;
            case '\0PNG':
                voip_decode_ping(tvb, voip_tree, decode_offset, packet_length);
                break;
            case '\0MIC':
                voip_decode_micr(tvb, voip_tree, decode_offset, packet_length, voip_flags);
                break;
            default:
                break;
        }

        // add summary
        if ((global_eatunnel_colinfo_set == FALSE) || (global_eatunnel_info == 2))
        {
            col_add_fstr(pinfo->cinfo, COL_INFO, "[%s] clid=0x%08x", val_to_str(voip_type, voip_info_names, "UNKN"), voip_clid);
            global_eatunnel_colinfo_set = TRUE;
        }
    }
}

void proto_register_voip(void)
{
    static hf_register_info hf[] =
    {
        { &hf_voip_head,                { "head",       "voip.head",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_head_type,           { "type",       "voip.head.type",       FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_head_flags,          { "flags",      "voip.head.flags",      FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_head_clid,           { "clid",       "voip.head.clid",       FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_head_sess,           { "sess",       "voip.head.sess",       FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_head_stat,           { "stat",       "voip.head.stat",       FT_UINT16, BASE_HEX,   NULL, 0x0, NULL, HFILL } },

        { &hf_voip_conn,                { "conn",       "voip.conn",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_conn_rclid,          { "rclid",      "voip.conn.rclid",      FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_conn_connected,      { "active",     "voip.conn.active",     FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_conn_pad,            { "pad",        "voip.conn.pad",        FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_conn_lusers,         { "lusers",     "voip.conn.lusers",     FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_conn_lusers_user,    { "luser",      "voip.conn.lusers.user", FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_conn_lusers_userblob,{ "luserblob",  "voip.conn.lusers.userblob", FT_BYTES,  BASE_NONE,  NULL, 0x0, NULL, HFILL } },

        { &hf_voip_disc,                { "disc",       "voip.disc",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_disc_rclid,          { "rclid",      "voip.disc.rclid",      FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },

        { &hf_voip_ping,                { "ping",       "voip.ping",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_ping_rclid,          { "rclid",      "voip.ping.rclid",      FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_ping_channels,       { "channels",   "voip.ping.channels",   FT_NONE,   BASE_NONE,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_ping_channels_data,  { "channels",   "voip.ping.channels.data", FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_ping_data,           { "data",       "voip.ping.data",       FT_BYTES, BASE_NONE,   NULL, 0x0, NULL, HFILL } },

        { &hf_voip_micr,                { "micr",       "voip.micr",            FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_sendmask,       { "sendmask",   "voip.micr.sendmask",   FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_channels,       { "channels",   "voip.micr.channels",   FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_seqn,           { "seqn",       "voip.micr.seqn",       FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_subpacknum,     { "subpacknum", "voip.micr.subpacknum", FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_subpacklen,     { "subpacklen", "voip.micr.subpacklen", FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_userindex,      { "userindex",  "voip.micr.userindex",  FT_UINT8,  BASE_DEC,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_meta,           { "meta",       "voip.micr.meta",       FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_meta_coid,      { "coid",       "voip.micr.meta.coid",  FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_meta_adev,      { "adev",       "voip.micr.meta.adev",  FT_UINT32, BASE_HEX,   NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_subpackets,     { "subpackets", "voip.micr.subpackets", FT_NONE,   BASE_NONE,  NULL, 0x0, NULL, HFILL } },
        { &hf_voip_micr_subpackets_data,{ "data",       "voip.micr.subpackets.data", FT_BYTES, BASE_NONE,   NULL, 0x0, NULL, HFILL } },
    };
    static gint *ett[] =
    {
        &ett_voip,
        &ett_head,
        &ett_conn,
        &ett_conn_lusers,
        &ett_disc,
        &ett_ping,
        &ett_ping_channels,
        &ett_micr,
        &ett_meta,
        &ett_micr_subpackets
    };
    static const enum_val_t voip_platform_prefs[] =
    {
        { "PC",  "Decode VoIP for PC",  VOIP_PLATFORM_PC  },
        { "PS4", "Decode VoIP for PS4", VOIP_PLATFORM_PS4 },
        { "XB1", "Decode VoIP for XB1", VOIP_PLATFORM_XB1 },
        { NULL,  NULL,                  0 }
    };

    // register our protocol
    proto_voip = proto_register_protocol("VoIP Protocol", "VoIP", "voip");
    // register protocol preferences (options)
    voip_module = prefs_register_protocol_subtree("eatunnel", proto_voip, NULL);
    // voip platform preference
    prefs_register_enum_preference(voip_module, "platform", "VoIP Platform",
        "Choose the platform VoIP data will be decoded for", &global_voip_platform,
        voip_platform_prefs, TRUE);
    // register dissecetor (this allows eatunnel to call us)
    voip_handle = register_dissector("voip", dissect_voip, proto_voip);
    // register hf/ett
    proto_register_field_array(proto_voip, hf, array_length(hf));
    proto_register_subtree_array(ett, array_length(ett));
}

void proto_reg_handoff_voip(void)
{
    static dissector_handle_t voip_handle;

    voip_handle = create_dissector_handle(dissect_voip, proto_voip);
    dissector_add_uint("udp.port", VOIP_PORT, voip_handle);
}
