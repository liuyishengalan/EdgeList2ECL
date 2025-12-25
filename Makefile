CXX      := g++
CXXFLAGS := -O3 -std=c++17

TARGET := edgelist2ecl
SRCS   := edgelist2ecl.cpp
HDRS   := ECLgraph.h

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
