SRC := $(wildcard component/easfc/*.cpp) \
       $(wildcard component/easfc/$(CUSTOMDIR)/*.cpp)


HDR := $(wildcard component/easfc/*.h)

TDF := $(wildcard component/easfc/gen/*.tdf)
RPC := $(wildcard component/easfc/gen/*.rpc)

RPC_TARGETS := easfcslave easfcmaster

# Define makefile type to include in the VS proj.
MAK := component