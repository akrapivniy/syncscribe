ifndef CC
CC = gcc
endif
ifndef LD
LD = gcc
endif


SOURCES=syncsmon.c
OBJECTS=$(SOURCES:.c=.o)


CFLAGS		+= -g -Wall -Wextra -I../libsyncs -I../include -Wno-unused-parameter -Wformat-truncation=0
LDFLAGS		+= -L../libsyncs  -lsyncs -lsyncs-net -lpthread -lconfig
all: syncslib syncsmon

syncslib:
	$(MAKE) -C ../libsyncs

syncsmon: $(OBJECTS)
	$(CC)  $(OBJECTS) -o $@.bin $(LDFLAGS)

.o:  $(SOURCES)
	$(CC) -I./ -c $(CFLAGS) $< 

clean:
	rm -f *.lst *.o *.elf *.bin *.d
