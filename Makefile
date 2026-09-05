CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc
TARGET = payease
SRCS = src/main.cpp src/tests.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

test: $(TARGET)
	./$(TARGET) --test

clean:
	rm -f $(TARGET) $(TARGET).exe src/payease src/payease.exe
	rm -rf *.dSYM src/*.dSYM

.PHONY: all test clean
