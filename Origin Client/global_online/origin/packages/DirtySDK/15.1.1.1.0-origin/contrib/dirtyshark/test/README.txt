DirtyShark sample packet captures
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
prototunnel_00.pcap
- Capture of tunneled udp packet data using Tester2
- Set PIdx0 dissector to "data" to view non-commudp payload data
- Tunnel Key: abcd

prototunnel_01.pcap
- Capture of tunneled 2p peer-to-peer gameplay with Ignition, including VoIP
- Does not include any use of the connections (no gameplay data, no VoIP MIC data)
- Tunnel Key: f1083d54-19b5-465f-b37c-5284133965e0-c9f6b8e9-ecee-4cab-bc93-f0386b260ca6-e4dd971f-c814-4a6a-a176-46dc56ca43ab

prototunnel_02.pcap
- Capture of tunneled 2p DirtyCast OTP gameplay with Ignition
- Full login to Blaze captured, includes gameplay data and PC VoIP
- Tunnel packets start at packet 367.  Note that earlier "EATunnel" packets are actually QoS packets; they are identified as
  EATunnel because they share the same canonical port
- Tunnel Key: f3971290-e151-408c-8fd9-d853b49a2cbd-3ec38169-e096-4743-a8ac-c3f8121fd846-f3971290-e151-408c-8fd9-d853b49a2cbd

prototunnel_03.pcap
- Capture of tunneled 2p peer-to-peer gameplay with Ignition, including VoIP, with CommUDP metadata enabled
- Includes stream packets and VoIP MIC packets
- Stream is greater than 256k in length, so stream offset wraps
- Tunnel Key: 67470e0e-27c9-4d29-8b2f-62c81aa1ca04-6096debf-2beb-445f-8a88-50ab86258cb4-ade701de-0a9f-4a7a-98a9-40486b022018

prototunnel_04.pcap
- Capture of tunneled udp packet data using Tester2
- Packet #5 contains HTTP data.  To view this data using the Wireshark HTTP decoder, set the EATunnel "PIdx0 Dissector"
  preference to "http"
- Packets #1-4 and #6 contain unformatted data.  Use the "data" decoder to view.
- Tunnel Key: abcd

prototunnel_05.pcap
- Capture of tunneled 2p peer-to-peer gameplay with Ignition, including PS4 VoIP
- Tunnel Key: f9f80e93-6049-4d48-9ec6-e877fac51ca1-025de1a3-e735-4b01-8297-8ca4042dbc6b-1d1c44bd-e496-4977-8f02-3fa8752faedd

prototunnel_06.pcap
- Capture of tunneled 2p DirtyCast OTP gameplay with Ignition, including XB1 VoIP
- Tunnel Key: a8e358a1-cee2-48a1-aacb-1a6eeba7a903-a8e358a1-cee2-48a1-aacb-1a6eeba7a903-aea37643-c5f1-49e3-9615-d87a6a5dc8cf

prototunnel_07.pcap
- Capture of tunneled 2p DirtyCast OTP gameplay with Ignition, including XB1 VoIP, with "fat" inputs
- Tunnel Key: 0cd12608-071b-43af-8cb2-05fafbcbb9ef-0cd12608-071b-43af-8cb2-05fafbcbb9ef-434e1aa6-a80b-43f1-83e7-08fd487c1bb3

prototunnel_08.pcap
- Capture of tunneled 2p peer-to-peer gameplay with Ignition, including PS4 VoIP and some NetGameLink payload data
- Tunnel Key: 66fbafd9-ca3e-4c1a-9a46-930a097cfec6-8c6c1ef9-6aba-40c2-a351-751ee3301afd-f722221f-3f7c-48fa-a1f3-5dc91ae4885b

prototunnel_09.pcap
- Capture of tunneled 3p DirtyCast OTP gameplay with Ignition, including PS4 VoIP
- Tunnel Key: 21262a19-1343-4be9-b958-a7fc1db6aea0-21262a19-1343-4be9-b958-a7fc1db6aea0-b0c4847e-200a-4709-84e4-f3439b0a98ce

prototunnel_10.pcap
- Capture of tunneled 3p DirtyCast OTP gameplay with Ignition, including PS4 VoIP, with "fat" inputs
- Tunnel Key: a1c1a7ee-3088-4ba9-9c3f-9ba6121aa973-a1c1a7ee-3088-4ba9-9c3f-9ba6121aa973-a81715c7-d356-4eb3-b9fa-a7d7ae3673ec
