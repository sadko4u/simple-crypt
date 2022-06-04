# Command-line flag to silence nested $(MAKE).
ifneq ($(VERBOSE),1)
.SILENT:
endif

INSTALL ?= install
BINARY = simple-crypt
PREFIX = /usr/local/bin

# Source code
FILES = \
  simple-crypt.cpp

HEADERS = \
  arguments.h \
  crypto.h \
  processing.h

# Flags
CXXFLAGS ?= \
  -fvisibility=hidden \
  -fno-rtti \
  -fno-exceptions \
  -O2 \
  -std=c++17 \
  -fdata-sections \
  -ffunction-sections \
  -fPIC \
  -fno-asynchronous-unwind-tables \
  -pipe\
  -Wall \
  -Wl,-z,relro,-z,now \
  -Wl,--gc-sections

# Dependencies
$(BINARY): $(FILES) $(HEADERS)

# Goals
.DEFAULT_GOAL              := all
.PHONY: all compile install uninstall clean

all: compile

compile: $(BINARY)
	echo "Build OK"

$(BINARY):
	echo "  $(CXX) $(BINARY)"
	$(CXX) -o $(BINARY) $(CXXFLAGS) $(FILES)

clean:
	-rm -f $(BINARY)
	echo "Clean OK"

install:
	$(INSTALL) $(BINARY) $(DESTDIR)$(PREFIX)/$(BINARY)
	echo "Install OK"

uninstall:
	-rm -f $(DESTDIR)$(PREFIX)/$(BINARY)
	echo "Uninstall OK"

