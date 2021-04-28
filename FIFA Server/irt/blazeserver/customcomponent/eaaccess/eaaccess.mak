SRC := $(wildcard component/eaaccess/*.cpp) \
       $(wildcard component/eaaccess/$(CUSTOMDIR)/*.cpp)


HDR := $(wildcard component/eaaccess/*.h)

TDF := $(wildcard component/eaaccess/gen/*.tdf)
RPC := $(wildcard component/eaaccess/gen/*.rpc)

RPC_TARGETS := eaaccessslave eaaccessmaster

# Define makefile type to include in the VS proj.
MAK := component