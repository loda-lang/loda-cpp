CXXFLAGS=-Iinclude -Iexternal -O2 -g -Wall -fmessage-length=0 -std=c++11

OBJS=extender.o external/jute.o finder.o generator.o generator_v1.o generator_v2.o generator_v3.o interpreter.o main.o matcher.o memory.o miner.o minimizer.o mutator.o number.o oeis.o optimizer.o parser.o program.o program_util.o reducer.o semantics.o stats.o synthesizer.o test.o util.o

LIBS=

TARGET=../loda

$(TARGET): init $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

init:
	[ -f external/jute.h ] || curl -sS -o external/jute.h https://raw.githubusercontent.com/amir-s/jute/master/jute.h
	[ -f external/jute.cpp ] || curl -sS -o external/jute.cpp https://raw.githubusercontent.com/amir-s/jute/master/jute.cpp

clean:
	rm -f $(OBJS) $(TARGET) external/*

test: $(TARGET)
	cd .. && ./loda test && cd src
