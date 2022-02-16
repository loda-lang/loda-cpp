# BEGIN PLATFORM CONFIG FOR MAC OS X86
CXX = clang++
CXXFLAGS = -target x86_64-apple-macos11
LDFLAGS = -target x86_64-apple-macos11
# END PLATFORM CONFIG FOR MAC OS X86

include Makefile
