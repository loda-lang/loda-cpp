CXXFLAGS=-O2 -g -Wall -fmessage-length=0 -std=c++14

OBJS=cmtool.o

LIBS=

TARGET=cmtool

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
