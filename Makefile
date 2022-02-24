INC=-I/usr/include/libevdev-1.0/ -Isrc
CFLAGS=-Wall -Werror -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-function -Wconversion -Wformat-security -Wformat -Wsign-conversion -Wfloat-conversion -Wunused-result $(INC)
LIBS=-levdev -lkeymap -lm
OBJ=src/uitype.o src/deckrypt_input.o
deckrypt_input: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS) 

clean:
	$(RM) deckrypt_input $(OBJ)

.PHONY: clean
