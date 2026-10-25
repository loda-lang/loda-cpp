# BEGIN PLATFORM CONFIG FOR MAC OS ARM64
CXX = clang++
CXXFLAGS = -target arm64-apple-macos14 -flto=thin
LDFLAGS = -target arm64-apple-macos14 -flto=thin
CURL_LIBS = -lcurl
BOOST_PREFIX := $(shell brew --prefix boost 2>/dev/null)
ifneq ($(strip $(BOOST_PREFIX)),)
CXXFLAGS += -I$(BOOST_PREFIX)/include
endif
# END PLATFORM CONFIG FOR MAC OS ARM64

include Makefile
