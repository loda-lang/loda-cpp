# BEGIN PLATFORM CONFIG FOR MAC OS X86
CXX = clang++
CXXFLAGS = -target x86_64-apple-macos14 -flto=thin
LDFLAGS = -target x86_64-apple-macos14 -flto=thin
# END PLATFORM CONFIG FOR MAC OS X86

include Makefile
