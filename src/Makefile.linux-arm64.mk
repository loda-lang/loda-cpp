# BEGIN PLATFORM CONFIG FOR LINUX ARM64
CXX = aarch64-linux-gnu-g++
LDFLAGS = -static -static-libstdc++ -static-libgcc
# END PLATFORM CONFIG FOR LINUX ARM64

include Makefile