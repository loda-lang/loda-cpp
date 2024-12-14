# BEGIN PLATFORM CONFIG FOR MAC OS X86
CXX = clang++
CXXFLAGS = -target x86_64-apple-macos14
LDFLAGS = -target x86_64-apple-macos14
# END PLATFORM CONFIG FOR MAC OS X86

include Makefile
