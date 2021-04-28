blazeserver_grpc_tools 15.1.1.10.2
Date: April 20, 2021

-+Overview+-

The blazeserver_grpc_tools package contains all the gRPC related tool functionality required by the Blaze server.


https://developer.ea.com/display/blaze/Blaze+2015+Winter+1.x+Continuous+Feature+Upgrade+Guide

-+blazeserver_grpc_tools Dependencies+-
eaconfig        5.07.00
gflags          2.2.1
grpc            1.10.0-3-prebuilt
libopenssl      1.1.1d-prebuilt
protobuf        3.4.1
UnixClang       4.0.1
UnixCrossTools  2.02.00
UnixGCC         0.10.00
VisualStudio    15.4.27004.2002-2-proxy
vstomaketools   2.06.09
WindowsSDK      10.0.14393-proxy
zlib            1.2.11

-+Note+-

Release notes are prefaced with these tags:

[needs action] - If you are working in this area and have implemented code, you will need to make a change as part of your upgrade.
[minor] - Should not affect your implementation or upgrade from the prior version.
[NEW] - A brand new feature in this release.

-+Overview+-

[NEW][needs action] blazeserver_grpc_tools has been updated to be able to support generating equivalent .protos, from Blaze Server's EATDF .tdfs and .rpcs. For details, see Blaze documentation on blazeserver_grpc_tools and here: https://developer.ea.com/display/TEAMS/TDF+To+Proto+Transition+Plan

-+Known Issues+-

-+Resolved Issues+-

https://eadpjira.ea.com/issues/?jql=project%20%3D%20GOS%20AND%20fixVersion%20%3D%20Urraca.2

