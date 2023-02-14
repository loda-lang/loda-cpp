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
  form/expression_util.cpp form/expression.cpp form/formula_gen.cpp form/formula.cpp form/pari.cpp \
  lang/big_number.cpp lang/comments.cpp lang/evaluator.cpp lang/evaluator_inc.cpp lang/evaluator_log.cpp lang/interpreter.cpp lang/memory.cpp lang/minimizer.cpp lang/number.cpp lang/optimizer.cpp lang/parser.cpp lang/program.cpp lang/program_util.cpp  lang/semantics.cpp lang/sequence.cpp \
  mine/api_client.cpp mine/blocks.cpp mine/config.cpp mine/distribution.cpp mine/extender.cpp mine/finder.cpp mine/generator.cpp mine/generator_v1.cpp mine/generator_v2.cpp mine/generator_v3.cpp mine/generator_v4.cpp mine/generator_v5.cpp mine/generator_v6.cpp mine/generator_v7.cpp mine/iterator.cpp mine/matcher.cpp mine/miner.cpp mine/mutator.cpp mine/reducer.cpp mine/stats.cpp \
  oeis/oeis_list.cpp oeis/oeis_manager.cpp oeis/oeis_program.cpp oeis/oeis_sequence.cpp \
  sys/file.cpp sys/git.cpp sys/jute.cpp sys/log.cpp sys/metrics.cpp sys/process.cpp sys/setup.cpp sys/util.cpp sys/web_client.cpp

loda: sys/jute.h sys/jute.cpp $(SRCS)
	cl /EHsc /Feloda.exe $(CXXFLAGS) $(SRCS)
	copy loda.exe ..

sys/jute.h:
	curl -sS -o sys/jute.h https://raw.githubusercontent.com/amir-s/jute/master/jute.h

sys/jute.cpp:
	curl -sS -o sys/jute.cpp https://raw.githubusercontent.com/amir-s/jute/master/jute.cpp

clean:
	del /f $(OBJS) loda ../loda sys/jute.h sys/jute.cpp
