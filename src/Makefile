CXXFLAGS=-O2 -g -Wall -fmessage-length=0 -std=c++14

OBJS=evaluator.o generator.o loda.o interpreter.o parser.o printer.o sequence.o test.o

LIBS=

TARGET=../loda

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
