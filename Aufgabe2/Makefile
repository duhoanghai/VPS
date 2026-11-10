# Aufgabe 2: kompakter Build ohne erforderlichen X11-Server.
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -Wextra -pedantic -Iinclude
LDFLAGS = -pthread
COMMON = src/board/LocalBoard.cc src/io/Rle.cc src/simulation/Life.cc
NETWORK = src/net/UDPNetwork.cc src/net/TCPNetwork.cc

.PHONY: all clean test
all: bin/server bin/client bin/local

bin:
	mkdir -p bin

bin/server: src/server.cc src/board/BoardServer.cc $(NETWORK) $(COMMON) | bin
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

bin/client: src/client.cc src/client/LifeClient.cc $(NETWORK) | bin
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

bin/local: src/local.cc $(COMMON) | bin
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

test: all
	python3 tests/test_integration.py

clean:
	rm -rf bin
