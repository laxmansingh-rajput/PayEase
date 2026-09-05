CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I.
TARGET = payease
SRCS = main.cpp tests.cpp

all: $(TARGET)

$(TARGET): $(SRCS) tests.h engine.h commission.h store.h audit.h models.h bits/stdc++.h
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

test: $(TARGET)
	./$(TARGET) --test

clean:
	rm -f $(TARGET)

.PHONY: all test clean
