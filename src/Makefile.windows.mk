CXXFLAGS = /Iinclude /Iexternal /O2 /std:c++20

!IFDEF LODA_VERSION
CXXFLAGS = $(CXXFLAGS) /DLODA_VERSION=$(LODA_VERSION)
!ELSE
CXXFLAGS = $(CXXFLAGS) /Z7
!ENDIF

!IFDEF LODA_PLATFORM
CXXFLAGS = $(CXXFLAGS) /DLODA_PLATFORM=$(LODA_PLATFORM)
!ENDIF

SRCS = api_client.cpp benchmark.cpp big_number.cpp blocks.cpp boinc.cpp commands.cpp comments.cpp config.cpp distribution.cpp evaluator.cpp evaluator_inc.cpp evaluator_log.cpp expression.cpp expression_util.cpp extender.cpp external/jute.cpp file.cpp finder.cpp formula.cpp formula_gen.cpp generator.cpp generator_v1.cpp generator_v2.cpp generator_v3.cpp generator_v4.cpp generator_v5.cpp generator_v6.cpp generator_v7.cpp git.cpp interpreter.cpp iterator.cpp log.cpp main.cpp matcher.cpp memory.cpp metrics.cpp miner.cpp minimizer.cpp mutator.cpp number.cpp oeis_list.cpp oeis_manager.cpp oeis_program.cpp oeis_sequence.cpp optimizer.cpp pari.cpp parser.cpp process.cpp program.cpp program_util.cpp reducer.cpp semantics.cpp sequence.cpp setup.cpp stats.cpp test.cpp util.cpp web_client.cpp

loda: external/jute.h external/jute.cpp $(SRCS)
	cl /EHsc /Feloda.exe $(CXXFLAGS) $(SRCS)
	copy loda.exe ..

external/jute.h:
	@if not exist external mkdir external
	curl -sS -o external/jute.h https://raw.githubusercontent.com/amir-s/jute/master/jute.h

external/jute.cpp:
	@if not exist external mkdir external
	curl -sS -o external/jute.cpp https://raw.githubusercontent.com/amir-s/jute/master/jute.cpp

clean:
	del /f $(OBJS) loda ../loda external
