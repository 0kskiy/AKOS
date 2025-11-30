CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lrt -pthread

all: manager pirate observer

manager: manager.cpp
	$(CXX) $(CXXFLAGS) -o manager manager.cpp $(LDFLAGS)

pirate: pirate.cpp
	$(CXX) $(CXXFLAGS) -o pirate pirate.cpp $(LDFLAGS)

observer: observer.cpp
	$(CXX) $(CXXFLAGS) -o observer observer.cpp $(LDFLAGS)

clean:
	rm -f manager pirate observer

.PHONY: all clean
