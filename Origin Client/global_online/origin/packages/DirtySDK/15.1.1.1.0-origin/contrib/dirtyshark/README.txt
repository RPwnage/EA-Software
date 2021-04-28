Overview
~~~~~~~~~~~~~~~
DirtyShark is a custom WireShark plug-in that decodes and (where required) decrypts DirtySock protocols.  Current
supported protocols are:

    - ProtoTunnel v1.1
    - CommUDP
    - NetGameLink
    - NetGameDist
    - VoIP

Protocols, when not versioned, are assumed to match the current SDK release.  A version of DirtyShark built against a
matching version of DirtySDK as used in the game client/game server may be required for full compatibility.

DirtySDK comes with pre-built DirtyShark and DirtySDK Win32 DLLs, built against a recent version of WireShark.

Building
~~~~~~~~~~~~~~~
Build DirtySDK using the -D:dirtysdk-samples=true flag to build contribs and samples, or generate a solution using the
same flag.  Only the pc-vc-dll-dev-debug and pc-vc-dll-dev-opt configurations are currently supported.

Installing
~~~~~~~~~~~~~~~
To install, download and install the matching version of WireShark.  The version of WireShark to install is referenced
in the DirtySDK masterconfig.xml file.  Make sure to install the correct version; currently only the 32bit version is
supported.  Copy dirtysock.dll from the DirtySDK build folder to WireShark root folder.  Copy dirtyshark.dll into the
WireShark/plugins/<version> folder.  When starting WireShark, the plugin will automatically load and be available.

Basic Use
~~~~~~~~~~~~~~~
Once installed, run WireShark, and open a capture file with the packet data you wish to inspect.  By default,
DirtyShark will be configured to dissect packets on the canonical port used in DirtySDK.  If the port is not the
canonical port, you can right-click on a packet and choose Decode As, and on the Transport tab select EATunnel.  To
decrypt EATunnel packets, the tunnel encryption key must be specified.  To set this parameter, right-click on the
dissected packet, select Protocol Preferences, and enter the tunnel key.  The tunnel key may be obtained from debug
DirtySDK logging on the game client or game server, or release logging on DirtyCast servers:

DirtySDK debug logging:
2015/04/09-10:51:40.249 prototunnel: [$0437e5e0][0000] allocated tunnel with id 0x01000000
key=462065f0-16bc-45a2-b226-46e906a12dfd-51f1ab89-2a0f-4d67-8080-3b79302021b7-818ae35a-8c4a-4158-9bc7-7423e97a1ffe

VoipServer release logging:
2015/04/09-10:51:40.249 voipserver: [gs000] processing 'nusr' command clientId=0x01d0000c (10) mode=1
key=462065f0-16bc-45a2-b226-46e906a12dfd-51f1ab89-2a0f-4d67-8080-3b79302021b7-818ae35a-8c4a-4158-9bc7-7423e97a1ffe

To decode VoIP properly, the platform must be specified.  The VoIP platform setting is in the VoIP protocol preferences
menu.  A VoIP platform setting that does not match the packet data will result in malformed packet warnings on
VoIP ping packets.

Advanced Use
~~~~~~~~~~~~~~~
Packet Display
The information that the Protocol and Info fields display can select between EATunnel, Game, and VoIP display.  Select
the EATunnel Protocol tree in the detailed view, right-click, and select Protocol Preferences to change the preference
on what is displayed.  Note that if Game info is selected, but an EATunnel packet only contains VoIP, VoIP info
will be displayed.  The opposite is also true.  To select only Game or VoIP packets for display, you can enter 'cudp'
or 'voip' in the Filter box.

Filtering
Packets can be filtered for display using the following semantics:

EATunnel:
 eat
 eat.ident
 eat.crypt

CommUDP:
 cudp
 cudp.seq
 cudp.ack
 cudp.clid
 cudp.meta
 cudp.meta.clid
 cudp.meta.rclid

NetGameLink:
 gamelink
 gamelink.strm
 gamelink.strm.idnt
 gamelink.strm.kind
 gamelink.strm.size
 gamelink.strm.data
 gamelink.data
 gamelink.sync
 gamelink.sync.repl
 gamelink.sync.echo
 gamelink.sync.late
 gamelink.sync.psnt
 gamelink.sync.prcvd
 gamelink.sync.plost
 gamelink.sync.nsnt
 gamelink.sync.flags
 gamelink.sync.size
 gamelink.kind

