CFLAGS=-Wall -Werror -Wextra -Wpedantic -Wno-unused-parameter -Wconversion -Wformat-security -Wformat -Wsign-conversion -Wfloat-conversion -Wunused-result
LIBS=-levdev
OBJ=deckrypt_input.o

deckrypt_input: $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	$(RM) deckrypt_input $(OBJ)

.PHONY: clean
