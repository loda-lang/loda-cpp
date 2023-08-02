CXXFLAGS += -I. -O2 -g -Wall -Werror -fmessage-length=0 -std=c++17

ifdef LODA_VERSION
CXXFLAGS += -DLODA_VERSION=$(LODA_VERSION)
endif

ifdef LODA_PLATFORM
CXXFLAGS += -DLODA_PLATFORM=$(LODA_PLATFORM)
endif

OBJS = cmd/benchmark.o cmd/boinc.o cmd/commands.o cmd/main.o cmd/test.o \
  form/expression_util.o form/expression.o form/formula_gen.o form/formula.o form/pari.o \
  lang/analyzer.o lang/big_number.o lang/comments.o lang/evaluator.o lang/evaluator_inc.o lang/interpreter.o lang/memory.o lang/minimizer.o lang/number.o lang/optimizer.o lang/parser.o lang/program.o lang/program_util.o lang/semantics.o lang/sequence.o \
  mine/api_client.o mine/blocks.o mine/config.o mine/distribution.o mine/extender.o mine/finder.o mine/generator_v1.o mine/generator_v2.o mine/generator_v3.o mine/generator_v4.o mine/generator_v5.o mine/generator_v6.o mine/generator_v7.o mine/generator_v8.o mine/generator.o mine/iterator.o mine/matcher.o mine/miner.o mine/mutator.o mine/reducer.o mine/stats.o \
  oeis/oeis_list.o oeis/oeis_manager.o oeis/oeis_program.o oeis/oeis_sequence.o \
  sys/file.o sys/git.o sys/jute.o sys/log.o sys/metrics.o sys/process.o sys/setup.o sys/util.o sys/web_client.o

loda: sys/jute.h sys/jute.cpp $(OBJS)
	$(CXX) $(LDFLAGS) -o loda $(OBJS)
	[ -L ../loda ] || ( cd .. && ln -s src/loda loda )
	du -sh loda

sys/jute.h:
	curl -sS -o sys/jute.h https://raw.githubusercontent.com/ckrause/jute/master/jute.h

sys/jute.cpp:
	curl -sS -o sys/jute.cpp https://raw.githubusercontent.com/ckrause/jute/master/jute.cpp

clean:
	rm -R -f $(OBJS) loda ../loda sys/jute.h sys/jute.cpp
