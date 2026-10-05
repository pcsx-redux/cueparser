# The sources include each other as "cueparser/...", so they're built through a cueparser/ symlink
# to the repository root.

ROOT := $(abspath ..)
BUILD ?= build
CC ?= cc
CFLAGS ?= -O1 -g -Wall -fsanitize=address,undefined -fno-sanitize-recover=undefined
FUZZ_CC ?= clang
FUZZ_CFLAGS ?= -O1 -g -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined

SRCS := $(ROOT)/cueparser.c $(ROOT)/fileabstract.c $(ROOT)/scheduler.c harness.c
INCLUDE := $(BUILD)/include

all: $(BUILD)/cuedump

$(INCLUDE)/cueparser:
	mkdir -p $(INCLUDE)
	ln -sfn $(ROOT) $@

$(BUILD)/cuedump: $(SRCS) cuedump.c harness.h | $(INCLUDE)/cueparser
	$(CC) $(CFLAGS) -I$(INCLUDE) -I. -o $@ $(SRCS) cuedump.c

$(BUILD)/fuzz: $(SRCS) fuzz.c harness.h | $(INCLUDE)/cueparser
	$(FUZZ_CC) $(FUZZ_CFLAGS) -I$(INCLUDE) -I. -o $@ $(SRCS) fuzz.c

check: $(BUILD)/cuedump
	./check.sh $(BUILD)/cuedump

gold: $(BUILD)/cuedump
	./check.sh $(BUILD)/cuedump --update

fuzz: $(BUILD)/fuzz

clean:
	rm -rf $(BUILD)

.PHONY: all check gold fuzz clean
