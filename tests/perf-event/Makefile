CC = gcc
CFLAGS = -Wall -Winline -pipe -I../../include -I../../libsyncs -I../tools -Wno-multichar -Wformat-truncation=0
LIBS =  -L../../libsyncs -pthread -lsyncs -lsyncs-net
OBJECTS = ../tools/test_tools.o

all:syncslib syncs-test-server syncs-test-write-client syncs-test-sync-event-client syncs-test-event-client syncs-test-monitor

syncslib:
	$(MAKE) -C ../../libsyncs

syncs-test-server:
	@$(CC) $(CFLAGS) $@.c $(OBJECTS) -o $@.bin $(LIBS)
syncs-test-monitor:
	@$(CC) $(CFLAGS) $@.c $(OBJECTS) -o $@.bin $(LIBS)
syncs-test-event-client:
	@$(CC) $(CFLAGS) $@.c $(OBJECTS) -o $@.bin $(LIBS)
syncs-test-sync-event-client:
	@$(CC) $(CFLAGS) $@.c $(OBJECTS) -o $@.bin $(LIBS)
syncs-test-write-client:
	@$(CC) $(CFLAGS) $@.c $(OBJECTS) -o $@.bin $(LIBS)

clean:
	rm -f *.o *.bin

