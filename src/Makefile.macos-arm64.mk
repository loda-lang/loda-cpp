# BEGIN PLATFORM CONFIG FOR MAC OS ARM64
CXX = clang++
CXXFLAGS = -target arm64-apple-macos14 -flto=thin
LDFLAGS = -target arm64-apple-macos14 -flto=thin
CURL_LIBS = -lcurl
# END PLATFORM CONFIG FOR MAC OS ARM64

include Makefile
