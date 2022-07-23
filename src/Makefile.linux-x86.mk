# BEGIN PLATFORM CONFIG FOR LINUX X86
# see https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
CXX = x86_64-linux-gnu-g++
CXXFLAGS += -pthread
LDFLAGS = -static -static-libstdc++ -static-libgcc -lrt -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
# END PLATFORM CONFIG FOR LINUX X86

include Makefile
