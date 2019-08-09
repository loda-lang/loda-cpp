CXXFLAGS=-O2 -g -Wall -fmessage-length=0 -std=c++11

OBJS=generator.o iterator.o interpreter.o main.o matcher.o miner.o number.o oeis.o optimizer.o parser.o printer.o program.o synthesizer.o test.o util.o

LIBS=

TARGET=../loda

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
