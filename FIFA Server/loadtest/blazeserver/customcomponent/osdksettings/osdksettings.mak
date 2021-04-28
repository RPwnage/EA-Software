SRC := $(wildcard component/osdksettings/*.cpp)

HDR := $(wildcard component/osdksettings/*.h)

TDF := $(wildcard component/osdksettings/gen/*.tdf)
RPC := $(wildcard component/osdksettings/gen/*.rpc)

RPC_TARGETS := osdksettingsslave osdksettingsmaster

# Define component specific config files here to include in the VS proj.
CFG := $(wildcard component/osdksettings/cfg/*.cfg)

# Define makefile type to include in the VS proj.
MAK := component