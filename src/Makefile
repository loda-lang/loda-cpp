CXXFLAGS=-Iinclude -Iexternal -O2 -g -Wall -fmessage-length=0 -std=c++11

SRC_DIR=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
OBJS=big_number.o blocks.o config.o evaluator.o extender.o external/jute.o finder.o generator.o generator_v1.o generator_v2.o generator_v3.o generator_v4.o generator_v5.o interpreter.o iterator.o main.o matcher.o memory.o miner.o minimizer.o mutator.o number.o oeis_manager.o oeis_maintenance.o oeis_sequence.o optimizer.o parser.o program.o program_util.o reducer.o semantics.o sequence.o stats.o test.o util.o
LIBS=

loda: external/jute.h external/jute.cpp $(OBJS)
	$(CXX) -o loda $(OBJS) $(LIBS)
	[ -L ../loda ] || ( cd .. && ln -s src/loda loda )

external/jute.h:
	mkdir -p external && curl -sS -o external/jute.h https://raw.githubusercontent.com/amir-s/jute/master/jute.h

external/jute.cpp:
	mkdir -p external && curl -sS -o external/jute.cpp https://raw.githubusercontent.com/amir-s/jute/master/jute.cpp

define crossbuild_loda
	docker run --rm -v $(SRC_DIR):/workdir -e CROSS_TRIPLE=$(1) multiarch/crossbuild make loda $(2)
	mkdir -p build/$(1)
	mv loda build/$(1)
endef

x86_64-linux-gnu: clean-core
	$(call crossbuild_loda,x86_64-linux-gnu)
	make clean-core

x86_64-apple-darwin: clean-core
	$(call crossbuild_loda,x86_64-apple-darwin,CXX=clang)
	make clean-core

x86_64-w64-mingw32: clean-core
	$(call crossbuild_loda,x86_64-w64-mingw32)
	make clean-core

multi-platform: x86_64-linux-gnu x86_64-apple-darwin x86_64-w64-mingw32

clean-core:
	rm -R -f $(OBJS) loda ../loda external

clean: clean-core
	rm -R -f build

test: loda
	cd .. && ./loda test

all: loda
