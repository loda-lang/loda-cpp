# BEGIN PLATFORM CONFIG FOR MAC OS ARM64
CXX = clang++
CXXFLAGS = -target arm64-apple-macos11
LDFLAGS = -target arm64-apple-macos11
# END PLATFORM CONFIG FOR MAC OS ARM64

include Makefile
