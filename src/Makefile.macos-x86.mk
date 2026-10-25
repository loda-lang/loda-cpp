# BEGIN PLATFORM CONFIG FOR MAC OS X86
CXX = clang++
CXXFLAGS = -target x86_64-apple-macos14 -flto=thin
LDFLAGS = -target x86_64-apple-macos14 -flto=thin
CURL_LIBS = -lcurl
BOOST_PREFIX := $(shell brew --prefix boost 2>/dev/null)
ifneq ($(strip $(BOOST_PREFIX)),)
CXXFLAGS += -I$(BOOST_PREFIX)/include
endif
# END PLATFORM CONFIG FOR MAC OS X86

include Makefile
