CXXFLAGS = /I. /O2 /std:c++17

!IFDEF LODA_VERSION
CXXFLAGS = $(CXXFLAGS) /DLODA_VERSION=$(LODA_VERSION)
!ELSE
CXXFLAGS = $(CXXFLAGS) /Z7
!ENDIF

!IFDEF LODA_PLATFORM
CXXFLAGS = $(CXXFLAGS) /DLODA_PLATFORM=$(LODA_PLATFORM)
!ENDIF

SRCS = cmd/benchmark.cpp cmd/boinc.cpp cmd/commands.cpp cmd/main.cpp cmd/test.cpp \
  eval/evaluator.cpp eval/evaluator_inc.cpp eval/evaluator_par.cpp eval/interpreter.cpp eval/memory.cpp eval/minimizer.cpp eval/optimizer.cpp eval/semantics.cpp \
  form/expression_util.cpp form/expression.cpp form/formula_gen.cpp form/formula_util.cpp form/formula.cpp form/pari.cpp form/range.cpp form/range_generator.cpp form/variant.cpp \
  lang/analyzer.cpp lang/comments.cpp lang/constants.cpp lang/parser.cpp lang/program.cpp lang/program_util.cpp lang/subprogram.cpp \
  math/big_number.cpp math/number.cpp math/sequence.cpp \
  mine/api_client.cpp mine/blocks.cpp mine/config.cpp mine/distribution.cpp mine/extender.cpp mine/finder.cpp mine/generator.cpp mine/generator_v1.cpp mine/generator_v2.cpp mine/generator_v3.cpp mine/generator_v4.cpp mine/generator_v5.cpp mine/generator_v6.cpp mine/generator_v7.cpp mine/generator_v8.cpp mine/iterator.cpp mine/matcher.cpp mine/miner.cpp mine/mutator.cpp mine/reducer.cpp mine/stats.cpp \
  oeis/oeis_list.cpp oeis/oeis_manager.cpp oeis/oeis_program.cpp oeis/oeis_sequence.cpp \
  sys/file.cpp sys/git.cpp sys/jute.cpp sys/log.cpp sys/metrics.cpp sys/process.cpp sys/setup.cpp sys/util.cpp sys/web_client.cpp

loda: $(SRCS)
	cl /EHsc /Feloda.exe $(CXXFLAGS) $(SRCS)
	copy loda.exe ..

clean:
	del /f $(OBJS) loda.exe ../loda.exe