NetGameDist:
 gamedist
 gamedist.input
 gamedist.input.data
 gamedist.minput
 gamedist.minput.data
 gamedist.minput.delta
 gamedist.minput.vers
 gamedist.minput.inputs
 gamedist.minput.inputs.data
 gamedist.meta
 gamedist.meta.vers
 gamedist.meta.mask
 gamedist.flow
 gamedist.flow.sendremote
 gamedist.flow.recvremote
 gamedist.flow.crcvalid
 gamedist.flow.crcdata
 gamedist.stat
 gamedist.stat.ping
 gamedist.stat.late
 gamedist.stat.bps
 gamedist.stat.pps

VoIP:
 voip
 voip.head
 voip.head.type
 voip.head.flags
 voip.head.clid
 voip.head.sess
 voip.head.stat
 voip.conn
 voip.conn.rclid
 voip.conn.active
 voip.conn.pad
 voip.conn.lusers
 voip.conn.lusers.user
 voip.conn.lusers.userblob
 voip.disc
 voip.disc.rclid
 voip.ping
 voip.ping.rclid
 voip.ping.channels
 voip.ping.channels.data
 voip.ping.data
 voip.micr
 voip.micr.sendmask
 voip.micr.channels
 voip.micr.seqn
 voip.micr.subpacknum
 voip.micr.subpacklen
 voip.micr.userindex
 voip.micr.subpackets
 voip.micr.subpackets.data

Non-standard Protocol Data
By default, DirtyShark is set up to decode packet data as it is typically sent using BlazeSDK and DirtySDK.  It is
therefore assumed that CommUDP data is tunneled on port index zero, and VoIP data is tunneled on port index one.  These
defaults can be changed by setting the name of the protocol in the portindex map in the tunnel preferences menu.  Any
protocol name supported by Wireshark may be entered here.  For example, if HTTP traffic is being tunneled using port
index five, the "PIdx5 Dissector" preference can be set to "http", and the tunneled data for that port index will use
the Wireshark HTTP dissector.

NetGameLink Protocol Data
Similar to "Non-standard Protocol Data", NetGameLink payload data may be decoded using the dissector specified in
the NetGameLink protocol preferences.

FAQ/Troubleshooting
~~~~~~~~~~~~~~~
-Why can't I decrypt my Xbox One peer-to-peer captures?
On Xbox One, DirtySDK uses the Microsoft proprietary Secure Sockets API.  This API utilizes its own encryption and
cannot be decrypted by DirtyShark.

-Why is my Info field blank?
When decrypting ProtoTunnel packets using the EATunnel dissector, if you see the message (packetoff != packetlen) in
the detailed window view, that means that packet length validation failed after decrypting the packet data.  Typically
this indicates the tunnel key is not correct.  Another possible reason is a mismatch between the version of the
protocol being dissected and the version that DirtyShark is expecting.  It is also a possible indication that there is
corrupt data or the packet being dissected is not actually a ProtoTunnel packet.  As an example of the latter case,
when a client logs into Blaze, it will send QoS packets against various QoS servers on the canonical ProtoTunnel port.
These packets will incorrectly be identifed as EATunnel packets in the Protocol column.

-I'm filtering (Game or VoIP) packets, why am I seeing some (VoIP or Game) packets?
This happens when filtering for game or voip traffic, but preferring display of the other protocol, when one or more
sub-packets are present for each protocol type in the packets in question.  To resolve, change filter preferences to
the protocol being filtered for.

-Why are some of my VoIP packets showing as malformed?
VoIP platform must be properly specified to ensure proper decoding; see "Basic Use" up above.

Future Work
~~~~~~~~~~~~~~~
-Clean up eatunnel decoder and decryption code
-Add relative time conversation support
-Add eat.nsubpackets for filtering
-Add support for multiple sub-packets in Info view
-Add more advanced protocol info (e.g. note commudp resends, prototunnel out of order packets, etc)
-Add ProtoAdvt dissector
-Add ability to display decrypted data in Follow Stream view
-Add means to get tunnel key from release build gameservers (from the Blaze server or from the game servers themselves)
-Add decoding of CRC request information to NetGameDist decoder


