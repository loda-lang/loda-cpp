CXXFLAGS=-O2 -g -Wall -fmessage-length=0 -std=c++11

OBJS=extender.o generator.o interpreter.o main.o matcher.o miner.o number.o oeis.o optimizer.o parser.o program.o program_util.o reducer.o semantics.o synthesizer.o test.o util.o

LIBS=

TARGET=../loda

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET)
	cd .. && ./loda test && cd src
