# BEGIN PLATFORM CONFIG FOR LINUX ARM64
# see https://stackoverflow.com/questions/35116327/when-g-static-link-pthread-cause-segmentation-fault-why
CXX = aarch64-linux-gnu-g++
CXXFLAGS += -pthread
LDFLAGS = -static -static-libstdc++ -static-libgcc -lrt -pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -s
# END PLATFORM CONFIG FOR LINUX ARM64

include Makefile
