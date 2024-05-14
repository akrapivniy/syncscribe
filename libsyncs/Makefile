ifndef CC
CC = gcc
endif
ifndef AR
AR = ar
endif

CFLAGS = -Wall -Winline -Wno-multichar -pipe -I../include -fPIC -Wformat-truncation=0 -g

LFLAGS = -lpthread -lopenssl
SYNCS_NET_SRC = syncs-net-client.c syncs-net-server.c syncs-net-common.c
SYNCS_NET_OBJ = $(SYNCS_NET_SRC:.c=.o)
SYNCS_NET_LIB = libsyncs-net.a

SYNCS_SRC = syncs-crypt.c syncs-client.c syncs-server.c
SYNCS_OBJ = $(SYNCS_SRC:.c=.o)
SYNCS_LIB = libsyncs.a
SYNCS_LIB_DYN = libsyncs.so.1

SYNCS_TYPE_HEADER_FILE := syncs-types.h
SYNCS_PYTHON := syncstypes.py

all:$(SYNCS_NET_LIB) $(SYNCS_LIB) $(SYNCS_LIB_DYN) $(SYNCS_PYTHON)

$(SYNCS_NET_LIB): $(SYNCS_NET_OBJ)
	@$(AR) rcs $@ $^

$(SYNCS_LIB): $(SYNCS_OBJ) $(SYNCS_NET_OBJ)
	@$(AR) rcs $@ $^

$(SYNCS_PYTHON): $(SYNCS_TYPE_HEADER_FILE)
	@echo "# This file is generated automatically. Do not edit." > $(SYNCS_PYTHON)
	@grep '^#define' $(SYNCS_TYPE_HEADER_FILE) | grep ' SYNCS_TYPE' | awk '{printf "%s = ", $$2; if ($$3 ~ /^[0-9]+$$/) print $$3; else if ($$3 ~ /^.*$$/) print $$3; else print $$3 }'  >> $(SYNCS_PYTHON)
	@mv $(SYNCS_PYTHON) ../python/

$(SYNCS_LIB_DYN): $(SYNCS_OBJ) $(SYNCS_NET_OBJ)
	@$(CC) -shared -Wl,-soname,$(SYNCS_LIB_DYN) -o $(SYNCS_LIB_DYN).0 $^

.c.o:
	@$(CC) -c $(CFLAGS) $< -o $@ $(LFLAGS)

clean:
	rm -f $(SYNCS_NET_OBJ) $(SYNCS_NET_LIB) $(SYNCS_OBJ) $(SYNCS_LIB) $(SYNCS_LIB_DYN).0
