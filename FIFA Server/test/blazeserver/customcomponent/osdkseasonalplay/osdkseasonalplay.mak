SRC := $(wildcard component/osdkseasonalplay/*.cpp) \
       $(wildcard component/osdkseasonalplay/$(CUSTOMDIR)/*.cpp)


HDR := $(wildcard component/osdkseasonalplay/*.h)

TDF := $(wildcard component/osdkseasonalplay/gen/*.tdf)
RPC := $(wildcard component/osdkseasonalplay/gen/*.rpc)

RPC_TARGETS := osdkseasonalplayslave osdkseasonalplaymaster

# Define component specific config files here to include in the VS proj.
CFG := $(wildcard component/osdkseasonalplay/cfg/*.cfg)

# Define makefile type to include in the VS proj.
MAK := component