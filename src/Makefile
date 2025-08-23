CXXFLAGS += -I. -g -Wall -Werror -fmessage-length=0 -std=c++17

ifdef LODA_VERSION
CXXFLAGS += -DLODA_VERSION=$(LODA_VERSION)
endif

ifdef LODA_PLATFORM
CXXFLAGS += -DLODA_PLATFORM=$(LODA_PLATFORM)
endif

OBJS = base/uid.o \
  cmd/benchmark.o cmd/boinc.o cmd/commands.o cmd/main.o cmd/test.o \
  eval/evaluator.o eval/evaluator_inc.o eval/evaluator_par.o eval/evaluator_vir.o eval/fold.o eval/interpreter.o eval/memory.o eval/minimizer.o eval/optimizer.o eval/range.o eval/range_generator.o eval/semantics.o \
  form/expression_util.o form/expression.o form/formula_gen.o form/formula_util.o form/formula.o form/pari.o form/variant.o \
  lang/analyzer.o lang/comments.o lang/constants.o lang/parser.o lang/program.o lang/program_cache.o lang/program_util.o lang/subprogram.o lang/virtual_seq.o \
  math/big_number.o math/number.o math/sequence.o \
  mine/api_client.o mine/blocks.o mine/checker.o mine/config.o mine/distribution.o mine/extender.o mine/finder.o mine/generator_v1.o mine/generator_v2.o mine/generator_v3.o mine/generator_v4.o mine/generator_v5.o mine/generator_v6.o mine/generator_v7.o mine/generator_v8.o mine/generator.o mine/iterator.o mine/matcher.o mine/miner.o mine/mutator.o mine/reducer.o mine/stats.o \
  oeis/invalid_matches.o oeis/oeis_list.o oeis/oeis_manager.o oeis/oeis_program.o \
  seq/managed_sequence.o \
  sys/file.o sys/git.o sys/jute.o sys/log.o sys/metrics.o sys/process.o sys/setup.o sys/util.o sys/web_client.o

loda: CXXFLAGS += -O2
loda: $(OBJS)
	$(CXX) $(LDFLAGS) -o loda $(OBJS)
	[ -L ../loda ] || ( cd .. && ln -s src/loda loda )
	du -sh loda

coverage: CXXFLAGS += -O0 -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += --coverage
coverage: $(OBJS)
	$(CXX) $(LDFLAGS) -o loda $(OBJS)
	[ -L ../loda ] || ( cd .. && ln -s src/loda loda )
	cd .. && ./loda test && cd src
	gcov $(OBJS:.o=.cpp)
	mkdir coverage
	lcov -c -d . -o coverage/coverage.info
	lcov -r coverage/coverage.info '/Library/*' '/usr/*' -o coverage/coverage-clean.info
	genhtml coverage/coverage-clean.info -o coverage

clean:
	rm -R -f $(OBJS) $(OBJS:.o=.gcda) $(OBJS:.o=.gcno) *.gcov coverage loda ../loda
