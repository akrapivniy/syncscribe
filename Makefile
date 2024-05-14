ifndef CC
CC = gcc
endif
ifndef LD
LD = gcc
endif

SUBDIRS = libsyncs syncsserver syncsctrl syncsmon tests

ifndef SOURCES_DIR
SOURCES_DIR := $(shell ( pwd -L ) )
endif

all clean install:
	$(foreach dir, $(SUBDIRS), $(MAKE) -C $(SOURCES_DIR)/$(dir) \
	SOURCES_DIR=$(SOURCES_DIR)/$(dir) $@ || exit 1; )