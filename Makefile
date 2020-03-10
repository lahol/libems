CC=gcc
CFLAGS += -Wall -O3 -g
LIBS=-pthread

PREFIX := /usr

ems_SRC := $(filter-out test.c, $(wildcard *.c))
ems_OBJ := $(ems_SRC:.c=.o)
ems_HEADERS := $(wildcard *.h)

all: libems.so.1.0 test

libems.so.1.0: $(ems_OBJ)
	$(CC) -shared -Wl,-soname,libems.so.1 -o $@ $^ $(LIBS)
	ln -sf libems.so.1.0 libems.so.1
	ln -sf libems.so.1 libems.so

test: test.c $(ems_HEADERS)
	$(CC) $(CFLAGS) -L. -o test test.c -lems $(LIBS)

%.o: %.c $(ems_HEADERS)
	$(CC) -I. $(CFLAGS) -fPIC -c -o $@ $<

install:
	install libems.so.1.0 $(PREFIX)/lib/
	ln -sf $(PREFIX)/lib/libems.so.1.0 $(PREFIX)/lib/libems.so.1
	ln -sf $(PREFIX)/lib/libems.so.1 $(PREFIX)/lib/libems.so
	cp ems-peer.h ems-message.h ems-msg-queue.h ems-communicator.h ems-util.h ems-util-list.h ems-util-fd.h ems-status-messages.h ems.h ems-error.h ems-memory.h ems-types.h $(PREFIX)/include

clean:
	$(RM) libems.so* $(ems_OBJ)

.PHONY: all clean install
