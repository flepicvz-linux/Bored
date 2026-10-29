CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/bored
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin

.PHONY: all check-deps run install uninstall clean

all: $(TARGET)

check-deps:
	@command -v $(CXX) >/dev/null 2>&1 || { echo "C++ compiler '$(CXX)' not found. Install g++ first."; exit 1; }

$(TARGET): main.cpp | check-deps
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

run: $(TARGET)
	./$(TARGET)

install: $(TARGET)
	@mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/bored

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/bored

clean:
	rm -rf $(BUILD_DIR)