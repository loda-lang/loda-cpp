CXXFLAGS += -Iinclude -Iexternal -O2 -g -Wall -Werror -fmessage-length=0 -std=c++20

ifdef LODA_VERSION
CXXFLAGS += -DLODA_VERSION=$(LODA_VERSION)
endif

ifdef LODA_PLATFORM
CXXFLAGS += -DLODA_PLATFORM=$(LODA_PLATFORM)
endif

OBJS = api_client.o benchmark.o big_number.o blocks.o boinc.o commands.o comments.o config.o distribution.o evaluator.o evaluator_inc.o evaluator_log.o expression.o expression_util.o extender.o external/jute.o file.o finder.o formula.o formula_gen.o generator.o generator_v1.o generator_v2.o generator_v3.o generator_v4.o generator_v5.o generator_v6.o generator_v7.o git.o interpreter.o iterator.o log.o main.o matcher.o memory.o metrics.o miner.o minimizer.o mutator.o number.o oeis_list.o oeis_manager.o oeis_program.o oeis_sequence.o optimizer.o pari.o parser.o process.o program.o program_util.o reducer.o semantics.o setup.o sequence.o stats.o test.o util.o web_client.o

loda: external/jute.h external/jute.cpp $(OBJS)
	$(CXX) $(LDFLAGS) -o loda $(OBJS)
	[ -L ../loda ] || ( cd .. && ln -s src/loda loda )
	du -sh loda

external/jute.h:
	mkdir -p external && curl -sS -o external/jute.h https://raw.githubusercontent.com/ckrause/jute/master/jute.h

external/jute.cpp:
	mkdir -p external && curl -sS -o external/jute.cpp https://raw.githubusercontent.com/ckrause/jute/master/jute.cpp

clean:
	rm -R -f $(OBJS) loda ../loda external
