CC = gcc
CFLAGS = -Wall -Winline -pipe -I../../include -I../../libsyncs -I../tools -Wno-multichar -Wformat-truncation=0
LIBS =  -L../../libsyncs  -pthread  -lsyncs -lsyncs-net -lconfig

all:syncslib syncs-test-server syncs-test-client syncs-test-monitor

syncslib:
	$(MAKE) -C ../../libsyncs

syncs-test-server:
	@$(CC) $(CFLAGS) $@.c  -o $@.bin $(LIBS)

syncs-test-monitor:
	@$(CC) $(CFLAGS) $@.c  -o $@.bin $(LIBS)

syncs-test-client:
	@$(CC) $(CFLAGS) $@.c  -o $@.bin $(LIBS)

clean:
	rm -f *.o *.bin

