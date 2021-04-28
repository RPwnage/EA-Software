SRC := $(wildcard component/fifacups/*.cpp) \
       $(wildcard component/fifacups/$(CUSTOMDIR)/*.cpp)


HDR := $(wildcard component/fifacups/*.h)

TDF := $(wildcard component/fifacups/gen/*.tdf)
RPC := $(wildcard component/fifacups/gen/*.rpc)

RPC_TARGETS := fifacupsslave fifacupsmaster

# Define component specific config files here to include in the VS proj.
CFG := $(wildcard component/fifacups/cfg/*.cfg)

# Define makefile type to include in the VS proj.
MAK := component