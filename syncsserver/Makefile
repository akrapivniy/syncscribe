ifndef CC
CC = gcc
endif
ifndef LD
LD = gcc
endif

SOURCES=syncsserver.c
OBJECTS=$(SOURCES:.c=.o)

CFLAGS		+= -g -Wall -Wextra -I../libsyncs -I../include
LDFLAGS		+= -L../libsyncs -lsyncs -lsyncs-net -lpthread -ljson-c
all: syncslib syncsserver

syncslib:
	$(MAKE) -C ../libsyncs

syncsserver: $(OBJECTS)
	$(CC)  $(OBJECTS) -o $@.bin $(LDFLAGS)

.o:  $(SOURCES)
	$(CC) -I./ -c $(CFLAGS) $<

clean:
	rm -f *.lst *.o *.elf *.bin *.d
