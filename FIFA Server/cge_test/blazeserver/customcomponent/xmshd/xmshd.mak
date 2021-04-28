SRC := $(wildcard component/xmshd/*.cpp) \
       $(wildcard component/xmshd/$(CUSTOMDIR)/*.cpp)


HDR := $(wildcard component/xmshd/*.h)

TDF := $(wildcard component/xmshd/gen/*.tdf)
RPC := $(wildcard component/xmshd/gen/*.rpc)

RPC_TARGETS := xmshdslave xmshdmaster

# Define makefile type to include in the VS proj.
MAK := component