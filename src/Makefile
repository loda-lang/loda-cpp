CXXFLAGS = -Iinclude -Iexternal -O2 -g -Wall -fmessage-length=0 -std=c++11

ifdef LODA_VERSION
CXXFLAGS += -DLODA_VERSION=$(LODA_VERSION)
endif

OBJS = api_client.o big_number.o blocks.o commands.o config.o evaluator.o extender.o external/jute.o file.o finder.o generator.o generator_v1.o generator_v2.o generator_v3.o generator_v4.o generator_v5.o interpreter.o iterator.o main.o matcher.o memory.o metrics.o miner.o minimizer.o mutator.o number.o oeis_list.o oeis_manager.o oeis_maintenance.o oeis_sequence.o optimizer.o parser.o program.o program_util.o reducer.o semantics.o setup.o sequence.o stats.o test.o util.o

loda: external/jute.h external/jute.cpp $(OBJS)
	$(CXX) -o loda $(OBJS) $(LIBS)
	[ -L ../loda ] || ( cd .. && ln -s src/loda loda )

external/jute.h:
	mkdir -p external && curl -sS -o external/jute.h https://raw.githubusercontent.com/amir-s/jute/master/jute.h

external/jute.cpp:
	mkdir -p external && curl -sS -o external/jute.cpp https://raw.githubusercontent.com/amir-s/jute/master/jute.cpp

clean:
	rm -R -f $(OBJS) loda ../loda external

test: loda
	cd .. && ./loda test

all: test
