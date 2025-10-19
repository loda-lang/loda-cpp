# BEGIN PLATFORM CONFIG FOR MAC OS ARM64
CXX = clang++
#CXXFLAGS = -target arm64-apple-macos14 -flto=thin -fprofile-generate
#LDFLAGS = -target arm64-apple-macos14 -flto=thin -fprofile-generate
CXXFLAGS = -target arm64-apple-macos14 -flto=thin -fprofile-use=../mine_1h.profdata -Wno-error=backend-plugin
LDFLAGS = -target arm64-apple-macos14 -flto=thin -fprofile-use=../mine_1h.profdata
#CXXFLAGS = -target arm64-apple-macos14 -flto=thin
#LDFLAGS = -target arm64-apple-macos14 -flto=thin
# END PLATFORM CONFIG FOR MAC OS ARM64

include Makefile
