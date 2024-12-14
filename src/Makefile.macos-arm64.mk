# BEGIN PLATFORM CONFIG FOR MAC OS ARM64
CXX = clang++
CXXFLAGS = -target arm64-apple-macos14
LDFLAGS = -target arm64-apple-macos14
# END PLATFORM CONFIG FOR MAC OS ARM64

include Makefile
