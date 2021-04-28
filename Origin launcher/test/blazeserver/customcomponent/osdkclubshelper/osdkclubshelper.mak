SRC := $(wildcard component/osdkclubshelper/*.cpp)

HDR := $(wildcard component/osdkclubshelper/*.h)

TDF := $(wildcard component/osdkclubshelper/gen/*.tdf)
RPC := $(wildcard component/osdkclubshelper/gen/*.rpc)

RPC_TARGETS := osdkclubshelperslave osdkclubshelpermaster

# Define component specific config files here to include in the VS proj.
CFG := $(wildcard component/osdkclubshelper/cfg/*.cfg)

# Define makefile type to include in the VS proj.
MAK := component
