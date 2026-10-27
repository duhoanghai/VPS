CXX = g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -O2 -g -Wall -Wextra -pedantic -std=c++11
LDLIBS += -lX11 -pthread

# XQuartz unter macOS; Linux verwendet die Standardpfade des Compilers.
ifeq ($(shell uname -s),Darwin)
CPPFLAGS += -I/opt/X11/include
LDFLAGS += -L/opt/X11/lib
endif

# Zusätzliche Implementierungen hier eintragen; main.cc wird separat eingebunden.
SRC_FILES = board/LocalBoard.cc io/Rle.cc simulation/Life.cc gui/DrawingWindow.cc
OBJECTS = $(patsubst %.cc,tmp/%.o,$(SRC_FILES))
CORE_OBJECTS = $(filter-out tmp/gui/DrawingWindow.o,$(OBJECTS))
DEPS = $(OBJECTS:.o=.d) tmp/main.d

.PHONY: all main depend directories clean test test-gui
all: bin/main
main: bin/main

bin/main: $(OBJECTS) tmp/main.o
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

# .cc-Dateien einzeln übersetzen; .d-Dateien erfassen die verwendeten Header.
tmp/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -pthread -MMD -MP -c $< -o $@

# Befehl aus der Vorgabe beibehalten; normale Builds erzeugen ebenfalls Dependencies.
depend:
	@set -e; for src in $(SRC_FILES) main.cc; do \
	  dep=tmp/$${src%.cc}.d; mkdir -p "$$(dirname "$$dep")"; \
	  $(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MP -MT "tmp/$${src%.cc}.o" "src/$$src" -MF "$$dep"; \
	done

directories:
	@mkdir -p bin tmp

bin/test_life: tests/test_life.cc $(CORE_OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

bin/test_thread: tests/test_thread.cc include/thread/Thread.h include/misc/debug.h
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -pthread $< -o $@

test: bin/test_life bin/test_thread bin/main
	./bin/test_life
	./bin/test_thread
	python3 tests/test_cli.py

bin/gui_probe: tests/gui_probe.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $< $(LDLIBS) -o $@

test-gui: bin/main bin/gui_probe
	python3 tests/test_gui.py

clean:
	rm -rf tmp bin

-include $(DEPS)
