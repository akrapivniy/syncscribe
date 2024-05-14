CC      = gcc
LD      = gcc

SOURCES=syncsctrl.c
OBJECTS=$(SOURCES:.c=.o)

CFLAGS		+= -g -Wall -Wextra -I../include -I../libsyncs -Wno-unused-parameter -Wformat-truncation=0
LDFLAGS		+= -L../libsyncs -L../librn  -lsyncs -lsyncs-net -lpthread -lconfig
all: syncslib syncsctrl

syncslib:
	$(MAKE) -C ../libsyncs

syncsctrl: $(OBJECTS)
	$(LD)  $(OBJECTS) -o $@.bin $(LDFLAGS)

.o:  $(SOURCES)
	$(CC) -I./ -c $(CFLAGS) $< 

clean:
	rm -f *.lst *.o *.elf *.bin *.d
