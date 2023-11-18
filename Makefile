SRC := src/*
INC := -Iinc -I/usr/include/libevdev-1.0 -Ikbd/include
LDFLAGS := -Lkbd/lib -l:libkeymap.a -l:libkbdfile.a -levdev -lm
CFLAGS := -Wall -Werror -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-function -Wconversion -Wformat-security -Wformat -Wsign-conversion -Wfloat-conversion -Wunused-result

KBD_LIBS := kbd/lib/libkeymap.a kbd/lib/libkbdfile.a

deckrypt_input: $(SRC) $(KBD_LIBS)
	$(CC) $(CFLAGS) $(INC) $(SRC) -o $@ $(LDFLAGS)

$(KBD_LIBS):
	$(shell [ -d kbd ] || git clone -b v2.6.3 https://git.kernel.org/pub/scm/linux/kernel/git/legion/kbd.git)
	cd kbd && ./autogen.sh
	cd kbd && ./configure --prefix=$(CURDIR)/kbd --enable-libkeymap --disable-tests
	$(MAKE) -C kbd install

clean:
	rm -rf deckrypt_input kbd

.PHONY: clean
