CXXFLAGS=-O2 -g -Wall -fmessage-length=0 -std=c++14

OBJS=database.o generator.o iterator.o loda.o interpreter.o number.o oeis.o optimizer.o parser.o printer.o serializer.o test.o util.o

LIBS=

TARGET=../loda

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
