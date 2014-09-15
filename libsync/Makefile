PREFIX?=/usr/local

all: libsync.so sync_test

libsync.so: sync.c
	gcc -shared -fPIC -Iinclude -o $@ $^

sync_test: sync.c sync_test.c
	gcc -Iinclude -pthread -o $@ $^

install: libsync.so include/sync/sync.h
	install -m 0755 libsync.so $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/sync
	install -m 0644 include/sync/* $(PREFIX)/include/sync

clean:
	rm -f libsync.so sync_test

.PHONY: all install clean
